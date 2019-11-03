#include "Circular_buffer.hpp"

namespace aff3ct
{
namespace tools
{
template <typename T>
size_t Circular_buffer<T>
::get_cur_buffer_nbr() const
{
	return this->head_buffer - this->tail_buffer;
}

template <typename T>
bool Circular_buffer<T>
::is_full() const
{
	return this->get_cur_buffer_nbr() == this->circular_buffer.size();
}

template <typename T>
bool Circular_buffer<T>
::is_empty() const
{
	return this->get_cur_buffer_nbr() == 0;
}
}
}
