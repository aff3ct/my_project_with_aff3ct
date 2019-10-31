#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <typeindex>
#include <typeinfo>
#include <aff3ct.hpp>

#include "Circular_Buffer.hpp"
#include "NT_Buffered_Socket.hpp"
#include "Buffered_Socket.hpp"

template<typename T>
Buffered_Socket<T>
::Buffered_Socket(std::vector<std::shared_ptr<aff3ct::module::Socket> > sockets, aff3ct::module::socket_t sockets_type, int buffer_size)
:NT_Buffered_Socket(sockets, sockets_type, buffer_size, std::type_index(typeid(T))),
sockets_data(),
buffer(nullptr),
pop_buffer_idx(0),
push_buffer_idx(0)
{
	int         n_elt     = sockets[0]->get_n_elmts();
	std::string type_name = sockets[0]->get_datatype_string();

	for (size_t i = 0; i < this->sockets_nbr ; i++)
		this->sockets_data.push_back(new std::vector<T>(n_elt,(T)0));

	for (size_t i = 0; i < this->sockets_nbr ; i++)
		this->sockets[i]->template bind<T>(*this->sockets_data[i]);

	if (sockets_type == aff3ct::module::socket_t::SOUT || sockets_type == aff3ct::module::socket_t::SIN_SOUT)
			this->buffer = new Circular_Buffer<T>(buffer_size, n_elt);
}


template<typename T>
Buffered_Socket<T>
::~Buffered_Socket(){

	if (this->sockets_type == aff3ct::module::socket_t::SOUT ||
	    this->sockets_type == aff3ct::module::socket_t::SIN_SOUT)
	{
		delete this->buffer;
	}
	for (auto &sd:this->sockets_data)
		delete sd;
};

template<typename T>
void Buffered_Socket<T>
:: reset()
{
	this->buffer->reset();
	this->pop_buffer_idx  = 0;
	this->push_buffer_idx = 0;
};

template<typename T>
void Buffered_Socket<T>
:: stop()
{
	this->buffer->stop();
};

template<typename T>
int Buffered_Socket<T>
:: push(int sck_idx)
{
	size_t the_push_idx = this->push_buffer_idx;
	if (sck_idx != (the_push_idx % this->sockets_nbr))
		return 1;

	if(this->buffer->push(&this->sockets_data[sck_idx]) == 1)
		return 1;

	this->sockets[sck_idx]->template bind<T>(*this->sockets_data[sck_idx]);
	this->push_buffer_idx++;
	return 0;
};

template<typename T>
int Buffered_Socket<T>
:: pop(int sck_idx)
{
	size_t the_pop_idx = this->pop_buffer_idx;
	if (sck_idx != (the_pop_idx % this->sockets_nbr))
		return 1;

	if(this->buffer->pop(&this->sockets_data[sck_idx]) == 1)
		return 1;

	this->sockets[sck_idx]->template bind<T>(*this->sockets_data[sck_idx]);
	this->pop_buffer_idx++;
	return 0;
};


template<typename T>
void Buffered_Socket<T>
:: wait_push(int sck_idx)
{
	/*for (int i=1 ; i < this->socket_data.size(); i++)
	{
		for (int j = 0 ; j < this->socket_data[0]->size() ; j++)
			this->socket_data[i]->at(j) = this->socket_data[0]->at(j);
	}

	for (int i=0 ; i<this->buffer.size(); i++)
		this->buffer[i]->wait_push(&this->socket_data[i]);

	this->socket->template bind<T>(*this->socket_data[0]);

	this->buffers[sck_idx]->wait_push(&this->sockets_data[sck_idx]);
	this->sockets[sck_idx]->template   bind<T>(*this->sockets_data[sck_idx]);
	*/
};


template<typename T>
void Buffered_Socket<T>
:: wait_pop(int sck_idx)
{
//	this->buffers[sck_idx]         ->wait_pop(&this->sockets_data[sck_idx]);
//	this->sockets[sck_idx]->template bind<T> (*this->sockets_data[sck_idx]);
};

template<typename T>
int Buffered_Socket<T>
::bind(Buffered_Socket<T>* s)
{
	if (s->get_buffer() == nullptr)
	{
		return 1;
	}
	else
	{
		this->buffer = s->get_buffer();
		return 0;
	}
};

template<typename T>
void Buffered_Socket<T>
::print_socket_data()
{
//	for (int j = 0; j<this->socket_data.size(); j++)
//	{
//		std::cout << this->name << "(" << j << "): Buffer Size : [ " << this->buffer[j]->get_cur_buffer_nbr()
//		<< "]" << "\n";
//	}
}

template class Buffered_Socket<int8_t >;
template class Buffered_Socket<int16_t>;
template class Buffered_Socket<int32_t>;
template class Buffered_Socket<int64_t>;
template class Buffered_Socket<float  >;
template class Buffered_Socket<double >;
