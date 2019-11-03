#ifndef PIPELINE_SOCKET_HPP
#define PIPELINE_SOCKET_HPP

#include <vector>
#include <memory>
#include <string>

#include <Module/Task.hpp>
#include <Module/Socket.hpp>

namespace aff3ct
{
namespace tools
{
class Pipeline_socket
{
protected:
	std::vector<std::shared_ptr<module::Socket>> sockets;
	const module::socket_t sockets_type;
	const size_t buffer_size;

public:
	inline Pipeline_socket(const std::vector<std::shared_ptr<module::Socket>> &sockets,
	                       const module::socket_t sockets_type,
	                       const size_t buffer_size);

	virtual ~Pipeline_socket() = default;

	virtual void reset   (                ) = 0;
	virtual bool try_pop (const size_t tid) = 0;
	virtual bool try_push(const size_t tid) = 0;

	inline const module::Socket& get_socket() const;
	inline const module::Socket& get_s     () const;
};
}
}

#include "Pipeline_socket.hxx"

#endif //PIPELINE_SOCKET_HPP
