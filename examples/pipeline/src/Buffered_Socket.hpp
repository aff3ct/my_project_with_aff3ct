//
// Created by Romain on 23/05/2018.
//

#ifndef BUFFERED_SOCKET_HPP
#define BUFFERED_SOCKET_HPP

#include <cassert>
#include <iostream>
#include <vector>
//#include <mipp.h>
#include <aff3ct.hpp>
#include "Circular_Buffer.hpp"

template<typename T>
class Buffered_Socket
{
public:
	Buffered_Socket(aff3ct::module::Socket* socket, aff3ct::module::Socket_type socket_type, int buffer_size);
	virtual ~Buffered_Socket();

	void reset();
	int pop();
	int push();
	int bind    (Buffered_Socket<T>* s);
	int bind_cpy(Buffered_Socket<T>* s);
	void print_socket_data();

	void create_new_out_buffer();

	inline Circular_Buffer<T>* get_last_buffer    () const { return this->buffer.at(this->buffer.size()-1);   }
	inline std::string         get_name           () const { return this->name;           }
	inline std::string         get_type_name      () const { return this->type_name;      }

protected:
	aff3ct::module::Socket* socket;
	aff3ct::module::Socket_type socket_type;
	unsigned int buffer_size;
	const std::string type_name;
	const std::string     name;

	std::vector<    std::vector<T>*> socket_data;
	std::vector<Circular_Buffer<T>*> buffer;

private:
	int pop_buffer_idx = 0;
};

#endif //BUFFERED_SOCKET_HPP
