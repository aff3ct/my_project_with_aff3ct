#ifndef NT_BUFFERED_SOCKET_HPP
#define NT_BUFFERED_SOCKET_HPP

#include <cassert>
#include <iostream>
#include <vector>
#include <aff3ct.hpp>
#include "Circular_Buffer.hpp"

class NT_Buffered_Socket
{
public:
	NT_Buffered_Socket(aff3ct::module::Socket* socket, aff3ct::module::Socket_type socket_type, int buffer_size)
	: socket(socket),
	  socket_type(socket_type),
	  buffer_size(buffer_size),
	  type_name(socket->get_datatype_string()),
	  name(socket->get_name())
	{
	}
	virtual ~NT_Buffered_Socket(){};

	virtual void stop() = 0;
	virtual void reset() = 0;
	virtual int  pop  () = 0;
	virtual int  push () = 0;
	virtual void  wait_pop  () = 0;
	virtual void  wait_push () = 0;	
	virtual void print_socket_data() = 0;
	
	inline std::string             get_name       () const { return this->name;           }
	inline std::string             get_type_name  () const { return this->type_name;      }
	inline aff3ct::module::Socket* get_socket     () const { return this->socket;           }

protected:
	aff3ct::module::Socket* socket;
	aff3ct::module::Socket_type socket_type;
	unsigned int buffer_size;
	const std::string type_name;
	const std::string     name;
};

#endif //NT_BUFFERED_SOCKET_HPP
