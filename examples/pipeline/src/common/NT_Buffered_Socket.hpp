#ifndef NT_BUFFERED_SOCKET_HPP
#define NT_BUFFERED_SOCKET_HPP

#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <typeindex>
#include <aff3ct.hpp>

#include "Circular_Buffer.hpp"

class NT_Buffered_Socket
{
public:
	NT_Buffered_Socket(std::vector<std::shared_ptr<aff3ct::module::Socket>> sockets, aff3ct::module::socket_t sockets_type, int buffer_size, const std::type_index datatype)
	: sockets(sockets.begin(), sockets.end()),
	  sockets_type(sockets_type),
	  sockets_nbr(sockets.size()),
	  buffer_size(buffer_size),
	  type_name(sockets[0]->get_datatype_string()),
	  name(sockets[0]->get_name()),
	  datatype(datatype)
	{
	}

	virtual ~NT_Buffered_Socket() = default;

	virtual void  stop             () = 0;
	virtual void  reset            () = 0;
	virtual int   pop              (size_t proc_idx) = 0;
	virtual int   push             (size_t proc_idx) = 0;
	virtual void  wait_pop         (size_t proc_idx) = 0;
	virtual void  wait_push        (size_t proc_idx) = 0;
	virtual void  print_socket_data() = 0;

	inline std::string                             get_name       () const { return this->name;       }
	inline std::string                             get_type_name  () const { return this->type_name;  }
	inline std::shared_ptr<aff3ct::module::Socket> get_socket     () const { return this->sockets[0]; }
	inline std::type_index                         get_datatype   () const { return this->datatype;   }

protected:
	std::vector<std::shared_ptr<aff3ct::module::Socket>> sockets;
	aff3ct::module::socket_t                             sockets_type;
	size_t                                               sockets_nbr;
	size_t                                               buffer_size;
	const std::string                                    type_name;
	const std::string                                    name;
	const std::type_index                                datatype;
};

#endif //NT_BUFFERED_SOCKET_HPP
