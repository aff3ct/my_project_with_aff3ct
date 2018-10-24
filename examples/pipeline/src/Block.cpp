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
buffered_sockets_int8_in(),
buffered_sockets_int16_in(),
buffered_sockets_int32_in(),
buffered_sockets_int64_in(),
buffered_sockets_float_in(),
buffered_sockets_double_in(),
buffered_sockets_int8_out(),
buffered_sockets_int16_out(),
buffered_sockets_int32_out(),
buffered_sockets_int64_out(),
buffered_sockets_float_out(),
buffered_sockets_double_out()
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
				this->buffered_sockets_int8_in.emplace(s->get_name(),
													new Buffered_Socket<int8_t>(s, task->get_socket_type(*s), 
													buffer_size));
			}
			else if(s->get_datatype_string() == "int16")
			{
				this->buffered_sockets_int16_in.emplace(s->get_name(),
													new Buffered_Socket<int16_t>(s, task->get_socket_type(*s), 
													buffer_size));
			}
			else if(s->get_datatype_string() == "int32")
			{
				this->buffered_sockets_int32_in.emplace(s->get_name(),
													new Buffered_Socket<int32_t>(s, task->get_socket_type(*s), 
													buffer_size));
			}
			else if(s->get_datatype_string() == "int64")
			{
				this->buffered_sockets_int64_in.emplace(s->get_name(),
													new Buffered_Socket<int64_t>(s, task->get_socket_type(*s), 
													buffer_size));
			}
			else if(s->get_datatype_string() == "float32")
			{
				this->buffered_sockets_float_in.emplace(s->get_name(),
													new Buffered_Socket<float>(s, task->get_socket_type(*s), 
													buffer_size));
			}
			else if(s->get_datatype_string() == "float64")
			{
				this->buffered_sockets_double_in.emplace(s->get_name(),
													new Buffered_Socket<double>(s, task->get_socket_type(*s), 
													buffer_size));
			}
		}
		else
		{
			if(s->get_datatype_string() == "int8")
			{
				this->buffered_sockets_int8_out.emplace(s->get_name(),
													new Buffered_Socket<int8_t>(s, task->get_socket_type(*s), 
													buffer_size));
			}
			else if(s->get_datatype_string() == "int16")
			{
				this->buffered_sockets_int16_out.emplace(s->get_name(),
													new Buffered_Socket<int16_t>(s, task->get_socket_type(*s), 
													buffer_size));
			}
			else if(s->get_datatype_string() == "int32")
			{
				this->buffered_sockets_int32_out.emplace(s->get_name(),
													new Buffered_Socket<int32_t>(s, task->get_socket_type(*s), 
													buffer_size));
			}
			else if(s->get_datatype_string() == "int64")
			{
				this->buffered_sockets_int64_out.emplace(s->get_name(),
													new Buffered_Socket<int64_t>(s, task->get_socket_type(*s), 
													buffer_size));
			}
			else if(s->get_datatype_string() == "float32")
			{
				this->buffered_sockets_float_out.emplace(s->get_name(),
													new Buffered_Socket<float>(s, task->get_socket_type(*s), 
													buffer_size));
			}
			else if(s->get_datatype_string() == "float64")
			{
				this->buffered_sockets_double_out.emplace(s->get_name(),
													new Buffered_Socket<double>(s, task->get_socket_type(*s), 
													buffer_size));
			}
		}
	}
}

Block
::~Block()
{
	for (auto &it : this->buffered_sockets_int8_in)
		delete it.second;
	for (auto &it : this->buffered_sockets_int16_in)
		delete it.second;
	for (auto &it : this->buffered_sockets_int32_in)
		delete it.second;
	for (auto &it : this->buffered_sockets_int64_in)
		delete it.second;
	for (auto &it : this->buffered_sockets_float_in)
		delete it.second;
	for (auto &it : this->buffered_sockets_double_in)
		delete it.second;

	for (auto &it : this->buffered_sockets_int8_out)
		delete it.second;
	for (auto &it : this->buffered_sockets_int16_out)
		delete it.second;
	for (auto &it : this->buffered_sockets_int32_out)
		delete it.second;
	for (auto &it : this->buffered_sockets_int64_out)
		delete it.second;
	for (auto &it : this->buffered_sockets_float_out)
		delete it.second;
	for (auto &it : this->buffered_sockets_double_out)
		delete it.second;
}

