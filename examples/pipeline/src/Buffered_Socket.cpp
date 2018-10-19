#include <cassert>
#include <iostream>
#include <vector>
//#include <mipp.h>
#include <aff3ct.hpp>

#include "Circular_Buffer.hpp"
#include "Buffered_Socket.hpp"

Buffered_Socket
::Buffered_Socket(aff3ct::module::Socket* socket, aff3ct::module::Socket_type socket_type, int buffer_size)
:socket(socket),
buffer_size(buffer_size),
socket_type(socket_type),
name(socket->get_name()),
type_name(socket->get_datatype_string()),
int8_socket_data(nullptr),int16_socket_data(nullptr),int32_socket_data(nullptr),int64_socket_data(nullptr),float_socket_data(nullptr),double_socket_data(nullptr),
int8_buffer(nullptr),int16_buffer(nullptr),int32_buffer(nullptr),int64_buffer(nullptr),float_buffer(nullptr),double_buffer(nullptr)
{
	int n_elt = socket->get_n_elmts();
	std::string type_name = socket->get_datatype_string();
	
	if (type_name == "int8")
		{
			int8_socket_data = new std::vector<int8_t>(n_elt,int8_t(0));
			socket->bind<int8_t>(*int8_socket_data);
			if (socket_type == aff3ct::module::Socket_type::OUT || socket_type == aff3ct::module::Socket_type::IN_OUT)
			{
				this->int8_buffer = new Circular_Buffer<int8_t>(buffer_size, n_elt);
				std::cout << "I Built an int8 Circular Buffer for socket named " << this->socket->get_name() << std::endl; 
			}
			else
			{
				this->int8_buffer = nullptr;
				std::cout << "No int8 Circular Buffer built for socket named " << this->socket->get_name() << std::endl; 
			}
		}
		else if (type_name == "int16")
		{
			int16_socket_data = new std::vector<int16_t>(n_elt,int16_t(0));
			socket->bind<int16_t>(*int16_socket_data);
			if (socket_type == aff3ct::module::Socket_type::OUT || socket_type == aff3ct::module::Socket_type::IN_OUT)
			{
				this->int16_buffer = new Circular_Buffer<int16_t>(buffer_size, n_elt);
				std::cout << "I Built an int16 Circular Buffer for socket named " << this->socket->get_name() << std::endl; 
			}
			else
			{
				this->int16_buffer = nullptr;
				std::cout << "No int8 Circular Buffer built for socket named " << this->socket->get_name() << std::endl; 
			}
		}
		else if (type_name == "int32")
		{
			int32_socket_data = new std::vector<int32_t>(n_elt,int32_t(0));
			socket->bind<int32_t>(*int32_socket_data);
			if (socket_type == aff3ct::module::Socket_type::OUT || socket_type == aff3ct::module::Socket_type::IN_OUT)
			{
				this->int32_buffer = new Circular_Buffer<int32_t>(buffer_size, n_elt);
				std::cout << "I Built an int32 Circular Buffer for socket named " << this->socket->get_name() << std::endl;
			}
			else
			{
				this->int32_buffer = nullptr;
				std::cout << "No int32 Circular Buffer built for socket named " << this->socket->get_name() << std::endl; 
			}
		}
		else if (type_name  == "int64")
		{
			int64_socket_data = new std::vector<int64_t>(n_elt,int64_t(0));
			socket->bind<int64_t>(*int64_socket_data);
			if (socket_type == aff3ct::module::Socket_type::OUT || socket_type == aff3ct::module::Socket_type::IN_OUT)
			{
				this->int64_buffer = new Circular_Buffer<int64_t>(buffer_size, n_elt);
				std::cout << "I Built an int64 Circular Buffer for socket named " << this->socket->get_name() << std::endl; 
			}
			else
			{
				this->int64_buffer = nullptr;
				std::cout << "No int64 Circular Buffer built for socket named " << this->socket->get_name() << std::endl; 
			}
		}
		else if (type_name == "float32")
		{
			float_socket_data = new std::vector<float>(n_elt,float(0));
			socket->bind<float>(*float_socket_data);
			if (socket_type == aff3ct::module::Socket_type::OUT || socket_type == aff3ct::module::Socket_type::IN_OUT)
			{
				this->float_buffer = new Circular_Buffer<float>(buffer_size, n_elt);
				std::cout << "I Built a float Circular Buffer for socket named " << this->socket->get_name() << std::endl;
			}
			else
			{
				this->float_buffer = nullptr;
				std::cout << "No float Circular Buffer built for socket named " << this->socket->get_name() << std::endl; 
			}
		}
		else if (type_name == "float64")
		{
			double_socket_data = new std::vector<double>(n_elt,double(0));
			socket->bind<double>(*double_socket_data);
			if (socket_type == aff3ct::module::Socket_type::OUT || socket_type == aff3ct::module::Socket_type::IN_OUT)
			{
				this->double_buffer = new Circular_Buffer<double>(buffer_size, n_elt);
				std::cout << "I Built a double Circular Buffer for socket named " << this->socket->get_name() << std::endl; 
			}
			else
			{
				this->double_buffer = nullptr;
				std::cout << "No double Circular Buffer built for socket named " << this->socket->get_name() << std::endl; 
			}
		}
	std::cout << "End of wrapping." << std::endl;
}

int Buffered_Socket
::bind(Buffered_Socket &s)
{	
	if (this->type_name == "int8")
		this->int8_buffer = s.get_int8_buffer();
	else if (this->type_name == "int16")
		this->int16_buffer = s.get_int16_buffer();
	else if (this->type_name == "int32")
		this->int32_buffer = s.get_int32_buffer();
	else if (this->type_name == "int64")
		this->int64_buffer = s.get_int64_buffer();
	else if (this->type_name == "float32")
		this->float_buffer = s.get_float_buffer();
	else if (this->type_name == "float64")
		this->double_buffer = s.get_double_buffer();
	else
		return 1;

	return 0;
}

