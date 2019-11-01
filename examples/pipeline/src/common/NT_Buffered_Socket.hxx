#include "NT_Buffered_Socket.hpp"

NT_Buffered_Socket
::NT_Buffered_Socket(const std::vector<std::shared_ptr<aff3ct::module::Socket>> &sockets,
                     const aff3ct::module::socket_t sockets_type,
                     const size_t buffer_size)
: sockets(sockets.begin(), sockets.end()),
  sockets_type(sockets_type),
  buffer_size(buffer_size)
{
}

const aff3ct::module::Socket& NT_Buffered_Socket
::get_socket() const
{
	return *this->sockets[0].get();
}

const aff3ct::module::Socket& NT_Buffered_Socket
::get_s() const
{
	return this->get_socket();
}