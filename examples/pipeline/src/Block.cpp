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
buffered_sockets_out(),
buffered_sockets_in(),
th()
{
	task->set_autoalloc(false);
	task->set_autoexec (false);
	task->set_fast     (false);
	for (auto &s : task->sockets)
	{
		if (task->get_socket_type(*s) == aff3ct::module::Socket_type::IN)
			this->buffered_sockets_in.emplace(s->get_name(),new Buffered_Socket(s, task->get_socket_type(*s), buffer_size));
		else
			this->buffered_sockets_out.emplace(s->get_name(),new Buffered_Socket(s, task->get_socket_type(*s), buffer_size));
	}
}

Block
::~Block()
{
	for (auto const& it : this->buffered_sockets_out)
	{
		delete(it.second);
	}
}

Buffered_Socket* Block::
get_buffered_socket_out(std::string name)
{
	return this->buffered_sockets_out[name];
}

Buffered_Socket* Block::
get_buffered_socket_in(std::string name)
{
	return this->buffered_sockets_in[name];
}

int Block
::bind(std::string start_sck_name, Block &dest_block, std::string dest_sck_name)
{
	return this->buffered_sockets_in[start_sck_name]->bind(*(dest_block.get_buffered_socket_out(dest_sck_name)));
}

void Block
::execute_task()
{
	static std::mutex print_mutex;
	
	while(1)
	{
		for (auto const& it : this->buffered_sockets_in)
		{
			while(it.second->pop()){};		
		}
		this->task->exec();
		print_mutex.lock();
		std::cout << "-------------------------------------------------------" << std::endl;
		std::cout << "Je suis le Block : " << this->name << std::endl;
		for (auto const& it : this->buffered_sockets_in)
		{
			std::cout << "J'ai consommé " << it.second->get_name() <<" : [ ";
			for (auto const& d: *it.second->get_int32_socket_data())
			{
				std::cout << d << " ";
			}
			std::cout << "]\n";
		}
		
		for (auto const& it : this->buffered_sockets_out)
		{
			std::cout << "J'ai produit  " << it.second->get_name() <<" : [ ";
			for (auto const& d: *it.second->get_int32_socket_data())
			{
				std::cout << d << " ";
			}
			std::cout << "]\n";
		}
		std::cout << "-------------------------------------------------------" << std::endl;

		print_mutex.unlock();

		for (auto const& it : this->buffered_sockets_out)
		{
			while(it.second->push()){};
			
		}
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