#include "Circular_Buffer.hpp"

template <typename T, class A>
size_t Circular_Buffer<T,A>
::get_max_buffer_nbr() const
{
	return this->max_buffer_nbr;
}

template <typename T, class A>
size_t Circular_Buffer<T,A>
::get_cur_buffer_nbr() const
{
	return this->head_buffer - this->tail_buffer + ((this->tail_buffer > this->head_buffer) ? this->cb_size :  0);
}

template <typename T, class A>
bool Circular_Buffer<T,A>
::is_full()  const
{
	return this->get_cur_buffer_nbr() == this->max_buffer_nbr;
}

template <typename T, class A>
bool Circular_Buffer<T,A>
::is_empty() const
{
	return this->get_cur_buffer_nbr() == 0;
}

