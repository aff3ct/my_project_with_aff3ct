#include "Pipeline_socket.hpp"

namespace aff3ct
{
namespace tools
{
Pipeline_socket
::Pipeline_socket(const std::vector<std::shared_ptr<module::Socket>> &sockets,
                  const module::socket_t sockets_type,
                  const size_t buffer_size)
: sockets(sockets.begin(), sockets.end()),
  sockets_type(sockets_type),
  buffer_size(buffer_size)
{
}

const module::Socket& Pipeline_socket
::get_socket() const
{
	return *this->sockets[0].get();
}

const module::Socket& Pipeline_socket
::get_s() const
{
	return this->get_socket();
}
}
}