#ifndef PIPELINE_BLOCK_HPP
#define PIPELINE_BLOCK_HPP

#include <map>
#include <thread>
#include <vector>
#include <memory>
#include <string>

#include <Module/Task.hpp>

#include "../Socket/Pipeline_socket.hpp"
#include "../Socket/Pipeline_socket_buffered.hpp"

namespace aff3ct
{
namespace tools
{
class Pipeline_block
{
protected:
	const size_t n_threads;
	const size_t buffer_size;
	std::vector<std::shared_ptr<module::Task>> tasks;
	std::vector<std::thread> threads;
	std::map<std::string, std::unique_ptr<Pipeline_socket>> buffered_sockets_in;
	std::map<std::string, std::unique_ptr<Pipeline_socket>> buffered_sockets_out;

public:
	Pipeline_block(const module::Task &task, const size_t buffer_size, const size_t n_threads = 1);
	virtual ~Pipeline_block() = default;
	void bind(const std::string &start_sck_name, Pipeline_block &dest_block, const std::string &dest_sck_name);
	void execute_task(const size_t tid, const bool &is_done);
	void run(const bool &is_done);
	void join();
	void reset();
	std::vector<const module::Task*> get_tasks() const;

private:
	template <typename T>
	void _bind(const std::string &start_sck_name, Pipeline_block &dest_block, const std::string &dest_sck_name);
	Pipeline_socket& get_buffered_socket(const std::string &name,
	                                     std::map<std::string, std::unique_ptr<Pipeline_socket>> &buffered_sockets);
	template <typename T>
	Pipeline_socket_buffered<T>& get_buffered_socket(const std::string &name,
	                                                 std::map<std::string, std::unique_ptr<Pipeline_socket>> &buffered_sockets);
	template <typename T>
	Pipeline_socket_buffered<T>& get_buffered_socket_in(const std::string &name);
	template <typename T>
	Pipeline_socket_buffered<T>& get_buffered_socket_out(const std::string &name);
};
}
}

#endif /* PIPELINE_BLOCK_HPP */