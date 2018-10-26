#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <map>
#include <thread>
#include <aff3ct.hpp>
#include "Buffered_Socket.hpp"

class Block
{
public:
	std::string name = "";

	Block(aff3ct::module::Task* task, int buffered_socket_size);
	virtual ~Block();

	int bind     (std::string start_sck_name, Block &dest_block, std::string dest_sck_name);
	int bind_cpy (std::string start_sck_name, Block &dest_block, std::string dest_sck_name);
	void execute_task(bool const * isDone);
	void run(bool const * isDone) {this->th = std::thread{&Block::execute_task,this,isDone};};
	void join() {this->th.join();};
	void reset();
	
	template <typename T>
	Buffered_Socket<T>* get_buffered_socket_in   (std::string name);
	template <typename T>
	Buffered_Socket<T>* get_buffered_socket_out   (std::string name);
	
	
protected:
	aff3ct::module::Task* task;
	unsigned int buffer_size;
	std::thread th;

	std::map<std::string, NT_Buffered_Socket*> buffered_sockets_in  ;
	std::map<std::string, NT_Buffered_Socket*> buffered_sockets_out  ;
};
#endif /* BLOCK_HPP */