#include <typeinfo>
#include <algorithm>
#include <map>
#include <thread>
#include <mutex>

#include "Block.hpp"
#include "Buffered_Socket.hpp"

Block
::Block(aff3ct::module::Task* task, int buffer_size)
: name(task->get_name()),
task(task),
buffer_size(buffer_size),
th(),
buffered_sockets_in(),
buffered_sockets_out()
{
	task->set_autoalloc(false);
	task->set_autoexec (false);
	task->set_fast     (false);
	
	for (auto &s : task->sockets)
	{
		if (task->get_socket_type(*s) == aff3ct::module::Socket_type::IN)
		{
			if(s->get_datatype_string() == "int8")
			{
				this->buffered_sockets_in.emplace(s->get_name(),
				                                  new Buffered_Socket<int8_t>(s, task->get_socket_type(*s), 
				                                  buffer_size));
			}
			else if(s->get_datatype_string() == "int16")
			{
				this->buffered_sockets_in.emplace(s->get_name(),
				                                  new Buffered_Socket<int16_t>(s, task->get_socket_type(*s), 
				                                  buffer_size));
			}
			else if(s->get_datatype_string() == "int32")
			{
				this->buffered_sockets_in.emplace(s->get_name(),
				                                  new Buffered_Socket<int32_t>(s, task->get_socket_type(*s), 
				                                  buffer_size));
			}
			else if(s->get_datatype_string() == "int64")
			{
				this->buffered_sockets_in.emplace(s->get_name(),
				                                  new Buffered_Socket<int64_t>(s, task->get_socket_type(*s), 
				                                  buffer_size));
			}
			else if(s->get_datatype_string() == "float32")
			{
				this->buffered_sockets_in.emplace(s->get_name(),
				                                  new Buffered_Socket<float>(s, task->get_socket_type(*s), 
				                                  buffer_size));
			}
			else if(s->get_datatype_string() == "float64")
			{
				this->buffered_sockets_in.emplace(s->get_name(),
				                                  new Buffered_Socket<double>(s, task->get_socket_type(*s), 
				                                  buffer_size));
			}
		}
		else
		{
			if(s->get_datatype_string() == "int8")
			{
				this->buffered_sockets_out.emplace(s->get_name(),
				                                   new Buffered_Socket<int8_t>(s, task->get_socket_type(*s), 
				                                   buffer_size));
			}
			else if(s->get_datatype_string() == "int16")
			{
				this->buffered_sockets_out.emplace(s->get_name(),
				                                   new Buffered_Socket<int16_t>(s, task->get_socket_type(*s), 
				                                   buffer_size));
			}
			else if(s->get_datatype_string() == "int32")
			{
				this->buffered_sockets_out.emplace(s->get_name(),
				                                   new Buffered_Socket<int32_t>(s, task->get_socket_type(*s), 
				                                   buffer_size));
			}
			else if(s->get_datatype_string() == "int64")
			{
				this->buffered_sockets_out.emplace(s->get_name(),
				                                   new Buffered_Socket<int64_t>(s, task->get_socket_type(*s), 
				                                   buffer_size));
			}
			else if(s->get_datatype_string() == "float32")
			{
				this->buffered_sockets_out.emplace(s->get_name(),
				                                   new Buffered_Socket<float>(s, task->get_socket_type(*s), 
				                                   buffer_size));
			}
			else if(s->get_datatype_string() == "float64")
			{
				this->buffered_sockets_out.emplace(s->get_name(),
				                                   new Buffered_Socket<double>(s, task->get_socket_type(*s), 
				                                   buffer_size));
			}
		}
	}
}

Block
::~Block()
{
	for (auto &it : this->buffered_sockets_in)
		delete it.second;

	for (auto &it : this->buffered_sockets_out)
		delete it.second;
}

