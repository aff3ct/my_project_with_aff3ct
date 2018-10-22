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
buffered_sockets_int8_out(),
buffered_sockets_int8_in(),
buffered_sockets_int16_out(),
buffered_sockets_int16_in(),
buffered_sockets_int32_out(),
buffered_sockets_int32_in(),
buffered_sockets_int64_out(),
buffered_sockets_int64_in(),
buffered_sockets_float_out(),
buffered_sockets_float_in(),
buffered_sockets_double_out(),
buffered_sockets_double_in(),
th()
{
	task->set_autoalloc(false);
	task->set_autoexec (false);
	task->set_fast     (false);
	
	for (auto &s : task->sockets)
	{
		std::cout << s->get_datatype_string() << std::endl;
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
	/*for (auto const& it : this->buffered_sockets_out)
	{
		delete it.second;
	}*/
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
		return 1;
	}	
}

void Block
::execute_task()
{
	static std::mutex print_mutex;
	
	while(1)
	{
		for (auto const& it : this->buffered_sockets_int8_in)  { while(it.second->pop()){}; }
		for (auto const& it : this->buffered_sockets_int16_in) { while(it.second->pop()){}; }
		for (auto const& it : this->buffered_sockets_int32_in) { while(it.second->pop()){}; }
		for (auto const& it : this->buffered_sockets_int64_in) { while(it.second->pop()){}; }
		for (auto const& it : this->buffered_sockets_float_in) { while(it.second->pop()){}; }
		for (auto const& it : this->buffered_sockets_double_in){ while(it.second->pop()){}; }

		this->task->exec();
		print_mutex.lock();
		std::cout << "-------------------------------------------------------" << std::endl;
		std::cout << "Je suis le Block : " << this->name << std::endl;
		for (auto const& it : this->buffered_sockets_int8_in)
		{
			std::cout << "J'ai consommé : "; 
			it.second->print_socket_data();
		}

		for (auto const& it : this->buffered_sockets_int16_in)
		{
			std::cout << "J'ai consommé : "; 
			it.second->print_socket_data();
		}

		for (auto const& it : this->buffered_sockets_int32_in)
		{
			std::cout << "J'ai consommé : "; 
			it.second->print_socket_data();
		}

		for (auto const& it : this->buffered_sockets_int64_in)
		{
			std::cout << "J'ai consommé : "; 
			it.second->print_socket_data();
		}

		for (auto const& it : this->buffered_sockets_float_in)
		{
			std::cout << "J'ai consommé : "; 
			it.second->print_socket_data();
		}

		for (auto const& it : this->buffered_sockets_double_in)
		{
			std::cout << "J'ai consommé : "; 
			it.second->print_socket_data();
		}
		for (auto const& it : this->buffered_sockets_int8_out)
		{
			std::cout << "J'ai produit  : ";
			it.second->print_socket_data();
		}

		for (auto const& it : this->buffered_sockets_int16_out)
		{
			std::cout << "J'ai produit  : ";
			it.second->print_socket_data();
		}

		for (auto const& it : this->buffered_sockets_int32_out)
		{
			std::cout << "J'ai produit  : ";
			it.second->print_socket_data();
		}

		for (auto const& it : this->buffered_sockets_int64_out)
		{
			std::cout << "J'ai produit  : ";
			it.second->print_socket_data();
		}		

		for (auto const& it : this->buffered_sockets_float_out)
		{
			std::cout << "J'ai produit  : ";
			it.second->print_socket_data();
		}		
		for (auto const& it : this->buffered_sockets_double_out)
		{
			std::cout << "J'ai produit  : ";
			it.second->print_socket_data();
		}		
		std::cout << "-------------------------------------------------------" << std::endl;
		print_mutex.unlock();

		for (auto const& it : this->buffered_sockets_int8_out)  { while(it.second->push()){}; }
		for (auto const& it : this->buffered_sockets_int16_out) { while(it.second->push()){}; }
		for (auto const& it : this->buffered_sockets_int32_out) { while(it.second->push()){}; }
		for (auto const& it : this->buffered_sockets_int64_out) { while(it.second->push()){}; }
		for (auto const& it : this->buffered_sockets_float_out) { while(it.second->push()){}; }
		for (auto const& it : this->buffered_sockets_double_out){ while(it.second->push()){}; }
	}
}

void Block
::run()
{
	this->th = std::thread{&Block::execute_task,this};
}

void Block
::join()
{
	this->th.join();
}