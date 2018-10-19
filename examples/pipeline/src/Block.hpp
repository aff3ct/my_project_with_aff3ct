#ifndef BLOCK_HPP_
#define BLOCK_HPP_

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

	int bind (std::string start_sck_name, Block &dest_block, std::string dest_sck_name);
	void execute_task();
	void run();
	void join();
	Buffered_Socket* get_buffered_socket_out(std::string name);
	Buffered_Socket* get_buffered_socket_in(std::string name);

protected:
	aff3ct::module::Task* task;
	unsigned int buffer_size;
	std::thread th;

	std::map<std::string, Buffered_Socket*> buffered_sockets_in;
	std::map<std::string, Buffered_Socket*> buffered_sockets_out;
};

#endif /* BLOCK_HPP_ */