int Block
::bind(std::string start_sck_name, Block &dest_block, std::string dest_sck_name)
{
	if 	(this->buffered_sockets_int8_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_int8_in[start_sck_name]->bind(dest_block.get_buffered_socket_int8_out(dest_sck_name));
	}
	else if (this->buffered_sockets_int16_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_int16_in[start_sck_name]->bind(dest_block.get_buffered_socket_int16_out(dest_sck_name));
	}
	else if (this->buffered_sockets_int32_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_int32_in[start_sck_name]->bind(dest_block.get_buffered_socket_int32_out(dest_sck_name));
	}
	else if (this->buffered_sockets_int64_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_int64_in[start_sck_name]->bind(dest_block.get_buffered_socket_int64_out(dest_sck_name));
	}
	else if (this->buffered_sockets_float_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_float_in[start_sck_name]->bind(dest_block.get_buffered_socket_float_out(dest_sck_name));
	}
	else if (this->buffered_sockets_double_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_double_in[start_sck_name]->bind(dest_block.get_buffered_socket_double_out(dest_sck_name));
	}
	else
	{
		std::cout << "No socket named '" << start_sck_name << "' for Task : '" << this->name << "'." << std::endl;
		return 1;
	}	
}


int Block
::bind_cpy(std::string start_sck_name, Block &dest_block, std::string dest_sck_name)
{
	if 	(this->buffered_sockets_int8_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_int8_in[start_sck_name]->bind_cpy(dest_block.get_buffered_socket_int8_out(dest_sck_name));
	}
	else if (this->buffered_sockets_int16_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_int16_in[start_sck_name]->bind_cpy(dest_block.get_buffered_socket_int16_out(dest_sck_name));
	}
	else if (this->buffered_sockets_int32_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_int32_in[start_sck_name]->bind_cpy(dest_block.get_buffered_socket_int32_out(dest_sck_name));
	}
	else if (this->buffered_sockets_int64_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_int64_in[start_sck_name]->bind_cpy(dest_block.get_buffered_socket_int64_out(dest_sck_name));
	}
	else if (this->buffered_sockets_float_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_float_in[start_sck_name]->bind_cpy(dest_block.get_buffered_socket_float_out(dest_sck_name));
	}
	else if (this->buffered_sockets_double_in.count(start_sck_name) > 0)
	{
		return this->buffered_sockets_double_in[start_sck_name]->bind_cpy(dest_block.get_buffered_socket_double_out(dest_sck_name));
	}
	else
	{
		std::cout << "No socket named '" << start_sck_name << "' for Task : '" << this->name << "'." << std::endl;
		return 1;
	}	
}

void Block
::execute_task(bool const * isDone)
{
	while(!(*isDone))
	{
		for (auto const& it : this->buffered_sockets_int8_in  ) { while(!(*isDone) && it.second->pop()){}; }
		for (auto const& it : this->buffered_sockets_int16_in ) { while(!(*isDone) && it.second->pop()){}; }
		for (auto const& it : this->buffered_sockets_int32_in ) { while(!(*isDone) && it.second->pop()){}; }
		for (auto const& it : this->buffered_sockets_int64_in ) { while(!(*isDone) && it.second->pop()){}; }
		for (auto const& it : this->buffered_sockets_float_in ) { while(!(*isDone) && it.second->pop()){}; }
		for (auto const& it : this->buffered_sockets_double_in) { while(!(*isDone) && it.second->pop()){}; }
		if (!(*isDone))
			this->task->exec();	
		
		for (auto const& it : this->buffered_sockets_int8_out  ) { while(!(*isDone) && it.second->push()){}; }
		for (auto const& it : this->buffered_sockets_int16_out ) { while(!(*isDone) && it.second->push()){}; }
		for (auto const& it : this->buffered_sockets_int32_out ) { while(!(*isDone) && it.second->push()){}; }
		for (auto const& it : this->buffered_sockets_int64_out ) { while(!(*isDone) && it.second->push()){}; }
		for (auto const& it : this->buffered_sockets_float_out ) { while(!(*isDone) && it.second->push()){}; }
		for (auto const& it : this->buffered_sockets_double_out) { while(!(*isDone) && it.second->push()){}; }			
	}
	this->reset();
}

