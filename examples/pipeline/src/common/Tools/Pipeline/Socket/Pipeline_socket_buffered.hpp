//
// Created by Romain on 23/05/2018.
//

#ifndef PIPELINE_SOCKET_BUFFERED_HPP
#define PIPELINE_SOCKET_BUFFERED_HPP

#include <vector>
#include <atomic>
#include <memory>

#include <Module/Task.hpp>
#include <Module/Socket.hpp>

#include "../../Algo/Circular_buffer/Circular_buffer.hpp"
#include "Pipeline_socket.hpp"

namespace aff3ct
{
namespace tools
{
template<typename T>
class Pipeline_socket_buffered : public Pipeline_socket
{
protected:
	Circular_buffer<T>* buffer;
	std::vector<T*> data;
	std::vector<T*> data_cpy;

private:
	std::atomic<size_t> pop_buffer_idx;
	std::atomic<size_t> push_buffer_idx;

public:
	Pipeline_socket_buffered(const std::vector<std::shared_ptr<module::Socket>> &sockets,
	                         const module::socket_t sockets_type,
	                         const size_t buffer_size);
	virtual ~Pipeline_socket_buffered();

	void reset   (                                    );
	bool try_pop (const size_t tid                    );
	bool try_push(const size_t tid                    );
	void bind    (const Pipeline_socket_buffered<T>& s);
};
}
}

#endif //PIPELINE_SOCKET_BUFFERED_HPP
