#include <cassert>
#include <limits>

#include "Circular_buffer.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

template <typename T>
Circular_buffer<T>
::Circular_buffer (const size_t buffer_size, const size_t n_elmts)
: head_buffer(0),
  tail_buffer(0),
  circular_buffer(buffer_size, nullptr)
{
	assert(buffer_size > 0);
	assert(n_elmts > 0);

	for (auto &b : this->circular_buffer)
		b = new T[n_elmts];
	this->circular_buffer_cpy = this->circular_buffer;
}

template <typename T>
Circular_buffer<T>
::~Circular_buffer()
{
	for (auto &b : this->circular_buffer_cpy)
		delete[] b;
}

template <typename T>
void Circular_buffer<T>
::reset()
{
	mtx.lock();
	this->head_buffer = 0;
	this->tail_buffer = 0;
	mtx.unlock();
}

template <typename T>
bool Circular_buffer<T>
::is_full() const
{
	return (this->head_buffer - this->tail_buffer) == this->circular_buffer.size();
}

template <typename T>
bool Circular_buffer<T>
::is_empty() const
{
	return (this->head_buffer - this->tail_buffer) == 0;
}

template <typename T>
T* Circular_buffer<T>
::try_pop(T* data_ptr)
{
	assert(data_ptr != nullptr);

	if (this->is_empty())
		return nullptr;

	mtx.lock();
	assert(!this->is_empty());
	std::swap(this->circular_buffer[this->tail_buffer % this->circular_buffer.size()], data_ptr);
	this->tail_buffer++;
	assert(this->tail_buffer != std::numeric_limits<size_t>::max());
	mtx.unlock();

	return data_ptr;
}

template <typename T>
T* Circular_buffer<T>
::try_push(T* data_ptr)
{
	assert(data_ptr != nullptr);

	if (this->is_full())
		return nullptr;

	mtx.lock();
	assert(!this->is_full());
	std::swap(this->circular_buffer[this->head_buffer % this->circular_buffer.size()], data_ptr);
	this->head_buffer++;
	assert(this->head_buffer != std::numeric_limits<size_t>::max());
	mtx.unlock();

	return data_ptr;
}

template class aff3ct::tools::Circular_buffer<int8_t >;
template class aff3ct::tools::Circular_buffer<int16_t>;
template class aff3ct::tools::Circular_buffer<int32_t>;
template class aff3ct::tools::Circular_buffer<int64_t>;
template class aff3ct::tools::Circular_buffer<float  >;
template class aff3ct::tools::Circular_buffer<double >;