int Block
::bind(std::string start_sck_name, Block &dest_block, std::string dest_sck_name)
{
	Buffered_Socket<int8_t>* socket_int8 = this->get_buffered_socket_in<int8_t>(start_sck_name);
	if 	(socket_int8 != nullptr)
		return socket_int8->bind(dest_block.get_buffered_socket_out<int8_t>(dest_sck_name));

	Buffered_Socket<int16_t>* socket_int16 = this->get_buffered_socket_in<int16_t>(start_sck_name);
	if 	(socket_int16 != nullptr)
		return socket_int16->bind(dest_block.get_buffered_socket_out<int16_t>(dest_sck_name));

	Buffered_Socket<int32_t>* socket_int32 = this->get_buffered_socket_in<int32_t>(start_sck_name);
	if 	(socket_int32 != nullptr)
		return socket_int32->bind(dest_block.get_buffered_socket_out<int32_t>(dest_sck_name));

	Buffered_Socket<int64_t>* socket_int64 = this->get_buffered_socket_in<int64_t>(start_sck_name);
	if 	(socket_int64 != nullptr)
		return socket_int64->bind(dest_block.get_buffered_socket_out<int64_t>(dest_sck_name));

	Buffered_Socket<float>* socket_float = this->get_buffered_socket_in<float>(start_sck_name);
	if 	(socket_float != nullptr)
		return socket_float->bind(dest_block.get_buffered_socket_out<float>(dest_sck_name));

	Buffered_Socket<double>* socket_double = this->get_buffered_socket_in<double>(start_sck_name);
	if 	(socket_double != nullptr)
		return socket_double->bind(dest_block.get_buffered_socket_out<double>(dest_sck_name));
	
	std::cout << "No socket named '" << start_sck_name << "' for Task : '" << this->name << "'." << std::endl;
	return 1;	
}

int Block
::bind_cpy(std::string start_sck_name, Block &dest_block, std::string dest_sck_name)
{
	Buffered_Socket<int8_t>* socket_int8 = this->get_buffered_socket_in<int8_t>(start_sck_name);
	if 	(socket_int8 != nullptr)
		return socket_int8->bind_cpy(dest_block.get_buffered_socket_out<int8_t>(dest_sck_name));

	Buffered_Socket<int16_t>* socket_int16 = this->get_buffered_socket_in<int16_t>(start_sck_name);
	if 	(socket_int16 != nullptr)
		return socket_int16->bind_cpy(dest_block.get_buffered_socket_out<int16_t>(dest_sck_name));

	Buffered_Socket<int32_t>* socket_int32 = this->get_buffered_socket_in<int32_t>(start_sck_name);
	if 	(socket_int32 != nullptr)
		return socket_int32->bind_cpy(dest_block.get_buffered_socket_out<int32_t>(dest_sck_name));

	Buffered_Socket<int64_t>* socket_int64 = this->get_buffered_socket_in<int64_t>(start_sck_name);
	if 	(socket_int64 != nullptr)
		return socket_int64->bind_cpy(dest_block.get_buffered_socket_out<int64_t>(dest_sck_name));

	Buffered_Socket<float>* socket_float = this->get_buffered_socket_in<float>(start_sck_name);
	if 	(socket_float != nullptr)
		return socket_float->bind_cpy(dest_block.get_buffered_socket_out<float>(dest_sck_name));

	Buffered_Socket<double>* socket_double = this->get_buffered_socket_in<double>(start_sck_name);
	if 	(socket_double != nullptr)
		return socket_double->bind_cpy(dest_block.get_buffered_socket_out<double>(dest_sck_name));
	
	std::cout << "No socket named '" << start_sck_name << "' for Task : '" << this->name << "'." << std::endl;
	return 1;	
}

void Block
::execute_task(bool const * isDone)
{
	static std::mutex print_mx;
	while(!(*isDone))
	{
		
		for (auto const& it : this->buffered_sockets_in  ) {it.second->wait_pop();}
		if (*isDone)
			break;
						
		this->task->exec();
		for (auto const& it : this->buffered_sockets_out  ) {it.second->wait_push();}
	}	

	for (auto const& it : this->buffered_sockets_out ) { it.second->stop();}
	for (auto const& it : this->buffered_sockets_in ) { it.second->stop();}
}

void Block
::reset()
{
		for (auto const& it : this->buffered_sockets_in  )  { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_out )  { it.second->reset(); }
}

template <typename T>
Buffered_Socket<T>* Block
::get_buffered_socket_in(std::string name)
{
	if (this->buffered_sockets_in.count(name) > 0)
	{	if (aff3ct::module::type_to_string[this->buffered_sockets_in[name]->get_socket()->get_datatype()] == aff3ct::module::type_to_string[typeid(T)])
			return static_cast<Buffered_Socket<T>*>(this->buffered_sockets_in[name]); 
	}
	return nullptr;
};

template <typename T>
Buffered_Socket<T>* Block
::get_buffered_socket_out(std::string name)
{
	if (this->buffered_sockets_out.count(name) > 0)
	{
		if (aff3ct::module::type_to_string[this->buffered_sockets_out[name]->get_socket()->get_datatype()] == aff3ct::module::type_to_string[typeid(T)])
			return static_cast<Buffered_Socket<T>*>(this->buffered_sockets_out[name]); 
	}
	return nullptr;

};