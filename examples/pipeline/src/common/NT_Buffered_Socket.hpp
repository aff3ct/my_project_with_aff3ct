#ifndef NT_BUFFERED_SOCKET_HPP
#define NT_BUFFERED_SOCKET_HPP

#include <vector>
#include <memory>
#include <string>
#include <aff3ct.hpp>

class NT_Buffered_Socket
{
protected:
	std::vector<std::shared_ptr<aff3ct::module::Socket>> sockets;
	const aff3ct::module::socket_t sockets_type;
	const size_t buffer_size;

public:
	inline NT_Buffered_Socket(const std::vector<std::shared_ptr<aff3ct::module::Socket>> &sockets,
	                          const aff3ct::module::socket_t sockets_type,
	                          const size_t buffer_size);

	virtual ~NT_Buffered_Socket() = default;

	virtual void  stop      (               ) = 0;
	virtual void  reset     (               ) = 0;
	virtual int   pop       (size_t proc_idx) = 0;
	virtual int   push      (size_t proc_idx) = 0;
	virtual void  wait_pop  (size_t proc_idx) = 0;
	virtual void  wait_push (size_t proc_idx) = 0;
	virtual void  print_data(               ) = 0;

	inline const aff3ct::module::Socket& get_socket() const;
	inline const aff3ct::module::Socket& get_s     () const;
};

#include "NT_Buffered_Socket.hxx"

#endif //NT_BUFFERED_SOCKET_HPP
