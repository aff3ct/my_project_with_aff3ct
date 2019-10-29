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
	NT_Buffered_Socket(std::vector<aff3ct::module::Socket* > sockets, aff3ct::module::Socket_type sockets_type, int buffer_size)
	: sockets(sockets.begin(), sockets.end()),
	  sockets_type(sockets_type),
	  sockets_nbr(sockets.size()),
	  buffer_size(buffer_size),
	  type_name(sockets[0]->get_datatype_string()),
	  name(sockets[0]->get_name())
	{
	}

	virtual ~NT_Buffered_Socket(){};

	virtual void  stop             () = 0;
	virtual void  reset            () = 0;
	virtual int   pop              (int proc_idx) = 0;
	virtual int   push             (int proc_idx) = 0;
	virtual void  wait_pop         (int proc_idx) = 0;
	virtual void  wait_push        (int proc_idx) = 0;	
	virtual void  print_socket_data() = 0;
	
	inline std::string             get_name       () const { return this->name;     }
	inline std::string             get_type_name  () const { return this->type_name;}
	inline aff3ct::module::Socket* get_socket     () const { return this->sockets[0];   }

protected:
	std::vector<aff3ct::module::Socket* > sockets;
	aff3ct::module::Socket_type           sockets_type;
	size_t                                sockets_nbr;
	size_t                                buffer_size;
	const std::string                     type_name;
	const std::string                     name;
};

#endif //NT_BUFFERED_SOCKET_HPP
