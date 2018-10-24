#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <aff3ct.hpp>

#include "Circular_Buffer.hpp"
#include "Buffered_Socket.hpp"

template<typename T>
Buffered_Socket<T>
::Buffered_Socket(aff3ct::module::Socket* socket, aff3ct::module::Socket_type socket_type, int buffer_size)
:socket(socket), 
socket_type(socket_type), 
buffer_size(buffer_size), 
type_name(socket->get_datatype_string()),
name(socket->get_name()),
socket_data(),
buffer(),
pop_buffer_idx(0)
{
	int n_elt = socket->get_n_elmts();
	std::string type_name = socket->get_datatype_string();
	
	this->socket_data.push_back(new std::vector<T>(n_elt,T(0)));
	this->socket->template bind<T>(*this->socket_data[0]);
	if (socket_type == aff3ct::module::Socket_type::OUT || socket_type == aff3ct::module::Socket_type::IN_OUT)
		this->buffer.push_back(new Circular_Buffer<T>(buffer_size, n_elt));
}


template<typename T>
Buffered_Socket<T>
::~Buffered_Socket(){

	if (this->socket_type == aff3ct::module::Socket_type::OUT || 
	    this->socket_type == aff3ct::module::Socket_type::IN_OUT)
	{
		for (auto &b:this->buffer)
			delete b;
	}
	for (auto &sd:this->socket_data)
		delete sd;
};

template<typename T>
void Buffered_Socket<T>
:: reset()
{
	for (auto const & buff:this->buffer)
		buff->reset();
};

template<typename T>
int Buffered_Socket<T>
:: pop()
{
	std::vector<T>* new_buffer = this->buffer[0]->pop(this->socket_data[0]);
	if (new_buffer == nullptr)
		return 1;
	else
	{
		this->socket_data[0] = new_buffer;
		this->socket->template bind<T>(*this->socket_data[0]);
		//pop_buffer_idx = (pop_buffer_idx+1) % this->buffer.size();
		return 0;
	}
};


template<typename T>
int Buffered_Socket<T>
:: push()
{
	for (int i=1 ; i < this->socket_data.size(); i++)
	{
		for (int j = 0 ; j < this->socket_data[0]->size() ; j++)
			this->socket_data[i]->at(j) = this->socket_data[0]->at(j);
	}

	for (int i=0 ; i<this->buffer.size(); i++)
	{
		if (this->buffer[i]->is_full())
			return 1;
	}

	for (int i=0 ; i<this->buffer.size(); i++)
		this->socket_data[i] = this->buffer[i]->push(this->socket_data[i]);

	this->socket->template bind<T>(*this->socket_data[0]);
	return 0;
};

template<typename T>
void Buffered_Socket<T>
::create_new_out_buffer() 
{	
	assert(this->socket_type != aff3ct::module::Socket_type::IN);
	int n_elt = this->socket->get_n_elmts();
	this->socket_data.push_back(new std::vector    <T>(n_elt,        T(0)));
	this->buffer     .push_back(new Circular_Buffer<T>(this->buffer_size, n_elt));
};

template<typename T>
int Buffered_Socket<T>
::bind(Buffered_Socket<T>* s) 
{	
	if (s->get_last_buffer() == nullptr)
		return 1;
	else
	{
		this->buffer.push_back(s->get_last_buffer());
		return 0;
	}
};

template<typename T>
int Buffered_Socket<T>
::bind_cpy(Buffered_Socket<T>* s) 
{	
	assert(this->socket_type == aff3ct::module::Socket_type::IN);
	if (s->get_last_buffer() == nullptr)
		return 1;
	else
	{
		s->create_new_out_buffer();
		this->buffer.push_back(s->get_last_buffer());
		return 0;
	}
};

template<typename T>
void Buffered_Socket<T>
::print_socket_data()
{
	for (int j = 0; j<this->socket_data.size(); j++)
	{
		std::cout << this->name << "(" << j << "): Buffer Size : [ " << this->buffer[j]->get_cur_buffer_nbr() 
		<< "]" << "\n";
	}
}

template class Buffered_Socket<int8_t >;
template class Buffered_Socket<int16_t>;
template class Buffered_Socket<int32_t>;
template class Buffered_Socket<int64_t>;
template class Buffered_Socket<float  >;
template class Buffered_Socket<double >;
