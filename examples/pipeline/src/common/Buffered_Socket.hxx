#include "Buffered_Socket.hpp"

template<typename T>
Circular_Buffer<T>* Buffered_Socket<T>
::get_buffer() const
{
	return this->buffer;
};

