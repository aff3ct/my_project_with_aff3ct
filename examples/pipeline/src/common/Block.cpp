#include <typeinfo>
#include <algorithm>
#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <sstream>
#include <aff3ct.hpp>

#include "Block.hpp"
#include "Buffered_Socket.hpp"

Block
::Block(aff3ct::module::Task* task, int buffer_size, int n_threads)
: name(task->get_name()),
  n_threads(n_threads),
  tasks(),
  buffer_size(buffer_size),
  threads(n_threads),
  buffered_sockets_in(),
  buffered_sockets_out()
{
	task->set_autoalloc(false);
	task->set_autoexec (false);
	task->set_fast     (false);

	for (int i = 0; i < n_threads; i++)
		tasks.push_back(std::shared_ptr<aff3ct::module::Task>(task->clone()));

	for (size_t s_idx = 0; s_idx < task->sockets.size(); s_idx++)
	{
		std::vector<std::shared_ptr<aff3ct::module::Socket>> s_vec;
		for (int i = 0 ; i < n_threads; i++)
			s_vec.push_back(tasks[i]->sockets[s_idx]);

		std::shared_ptr<aff3ct::module::Socket> s = task->sockets[s_idx];
		const auto sdatatype = s->get_datatype_string();
		const auto sname = s->get_name();
		const auto stype = task->get_socket_type(*s);

		std::function<void(NT_Buffered_Socket*)> add_socket;
		if (stype == aff3ct::module::socket_t::SIN)
			add_socket = [this, sname](NT_Buffered_Socket* socket) {
				this->buffered_sockets_in[sname] = std::unique_ptr<NT_Buffered_Socket>(socket);
			};
		else
			add_socket = [this, sname](NT_Buffered_Socket* socket) {
				this->buffered_sockets_out[sname] = std::unique_ptr<NT_Buffered_Socket>(socket);
			};

		     if (sdatatype == "int8"   ) add_socket(new Buffered_Socket<int8_t >(s_vec, stype, buffer_size));
		else if (sdatatype == "int16"  ) add_socket(new Buffered_Socket<int16_t>(s_vec, stype, buffer_size));
		else if (sdatatype == "int32"  ) add_socket(new Buffered_Socket<int32_t>(s_vec, stype, buffer_size));
		else if (sdatatype == "int64"  ) add_socket(new Buffered_Socket<int64_t>(s_vec, stype, buffer_size));
		else if (sdatatype == "float32") add_socket(new Buffered_Socket<float  >(s_vec, stype, buffer_size));
		else if (sdatatype == "float64") add_socket(new Buffered_Socket<double >(s_vec, stype, buffer_size));
	}
}

template <typename T>
int Block
::bind_by_type(const std::string &start_sck_name, Block &dest_block, const std::string &dest_sck_name)
{
	auto socket = this->get_buffered_socket_in<T>(start_sck_name);
	if (socket != nullptr)
		return socket->bind(dest_block.get_buffered_socket_out<T>(dest_sck_name));
	else
		return -1;
}

int Block
::bind(const std::string &start_sck_name, Block &dest_block, const std::string &dest_sck_name)
{
	int rval = -1;
	auto sin_datatype = this->buffered_sockets_in[start_sck_name]->get_datatype();
	     if (sin_datatype == typeid(int8_t )) rval = bind_by_type<int8_t >(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(int16_t)) rval = bind_by_type<int16_t>(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(int32_t)) rval = bind_by_type<int32_t>(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(int64_t)) rval = bind_by_type<int64_t>(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(float  )) rval = bind_by_type<float  >(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(double )) rval = bind_by_type<double >(start_sck_name, dest_block, dest_sck_name);

	if (rval == -1)
	{
		std::stringstream message;
		message << "No 'socket' named '" << start_sck_name << "' for 'task': '" << this->name << "'.";
		throw aff3ct::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}
	else
		return rval;
}

void Block
::run(bool const * is_done)
{
	for (int i = 0; i < this->n_threads; i++)
		this->threads[i] = std::thread{&Block::execute_task,this,i,is_done};
};

void Block
::join()
{
	for (auto &th : this->threads)
		th.join();
};

void Block
::execute_task(const int task_id, const bool *is_done)
{
	while(!(*is_done))
	{
		for (auto const& it : this->buffered_sockets_in)
			while (!(*is_done) && it.second->pop(task_id)){};

		if (*is_done)
			break;

		this->tasks[task_id]->exec();

		for (auto const& it : this->buffered_sockets_out)
			while (!(*is_done) && it.second->push(task_id)){};
	}

	for (auto const& it : this->buffered_sockets_out) { it.second->stop(); }
	for (auto const& it : this->buffered_sockets_in ) { it.second->stop(); }
}

void Block
::reset()
{
	for (auto const& it : this->buffered_sockets_in ) { it.second->reset(); }
	for (auto const& it : this->buffered_sockets_out) { it.second->reset(); }
}

template <typename T>
Buffered_Socket<T>* Block
::get_buffered_socket_in(std::string name)
{
	if (this->buffered_sockets_in.count(name) > 0)
		if (aff3ct::module::type_to_string[this->buffered_sockets_in[name]->get_socket()->get_datatype()] ==
		    aff3ct::module::type_to_string[typeid(T)])
			return static_cast<Buffered_Socket<T>*>(this->buffered_sockets_in[name].get());
	return nullptr;
};

template <typename T>
Buffered_Socket<T>* Block
::get_buffered_socket_out(std::string name)
{
	if (this->buffered_sockets_out.count(name) > 0)
		if (aff3ct::module::type_to_string[this->buffered_sockets_out[name]->get_socket()->get_datatype()] ==
		    aff3ct::module::type_to_string[typeid(T)])
			return static_cast<Buffered_Socket<T>*>(this->buffered_sockets_out[name].get());
	return nullptr;

};