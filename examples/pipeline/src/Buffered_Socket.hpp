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

class Buffered_Socket
{
protected:
aff3ct::module::Socket* socket;
unsigned int buffer_size;
aff3ct::module::Socket_type socket_type;
const std::string     name;
const std::string type_name;

std::vector<int8_t >*   int8_socket_data;
std::vector<int16_t>*  int16_socket_data;
std::vector<int32_t>*  int32_socket_data;
std::vector<int64_t>*  int64_socket_data;
std::vector<float  >*  float_socket_data;
std::vector<double >* double_socket_data;

Circular_Buffer<int8_t >*   int8_buffer;
Circular_Buffer<int16_t>*  int16_buffer;
Circular_Buffer<int32_t>*  int32_buffer;
Circular_Buffer<int64_t>*  int64_buffer;
Circular_Buffer<float  >*  float_buffer;
Circular_Buffer<double >* double_buffer;

public:
Buffered_Socket(aff3ct::module::Socket* socket, aff3ct::module::Socket_type socket_type, int buffer_size);
virtual ~Buffered_Socket();

int pop();
int push();
int bind(Buffered_Socket & s);
inline std::string     get_name            () const { return this->name;        }
inline std::string     get_type_name       () const { return this->type_name;   }

Circular_Buffer<int8_t >* get_int8_buffer  () const {return this->int8_buffer;  }
Circular_Buffer<int16_t>* get_int16_buffer () const {return this->int16_buffer; }
Circular_Buffer<int32_t>* get_int32_buffer () const {return this->int32_buffer; }
Circular_Buffer<int64_t>* get_int64_buffer () const {return this->int64_buffer; }
Circular_Buffer<float  >* get_float_buffer () const {return this->float_buffer; }
Circular_Buffer<double >* get_double_buffer() const {return this->double_buffer;}

std::vector<int8_t >* get_int8_socket_data  () const {return this->int8_socket_data;  }
std::vector<int16_t>* get_int16_socket_data () const {return this->int16_socket_data; }
std::vector<int32_t>* get_int32_socket_data () const {return this->int32_socket_data; }
std::vector<int64_t>* get_int64_socket_data () const {return this->int64_socket_data; }
std::vector<float  >* get_float_socket_data () const {return this->float_socket_data; }
std::vector<double >* get_double_socket_data() const {return this->double_socket_data;}

};

#endif //BUFFERED_SOCKET_HPP
