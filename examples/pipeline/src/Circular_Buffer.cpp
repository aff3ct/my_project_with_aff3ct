#include <cassert>
#include <iostream>
#include <vector>
#include <mutex>
#include <algorithm>
#include "Circular_Buffer.hpp"

template <typename T, class A>
Circular_Buffer<T,A>
::Circular_Buffer (size_t max_buffer_nbr, size_t elt_per_buffer)
: max_buffer_nbr(max_buffer_nbr), 
  elt_per_buffer(elt_per_buffer),
  head_buffer(0), 
  tail_buffer(0),
  cb_size(max_buffer_nbr+1),
  circular_buffer(max_buffer_nbr+1, nullptr),
  lock_(),
  cond_(),
  stop_signal(false)
{
	assert(max_buffer_nbr > 0);
	assert(elt_per_buffer > 0);
	for (auto &b:this->circular_buffer)
		b = new std::vector<T,A>(elt_per_buffer,T(0));
	
}

template <typename T, class A>
Circular_Buffer<T,A>
::~Circular_Buffer()
{

	//delete this->lock;
	
	for (auto &b:this->circular_buffer)
		delete b;
}

template <typename T, class A>
void Circular_Buffer<T,A>
::reset()
{
	this->head_buffer = 0; 
	this->tail_buffer = 0;
	this->stop_signal = false;
}

template <typename T, class A>
void Circular_Buffer<T,A>
::stop()
{
	std::unique_lock<std::mutex> mlock(this->lock_);
	this->stop_signal = true;
	mlock.unlock();
	this->cond_.notify_all();
}

template <typename T, class A>
int Circular_Buffer<T,A>
::pop( std::vector<T,A>** elt)
{
	assert(elt[0]->size() == this->elt_per_buffer);

	if (this->is_empty())
		return 1;

	std::unique_lock<std::mutex> mlock(this->lock_);

	std::vector<T,A>* tmp = this->circular_buffer[this->tail_buffer];
	this->circular_buffer[this->tail_buffer] = *elt;
	*elt = tmp;
	this->tail_buffer = (this->tail_buffer + 1)%this->cb_size;
	mlock.unlock();
	this->cond_.notify_one();
	return 0;
}

template <typename T, class A>
int Circular_Buffer<T,A>
::push(std::vector<T,A>** elt)
{
	assert(elt[0]->size() == this->elt_per_buffer);
	if (this->is_full())
		return 1;

	std::unique_lock<std::mutex> mlock(this->lock_);

	std::vector<T,A>* tmp=this->circular_buffer[this->head_buffer];
	this->circular_buffer[this->head_buffer] = *elt;
	*elt = tmp;
	this->head_buffer = (this->head_buffer + 1)%this->cb_size;
	mlock.unlock();
	this->cond_.notify_one();
	return 0;
}

template <typename T, class A>
void Circular_Buffer<T,A>
::wait_pop( std::vector<T,A>** elt)
{
	assert(elt[0]->size() == this->elt_per_buffer);

	std::unique_lock<std::mutex> mlock(this->lock_);
	while (this->is_empty() && !this->stop_signal)
	{
		this->cond_.wait(mlock);
	}
	if (this->stop_signal)
		return;
	
	std::vector<T,A>* tmp = this->circular_buffer[this->tail_buffer];
	this->circular_buffer[this->tail_buffer] = *elt;
	*elt = tmp;
	this->tail_buffer = (this->tail_buffer + 1)%this->cb_size;
	mlock.unlock();
	this->cond_.notify_one();
}

template <typename T, class A>
void Circular_Buffer<T,A>
::wait_push(std::vector<T,A>** elt)
{
	assert(elt[0]->size() == this->elt_per_buffer);
	std::unique_lock<std::mutex> mlock(this->lock_);
	while (this->is_full() && !this->stop_signal)
	{
		this->cond_.wait(mlock);
	}
	
	if (this->stop_signal)
		return;

	std::vector<T,A>* tmp=this->circular_buffer[this->head_buffer];
	this->circular_buffer[this->head_buffer] = *elt;
	*elt = tmp;
	this->head_buffer = (this->head_buffer + 1)%this->cb_size;
	mlock.unlock();
	this->cond_.notify_one();
}

template <typename T, class A>
void Circular_Buffer<T,A>
::print() const
{
    std::cout << "---------------------------------------" << std::endl;
	std::cout << "Buffer max size : "<< this->max_buffer_nbr << std::endl;
	std::cout << "Buffer current size : "<< this->get_cur_buffer_nbr() << std::endl;
	std::cout << "Head : "<< this->head_buffer << std::endl;
	std::cout << "Tail : "<< this->tail_buffer << std::endl;
	std::cout << "---------------------------------------" << std::endl;
	for (size_t it = 0 ; it < this->cb_size ; it++ )
	{
		for (size_t elt = 0 ; elt < this->elt_per_buffer ; elt++ ) 
			std::cout << (T)(this->circular_buffer[it]->at(elt)) << "\t ";
    	if (it == this->head_buffer)
			std::cout << "<-H";
    	if (it == this->tail_buffer)
			std::cout << "<-T";			
		std::cout << std::endl;
	}
	std::cout << "---------------------------------------" << std::endl;	
}

template class Circular_Buffer<int8_t >;
template class Circular_Buffer<int16_t>;
template class Circular_Buffer<int32_t>;
template class Circular_Buffer<int64_t>;
template class Circular_Buffer<float  >;
template class Circular_Buffer<double >;
