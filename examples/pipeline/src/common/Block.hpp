#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <map>
#include <thread>
#include <vector>
#include <memory>
#include <aff3ct.hpp>

#include "Buffered_Socket.hpp"

class Block
{
public:
	Block(aff3ct::module::Task* task, int buffered_socket_size, int n_threads = 1);
	virtual ~Block() = default;
	int bind(const std::string &start_sck_name, Block &dest_block, const std::string &dest_sck_name);
	void execute_task(const int task_id, const bool * is_done);
	void run(const bool * is_done);
	void join();
	void reset();

	template <typename T>
	Buffered_Socket<T>* get_buffered_socket_in(std::string name);
	template <typename T>
	Buffered_Socket<T>* get_buffered_socket_out(std::string name);


protected:
	template <typename T>
	int bind_by_type(const std::string &start_sck_name, Block &dest_block, const std::string &dest_sck_name);

	std::string name = "";
	int n_threads;
	std::vector<std::shared_ptr<aff3ct::module::Task>> tasks;
	unsigned int buffer_size;
	std::vector<std::thread> threads;
	std::map<std::string, std::unique_ptr<NT_Buffered_Socket>> buffered_sockets_in;
	std::map<std::string, std::unique_ptr<NT_Buffered_Socket>> buffered_sockets_out;
};

#endif /* BLOCK_HPP */