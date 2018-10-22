#include <cassert>
#include <iostream>
#include <vector>
//#include <mipp.h>
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
connection_nbr(0),
socket_data(nullptr),
buffer(nullptr)
{
	int n_elt = socket->get_n_elmts();
	std::string type_name = socket->get_datatype_string();
	
	this->socket_data = new std::vector<T>(n_elt,T(0));
	socket->bind<T>(*socket_data);
	if (socket_type == aff3ct::module::Socket_type::OUT || socket_type == aff3ct::module::Socket_type::IN_OUT)
	{
		this->buffer = new Circular_Buffer<T>(buffer_size, n_elt);
		std::cout << "I Built a "<< this->socket->get_datatype_string() <<" Circular Buffer for socket named " << this->socket->get_name() << std::endl; 
	}
	else
	{
		this->buffer = nullptr;
		std::cout << "No " << this->socket->get_datatype_string() <<" Circular Buffer built for socket named " << this->socket->get_name() << std::endl; 
	}

}


template<typename T>
Buffered_Socket<T>
::~Buffered_Socket(){
	std::cout << "Start of cleaning allocated Circular Buffer..." << std::endl;
	int buffer_idx = 0;
	std::string type_name = this->socket->get_datatype_string();

	if (this->socket_type == aff3ct::module::Socket_type::OUT || this->socket_type == aff3ct::module::Socket_type::IN_OUT)
	{
		std::cout << "Cleaning of a circular buffer for socket "<< this->socket->get_name() << std::endl;
		delete this->buffer;
	}
	delete this->socket_data;

	std::cout << "End of cleaning." << std::endl;
};

template<typename T>
int Buffered_Socket<T>
:: pop()
{	
	std::vector<T>* new_buffer = this->buffer->pop(this->socket_data);
	if (new_buffer == nullptr)
		return 1;
	else
	{
		this->socket_data = new_buffer;
		this->socket->template bind<T>(*this->socket_data);
		return 0;
	}
};


template<typename T>
int Buffered_Socket<T>
:: push()
{
		std::vector<T>* new_buffer = this->buffer->push(this->socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->socket_data = new_buffer;
			this->socket->template bind<T>(*socket_data);
			return 0;
		}
};

template<typename T>
int Buffered_Socket<T>
::bind(Buffered_Socket<T>* s) 
{	
	if (s->get_buffer() == nullptr)
		return 1;
	else
	{
		this->buffer = s->get_buffer();
		connection_nbr++;
		return 0;
	}
};

template<typename T>
void Buffered_Socket<T>
::print_socket_data()
{
	std::cout << this->name <<" : [ ";
	for (int i = 0 ; i < this->socket_data->size() ; i++)
	{
		std::cout << this->socket_data->at(i) << "\t ";
	}
	std::cout << "]\n";
}

template class Buffered_Socket<int8_t>;
template class Buffered_Socket<int16_t>;
template class Buffered_Socket<int32_t>;
template class Buffered_Socket<int64_t>;
template class Buffered_Socket<float>;
template class Buffered_Socket<double>;
