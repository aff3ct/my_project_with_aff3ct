#include <string>
#include <cassert>
#include <sstream>

#include <Tools/Exception/exception.hpp>

#include "Pipeline_socket_buffered.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

template<typename T>
Pipeline_socket_buffered<T>
::Pipeline_socket_buffered(const std::vector<std::shared_ptr<module::Socket>> &sockets,
                           const module::socket_t sockets_type,
                           const size_t buffer_size)
: Pipeline_socket(sockets, sockets_type, buffer_size),
  buffer(nullptr),
  pop_buffer_idx(0),
  push_buffer_idx(0)
{
	assert( pop_buffer_idx.is_lock_free());
	assert(push_buffer_idx.is_lock_free());

	auto n_elmts = sockets[0]->get_n_elmts();

	for (size_t i = 0; i < this->sockets.size(); i++)
		this->data.push_back(new T[n_elmts]);
	this->data_cpy = this->data;

	for (size_t i = 0; i < this->sockets.size(); i++)
		this->sockets[i]->template bind<T>(this->data[i]);

	if (sockets_type == module::socket_t::SOUT ||
	    sockets_type == module::socket_t::SIN_SOUT)
		this->buffer = new Circular_buffer<T>(buffer_size, n_elmts);
}

template<typename T>
Pipeline_socket_buffered<T>
::~Pipeline_socket_buffered()
{
	if (sockets_type == module::socket_t::SOUT ||
	    sockets_type == module::socket_t::SIN_SOUT)
		delete this->buffer;
	for (auto &d : this->data_cpy)
		delete[] d;
}

template<typename T>
void Pipeline_socket_buffered<T>
::reset()
{
	this->buffer->reset();
	this->pop_buffer_idx  = 0;
	this->push_buffer_idx = 0;
}

template<typename T>
bool Pipeline_socket_buffered<T>
::try_pop(const size_t tid)
{
	assert(tid < this->sockets.size());

	if (tid != this->pop_buffer_idx)
		return false;

	auto new_ptr = this->buffer->try_pop(static_cast<T*>(this->sockets[tid]->get_dataptr()));

	if (new_ptr == nullptr)
		return false;
	else
	{
		this->sockets[tid]->template bind<T>(new_ptr);
		this->pop_buffer_idx = (this->pop_buffer_idx +1) % this->sockets.size();
		return true;
	}
}

template<typename T>
bool Pipeline_socket_buffered<T>
::try_push(const size_t tid)
{
	assert(tid < this->sockets.size());

	if (tid != this->push_buffer_idx)
		return false;

	auto new_ptr = this->buffer->try_push(static_cast<T*>(this->sockets[tid]->get_dataptr()));

	if (new_ptr == nullptr)
		return false;
	else
	{
		this->sockets[tid]->template bind<T>(new_ptr);
		this->push_buffer_idx = (this->push_buffer_idx +1) % this->sockets.size();
		return true;
	}
}

template<typename T>
void Pipeline_socket_buffered<T>
::bind(const Pipeline_socket_buffered<T>& s)
{
	if (s.buffer == nullptr)
	{
		std::stringstream message;
		message << "'s.buffer' can't be nullptr.";
		throw runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->buffer = s.buffer;
}

template class aff3ct::tools::Pipeline_socket_buffered<int8_t >;
template class aff3ct::tools::Pipeline_socket_buffered<int16_t>;
template class aff3ct::tools::Pipeline_socket_buffered<int32_t>;
template class aff3ct::tools::Pipeline_socket_buffered<int64_t>;
template class aff3ct::tools::Pipeline_socket_buffered<float  >;
template class aff3ct::tools::Pipeline_socket_buffered<double >;