int Buffered_Socket
::pop()
{	
	if (this->type_name == "int8")
	{
		std::vector<int8_t>* new_buffer = this->int8_buffer->pop(this->int8_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->int8_socket_data = new_buffer;
			this->socket->bind<int8_t>(*int8_socket_data);
			return 0;
		}
	}
	else if (this->type_name == "int16")
	{
		std::vector<int16_t>* new_buffer = this->int16_buffer->pop(this->int16_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->int16_socket_data = new_buffer;
			this->socket->bind<int16_t>(*int16_socket_data);
			return 0;
		}
	}
	else if (this->type_name == "int32")
	{
		std::vector<int32_t>* new_buffer = this->int32_buffer->pop(this->int32_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->int32_socket_data = new_buffer;
			this->socket->bind<int32_t>(*int32_socket_data);
			return 0;
		}
	}
	else if (this->type_name == "int64")
	{
		std::vector<int64_t>* new_buffer = this->int64_buffer->pop(this->int64_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->int64_socket_data = new_buffer;
			this->socket->bind<int64_t>(*int64_socket_data);
			return 0;
		}
	}
	else if (this->type_name == "float32")
	{
		std::vector<float>* new_buffer = this->float_buffer->pop(this->float_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->float_socket_data = new_buffer;
			this->socket->bind<float>(*float_socket_data);
			return 0;
		}
	}
	else if (this->type_name == "float64")
	{
		std::vector<double>* new_buffer = this->double_buffer->pop(this->double_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->double_socket_data = new_buffer;
			this->socket->bind<double>(*double_socket_data);
			return 0;
		}
	}
	else
		return 1;
}

int Buffered_Socket
::push()
{	
	if (this->type_name == "int8")
	{
		std::vector<int8_t>* new_buffer = this->int8_buffer->push(this->int8_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->int8_socket_data = new_buffer;
			this->socket->bind<int8_t>(*int8_socket_data);
			return 0;
		}
	}
	else if (this->type_name == "int16")
	{
		std::vector<int16_t>* new_buffer = this->int16_buffer->push(this->int16_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->int16_socket_data = new_buffer;
			this->socket->bind<int16_t>(*int16_socket_data);
			return 0;
		}
	}
	else if (this->type_name == "int32")
	{
		std::vector<int32_t>* new_buffer = this->int32_buffer->push(this->int32_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->int32_socket_data = new_buffer;
			this->socket->bind<int32_t>(*int32_socket_data);
			return 0;
		}
	}
	else if (this->type_name == "int64")
	{
		std::vector<int64_t>* new_buffer = this->int64_buffer->push(this->int64_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->int64_socket_data = new_buffer;
			this->socket->bind<int64_t>(*int64_socket_data);
			return 0;
		}
	}
	else if (this->type_name == "float32")
	{
		std::vector<float>* new_buffer = this->float_buffer->push(this->float_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->float_socket_data = new_buffer;
			this->socket->bind<float>(*float_socket_data);
			return 0;
		}
	}
	else if (this->type_name == "float64")
	{
		std::vector<double>* new_buffer = this->double_buffer->push(this->double_socket_data);
		if (new_buffer == nullptr)
			return 1;
		else
		{
			this->double_socket_data = new_buffer;
			this->socket->bind<double>(*double_socket_data);
			return 0;
		}
	}
	else
		return 1;
}
Buffered_Socket
::~Buffered_Socket()
{
	std::cout << "Start of cleaning allocated Circular Buffer..." << std::endl;
	int buffer_idx = 0;
	std::string type_name = this->socket->get_datatype_string();

	if (type_name == "int8" && (this->socket_type == aff3ct::module::Socket_type::OUT || this->socket_type == aff3ct::module::Socket_type::IN_OUT))
	{
		std::cout << "Cleaning of a int8 circular buffer for socket "<< this->socket->get_name() << std::endl;
		delete(int8_buffer);
	}
	else if (type_name == "int16" && (this->socket_type == aff3ct::module::Socket_type::OUT || this->socket_type == aff3ct::module::Socket_type::IN_OUT))
	{
		std::cout << "Cleaning of a int8 circular buffer for socket "<< this->socket->get_name() << std::endl;
		delete(int16_buffer);
	}
	else if (type_name == "int32" && (this->socket_type == aff3ct::module::Socket_type::OUT || this->socket_type == aff3ct::module::Socket_type::IN_OUT))
	{
		std::cout << "Cleaning of a int32 circular buffer for socket "<< this->socket->get_name() << std::endl;
		delete(int32_buffer);
	}
	else if (type_name == "int64" && (this->socket_type == aff3ct::module::Socket_type::OUT || this->socket_type == aff3ct::module::Socket_type::IN_OUT))
	{
		std::cout << "Cleaning of a int64 circular buffer for socket "<< this->socket->get_name() << std::endl;
		delete(int64_buffer);
	}
	else if (type_name == "float32" && (this->socket_type == aff3ct::module::Socket_type::OUT || this->socket_type == aff3ct::module::Socket_type::IN_OUT))
	{
		std::cout << "Cleaning of a float circular buffer for socket "<< this->socket->get_name() << std::endl;
		delete(float_buffer);
	}
	else if (type_name == "float64" && (this->socket_type == aff3ct::module::Socket_type::OUT || this->socket_type == aff3ct::module::Socket_type::IN_OUT))
	{
		std::cout << "Cleaning of a double circular buffer for socket "<< this->socket->get_name() << std::endl;
		delete(double_buffer);
	}
	std::cout << "End of cleaning." << std::endl;
}