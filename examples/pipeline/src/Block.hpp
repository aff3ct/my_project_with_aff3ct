#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <map>
#include <thread>
#include <vector>
#include <aff3ct.hpp>
#include "Buffered_Socket.hpp"

class Block
{
public:
	std::string name = "";

	Block(aff3ct::module::Task* task, int buffered_socket_size, int n_threads = 1);
	virtual ~Block();
	int bind     (std::string start_sck_name, Block &dest_block, std::string dest_sck_name);
	void execute_task(const int task_id, const bool * isDone);
	void run         (const bool * isDone);
	void join();
	void reset();

	template <typename T>
	Buffered_Socket<T>* get_buffered_socket_in    (std::string name);
	template <typename T>
	Buffered_Socket<T>* get_buffered_socket_out   (std::string name);
	
	
protected:
	int n_threads;
	std::vector<aff3ct::module::Task *> tasks;
	unsigned int buffer_size;
	std::vector<std::thread> threads;
	std::map<std::string, NT_Buffered_Socket*> buffered_sockets_in  ;
	std::map<std::string, NT_Buffered_Socket*> buffered_sockets_out ;
};
#endif /* BLOCK_HPP */