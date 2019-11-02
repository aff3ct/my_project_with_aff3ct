#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <map>
#include <thread>
#include <vector>
#include <memory>
#include <string>
#include <aff3ct.hpp>

#include "Buffered_Socket.hpp"

class Block
{
protected:
	const size_t n_threads;
	const size_t buffer_size;
	std::vector<std::shared_ptr<aff3ct::module::Task>> tasks;
	std::vector<std::thread> threads;
	std::map<std::string, std::unique_ptr<NT_Buffered_Socket>> buffered_sockets_in;
	std::map<std::string, std::unique_ptr<NT_Buffered_Socket>> buffered_sockets_out;

public:
	Block(const aff3ct::module::Task &task, const size_t buffer_size, const size_t n_threads = 1);
	virtual ~Block() = default;
	int bind(const std::string &start_sck_name, Block &dest_block, const std::string &dest_sck_name);
	void execute_task(const size_t tid, const bool &is_done);
	void run(const bool &is_done);
	void join();
	void reset();
	std::vector<const aff3ct::module::Task*> get_tasks() const;

private:
	template <typename T>
	int _bind(const std::string &start_sck_name, Block &dest_block, const std::string &dest_sck_name);
	NT_Buffered_Socket& get_buffered_socket(const std::string &name,
	                                        std::map<std::string, std::unique_ptr<NT_Buffered_Socket>> &buffered_sockets);
	template <typename T>
	Buffered_Socket<T>& get_buffered_socket(const std::string &name,
	                                        std::map<std::string, std::unique_ptr<NT_Buffered_Socket>> &buffered_sockets);
	template <typename T>
	Buffered_Socket<T>& get_buffered_socket_in(const std::string &name);
	template <typename T>
	Buffered_Socket<T>& get_buffered_socket_out(const std::string &name);
};

#endif /* BLOCK_HPP */