void Block
::reset()
{

		for (auto const& it : this->buffered_sockets_int8_in  )  { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_int16_in )  { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_int32_in )  { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_int64_in )  { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_float_in )  { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_double_in)  { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_int8_out  ) { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_int16_out ) { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_int32_out ) { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_int64_out ) { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_float_out ) { it.second->reset(); }
		for (auto const& it : this->buffered_sockets_double_out) { it.second->reset(); }
}

Buffered_Socket<int8_t>* Block
::get_buffered_socket_int8_in(std::string name)
{
	if (this->buffered_sockets_int8_in.count(name) > 0)
		return this->buffered_sockets_int8_in[name]; 
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}
};

Buffered_Socket<int8_t>* Block
::get_buffered_socket_int8_out(std::string name)
{
	if (this->buffered_sockets_int8_out.count(name) > 0)
		return this->buffered_sockets_int8_out[name]; 
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}	
};

Buffered_Socket<int16_t>* Block
::get_buffered_socket_int16_in(std::string name)
{
	if (this->buffered_sockets_int16_in.count(name) > 0)
	{
		return this->buffered_sockets_int16_in[name]; 
	}
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}
};

Buffered_Socket<int16_t>* Block
::get_buffered_socket_int16_out(std::string name)
{
	if (this->buffered_sockets_int16_out.count(name) > 0)
	{
		return this->buffered_sockets_int16_out[name]; 
	}
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}	
};

Buffered_Socket<int32_t>* Block
::get_buffered_socket_int32_in(std::string name)
{
	if (this->buffered_sockets_int32_in.count(name) > 0)
	{
		return this->buffered_sockets_int32_in[name]; 
	}
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}
};

Buffered_Socket<int32_t>* Block
::get_buffered_socket_int32_out(std::string name)
{
	if (this->buffered_sockets_int32_out.count(name) > 0)
	{
		return this->buffered_sockets_int32_out[name]; 
	}
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}	
};

Buffered_Socket<int64_t>* Block
::get_buffered_socket_int64_in(std::string name)
{
	if (this->buffered_sockets_int64_in.count(name) > 0)
	{
		return this->buffered_sockets_int64_in[name]; 
	}
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}
};

Buffered_Socket<int64_t>* Block
::get_buffered_socket_int64_out(std::string name)
{
	if (this->buffered_sockets_int64_out.count(name) > 0)
	{
		return this->buffered_sockets_int64_out[name]; 
	}
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}	
};

Buffered_Socket<float>* Block
::get_buffered_socket_float_in(std::string name)
{
	if (this->buffered_sockets_float_in.count(name) > 0)
	{
		return this->buffered_sockets_float_in[name]; 
	}
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}
};

Buffered_Socket<float>* Block
::get_buffered_socket_float_out(std::string name)
{
	if (this->buffered_sockets_float_out.count(name) > 0)
	{
		return this->buffered_sockets_float_out[name]; 
	}
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}	
};

Buffered_Socket<double>* Block
::get_buffered_socket_double_in(std::string name)
{
	if (this->buffered_sockets_double_in.count(name) > 0)
	{
		return this->buffered_sockets_double_in[name]; 
	}
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}
};

Buffered_Socket<double>* Block
::get_buffered_socket_double_out(std::string name)
{
	if (this->buffered_sockets_double_out.count(name) > 0)
	{
		return this->buffered_sockets_double_out[name]; 
	}
	else
	{
		std::cout << "No socket named '" << name << "' for Task : '" << this->name << "'." << std::endl;
		return nullptr;
	}	
};	