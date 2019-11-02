#include <sstream>
#include <typeinfo>
#include <algorithm>
#include <functional>
#include <aff3ct.hpp>

#include "Buffered_Socket.hpp"
#include "Block.hpp"

Block
::Block(const aff3ct::module::Task &task, const size_t buffer_size, const size_t n_threads)
: n_threads(n_threads),
  buffer_size(buffer_size),
  threads(n_threads)
{
	if (n_threads == 0)
	{
		std::stringstream message;
		message << "'n_threads' has to be strictly positive ('n_threads' = " << n_threads << ").";
		throw aff3ct::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (buffer_size < n_threads)
	{
		std::stringstream message;
		message << "'buffer_size' has to be equal or greater than 'n_threads' ("
		        << "'buffer_size'" << " = " << buffer_size << ", "
		        << "'n_threads'"   << " = " << n_threads
		        << ").";
		throw aff3ct::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto task_cpy = std::unique_ptr<aff3ct::module::Task>(task.clone());
	task_cpy->set_autoalloc(false);
	task_cpy->set_autoexec (false);

	for (size_t tid = 0; tid < n_threads; tid++)
		tasks.push_back(std::shared_ptr<aff3ct::module::Task>(task_cpy->clone()));

	for (size_t sid = 0; sid < task.sockets.size(); sid++)
	{
		std::vector<std::shared_ptr<aff3ct::module::Socket>> s_vec;
		for (size_t tid = 0; tid < n_threads; tid++)
			s_vec.push_back(tasks[tid]->sockets[sid]);

		std::shared_ptr<aff3ct::module::Socket> s = task.sockets[sid];
		const auto sdatatype = s->get_datatype();
		const auto sname = s->get_name();
		const auto stype = task.get_socket_type(*s);

		std::function<void(NT_Buffered_Socket*)> add_socket;
		if (stype == aff3ct::module::socket_t::SIN)
			add_socket = [this, sname](NT_Buffered_Socket* socket) {
				this->buffered_sockets_in[sname] = std::unique_ptr<NT_Buffered_Socket>(socket);
			};
		else
			add_socket = [this, sname](NT_Buffered_Socket* socket) {
				this->buffered_sockets_out[sname] = std::unique_ptr<NT_Buffered_Socket>(socket);
			};

		     if (sdatatype == typeid(int8_t )) add_socket(new Buffered_Socket<int8_t >(s_vec, stype, buffer_size));
		else if (sdatatype == typeid(int16_t)) add_socket(new Buffered_Socket<int16_t>(s_vec, stype, buffer_size));
		else if (sdatatype == typeid(int32_t)) add_socket(new Buffered_Socket<int32_t>(s_vec, stype, buffer_size));
		else if (sdatatype == typeid(int64_t)) add_socket(new Buffered_Socket<int64_t>(s_vec, stype, buffer_size));
		else if (sdatatype == typeid(float  )) add_socket(new Buffered_Socket<float  >(s_vec, stype, buffer_size));
		else if (sdatatype == typeid(float  )) add_socket(new Buffered_Socket<double >(s_vec, stype, buffer_size));
	}
}

std::vector<const aff3ct::module::Task*> Block
::get_tasks() const
{
	std::vector<const aff3ct::module::Task*> tasks_bis(this->tasks.size(), nullptr);
	for (size_t t = 0; t < this->tasks.size(); t++)
		tasks_bis[t] = const_cast<const aff3ct::module::Task*>(this->tasks[t].get());
	return tasks_bis;
}

template <typename T>
int Block
::_bind(const std::string &start_sck_name, Block &dest_block, const std::string &dest_sck_name)
{
	auto &sin = this->get_buffered_socket_in<T>(start_sck_name);
	auto &sout = dest_block.get_buffered_socket_out<T>(dest_sck_name);
	return sin.bind(sout);
}

int Block
::bind(const std::string &start_sck_name, Block &dest_block, const std::string &dest_sck_name)
{
	auto sin_datatype = this->get_buffered_socket(start_sck_name, this->buffered_sockets_in).get_s().get_datatype();
	     if (sin_datatype == typeid(int8_t )) return _bind<int8_t >(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(int16_t)) return _bind<int16_t>(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(int32_t)) return _bind<int32_t>(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(int64_t)) return _bind<int64_t>(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(float  )) return _bind<float  >(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(double )) return _bind<double >(start_sck_name, dest_block, dest_sck_name);

	std::stringstream message;
	message << "This should not happen :-(.";
	throw aff3ct::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
}

void Block
::run(const bool &is_done)
{
	for (size_t tid = 0; tid < this->n_threads; tid++)
		this->threads[tid] = std::thread(&Block::execute_task, this, tid, std::ref(is_done));
};

void Block
::join()
{
	for (auto &th : this->threads)
		th.join();
};

void Block
::execute_task(const size_t tid, const bool &is_done)
{
	while (!is_done)
	{
		for (auto const& sin : this->buffered_sockets_in)
			while (!is_done && sin.second->pop(tid));

		if (is_done)
			break;

		this->tasks[tid]->exec();

		for (auto const& sout : this->buffered_sockets_out)
			while (!is_done && sout.second->push(tid));
	}

	for (auto const& sout : this->buffered_sockets_out) { sout.second->stop(); }
	for (auto const& sin  : this->buffered_sockets_in ) { sin .second->stop(); }
}

void Block
::reset()
{
	for (auto const& sin  : this->buffered_sockets_in ) { sin .second->reset(); }
	for (auto const& sout : this->buffered_sockets_out) { sout.second->reset(); }
}

NT_Buffered_Socket& Block
::get_buffered_socket(const std::string &name,
                      std::map<std::string, std::unique_ptr<NT_Buffered_Socket>> &buffered_sockets)
{
	if (buffered_sockets.count(name) == 0)
	{
		std::stringstream message;
		message << "'buffered_sockets_in.count(name)' has to be strictly positive ("
		        << "'name'"                            << " = " << name << ", "
		        << "'buffered_sockets_in.count(name)'" << " = " << buffered_sockets_in.count(name)
		        << ").";
		throw aff3ct::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	return *buffered_sockets[name].get();
}

template <typename T>
Buffered_Socket<T>& Block
::get_buffered_socket(const std::string &name,
                      std::map<std::string, std::unique_ptr<NT_Buffered_Socket>> &buffered_sockets)
{
	auto &socket = this->get_buffered_socket(name, buffered_sockets);
	if (aff3ct::module::type_to_string[socket.get_s().get_datatype()] != aff3ct::module::type_to_string[typeid(T)])
	{
		std::stringstream message;
		message << "'socket.get_s().get_datatype()' has to be equal to 'typeid(T)' ("
		        << "'name'" << " = " << name << ", "
		        << "'socket.get_s().get_datatype()'" << " = "
		        << aff3ct::module::type_to_string[socket.get_s().get_datatype()] << ", "
		        << "'typeid(T)'" << " = " << aff3ct::module::type_to_string[typeid(T)]
		        << ").";
		throw aff3ct::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	return static_cast<Buffered_Socket<T>&>(socket);
}

template <typename T>
Buffered_Socket<T>& Block
::get_buffered_socket_in(const std::string &name)
{
	return this->get_buffered_socket<T>(name, this->buffered_sockets_in);
}

template <typename T>
Buffered_Socket<T>& Block
::get_buffered_socket_out(const std::string &name)
{
	return this->get_buffered_socket<T>(name, this->buffered_sockets_out);
}