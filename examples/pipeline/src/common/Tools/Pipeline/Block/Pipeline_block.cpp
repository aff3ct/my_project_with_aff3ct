#include <sstream>
#include <typeinfo>
#include <algorithm>
#include <functional>

#include <Module/Socket.hpp>
#include <Tools/Exception/exception.hpp>

#include "Pipeline_block.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

Pipeline_block
::Pipeline_block(const module::Task &task, const size_t buffer_size, const size_t n_threads)
: n_threads(n_threads),
  buffer_size(buffer_size),
  threads(n_threads)
{
	if (n_threads == 0)
	{
		std::stringstream message;
		message << "'n_threads' has to be strictly positive ('n_threads' = " << n_threads << ").";
		throw invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (buffer_size == 0)
	{
		std::stringstream message;
		message << "'buffer_size' has to be strictly positive ('buffer_size' = " << buffer_size << ").";
		throw invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto task_cpy = std::unique_ptr<module::Task>(task.clone());
	task_cpy->set_autoalloc(false);
	task_cpy->set_autoexec (false);

	for (size_t tid = 0; tid < n_threads; tid++)
		tasks.push_back(std::shared_ptr<module::Task>(task_cpy->clone()));

	for (size_t sid = 0; sid < task.sockets.size(); sid++)
	{
		std::vector<std::shared_ptr<module::Socket>> s_vec;
		for (size_t tid = 0; tid < n_threads; tid++)
			s_vec.push_back(tasks[tid]->sockets[sid]);

		std::shared_ptr<module::Socket> s = task.sockets[sid];
		const auto sdatatype = s->get_datatype();
		const auto sname = s->get_name();
		const auto stype = task.get_socket_type(*s);

		std::function<void(Pipeline_socket*)> add_socket;
		if (stype == module::socket_t::SIN)
			add_socket = [this, sname](Pipeline_socket* socket) {
				this->buffered_sockets_in[sname] = std::unique_ptr<Pipeline_socket>(socket);
			};
		else
			add_socket = [this, sname](Pipeline_socket* socket) {
				this->buffered_sockets_out[sname] = std::unique_ptr<Pipeline_socket>(socket);
			};

		     if (sdatatype == typeid(int8_t )) add_socket(new Pipeline_socket_buffered<int8_t >(s_vec, stype, buffer_size));
		else if (sdatatype == typeid(int16_t)) add_socket(new Pipeline_socket_buffered<int16_t>(s_vec, stype, buffer_size));
		else if (sdatatype == typeid(int32_t)) add_socket(new Pipeline_socket_buffered<int32_t>(s_vec, stype, buffer_size));
		else if (sdatatype == typeid(int64_t)) add_socket(new Pipeline_socket_buffered<int64_t>(s_vec, stype, buffer_size));
		else if (sdatatype == typeid(float  )) add_socket(new Pipeline_socket_buffered<float  >(s_vec, stype, buffer_size));
		else if (sdatatype == typeid(float  )) add_socket(new Pipeline_socket_buffered<double >(s_vec, stype, buffer_size));
		else
		{
			std::stringstream message;
			message << "This should not happen :-(.";
			throw runtime_error(__FILE__, __LINE__, __func__, message.str());
		}
	}
}

std::vector<const module::Task*> Pipeline_block
::get_tasks() const
{
	std::vector<const module::Task*> tasks_bis(this->tasks.size(), nullptr);
	for (size_t t = 0; t < this->tasks.size(); t++)
		tasks_bis[t] = const_cast<const module::Task*>(this->tasks[t].get());
	return tasks_bis;
}

template <typename T>
void Pipeline_block
::_bind(const std::string &start_sck_name, Pipeline_block &dest_block, const std::string &dest_sck_name)
{
	auto &sin = this->get_buffered_socket_in<T>(start_sck_name);
	auto &sout = dest_block.get_buffered_socket_out<T>(dest_sck_name);
	sin.bind(sout);
}

void Pipeline_block
::bind(const std::string &start_sck_name, Pipeline_block &dest_block, const std::string &dest_sck_name)
{
	auto sin_datatype = this->get_buffered_socket(start_sck_name, this->buffered_sockets_in).get_s().get_datatype();
	     if (sin_datatype == typeid(int8_t )) _bind<int8_t >(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(int16_t)) _bind<int16_t>(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(int32_t)) _bind<int32_t>(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(int64_t)) _bind<int64_t>(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(float  )) _bind<float  >(start_sck_name, dest_block, dest_sck_name);
	else if (sin_datatype == typeid(double )) _bind<double >(start_sck_name, dest_block, dest_sck_name);
	else
	{
		std::stringstream message;
		message << "This should not happen :-(.";
		throw runtime_error(__FILE__, __LINE__, __func__, message.str());
	}
}

void Pipeline_block
::run(const bool &is_done)
{
	for (size_t tid = 0; tid < this->n_threads; tid++)
		this->threads[tid] = std::thread(&Pipeline_block::execute_task, this, tid, std::ref(is_done));
};

void Pipeline_block
::join()
{
	for (auto &th : this->threads)
		th.join();
};

void Pipeline_block
::execute_task(const size_t tid, const bool &is_done)
{
	while (!is_done)
	{
		for (const auto &sin : this->buffered_sockets_in)
			while (!is_done && !sin.second->try_pop(tid));

		if (is_done)
			break;

		this->tasks[tid]->exec();

		for (const auto &sout : this->buffered_sockets_out)
			while (!is_done && !sout.second->try_push(tid));
	}
}

void Pipeline_block
::reset()
{
	for (auto const& sin  : this->buffered_sockets_in ) { sin .second->reset(); }
	for (auto const& sout : this->buffered_sockets_out) { sout.second->reset(); }
}

Pipeline_socket& Pipeline_block
::get_buffered_socket(const std::string &name,
                      std::map<std::string, std::unique_ptr<Pipeline_socket>> &buffered_sockets)
{
	if (buffered_sockets.count(name) == 0)
	{
		std::stringstream message;
		message << "'buffered_sockets_in.count(name)' has to be strictly positive ("
		        << "'name'"                            << " = " << name << ", "
		        << "'buffered_sockets_in.count(name)'" << " = " << buffered_sockets_in.count(name)
		        << ").";
		throw runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	return *buffered_sockets[name].get();
}

template <typename T>
Pipeline_socket_buffered<T>& Pipeline_block
::get_buffered_socket(const std::string &name,
                      std::map<std::string, std::unique_ptr<Pipeline_socket>> &buffered_sockets)
{
	auto &socket = this->get_buffered_socket(name, buffered_sockets);
	if (module::type_to_string[socket.get_s().get_datatype()] != module::type_to_string[typeid(T)])
	{
		std::stringstream message;
		message << "'socket.get_s().get_datatype()' has to be equal to 'typeid(T)' ("
		        << "'name'" << " = " << name << ", "
		        << "'socket.get_s().get_datatype()'" << " = "
		        << module::type_to_string[socket.get_s().get_datatype()] << ", "
		        << "'typeid(T)'" << " = " << module::type_to_string[typeid(T)]
		        << ").";
		throw runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	return static_cast<Pipeline_socket_buffered<T>&>(socket);
}

template <typename T>
Pipeline_socket_buffered<T>& Pipeline_block
::get_buffered_socket_in(const std::string &name)
{
	return this->get_buffered_socket<T>(name, this->buffered_sockets_in);
}

template <typename T>
Pipeline_socket_buffered<T>& Pipeline_block
::get_buffered_socket_out(const std::string &name)
{
	return this->get_buffered_socket<T>(name, this->buffered_sockets_out);
}