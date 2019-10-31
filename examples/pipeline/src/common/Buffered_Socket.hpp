//
// Created by Romain on 23/05/2018.
//

#ifndef BUFFERED_SOCKET_HPP
#define BUFFERED_SOCKET_HPP

#include <cassert>
#include <iostream>
#include <vector>
#include <atomic>
//#include <mipp.h>
#include <aff3ct.hpp>

#include "NT_Buffered_Socket.hpp"
#include "Circular_Buffer.hpp"

template<typename T>
class Buffered_Socket : public NT_Buffered_Socket
{
public:
	Buffered_Socket(std::vector<std::shared_ptr<aff3ct::module::Socket> > sockets, aff3ct::module::socket_t sockets_type, int buffer_size);
	virtual ~Buffered_Socket();

	void  stop      (           );
	void  reset     (           );
	int   pop       (int sck_idx);
	int   push      (int sck_idx);
	void  wait_pop  (int sck_idx);
	void  wait_push (int sck_idx);

	int  bind       (Buffered_Socket<T>* s);

	void print_socket_data();

	inline Circular_Buffer<T>* get_buffer     () const { return this->buffer; };

protected:
	std::vector<    std::vector<T>* > sockets_data;
	//std::vector<Circular_Buffer<T>* > buffers;
	Circular_Buffer<T>*               buffer;

private:
	std::atomic<size_t> pop_buffer_idx;
	std::atomic<size_t> push_buffer_idx;
};

#endif //BUFFERED_SOCKET_HPP
