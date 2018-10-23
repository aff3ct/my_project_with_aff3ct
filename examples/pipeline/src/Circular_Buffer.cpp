#include <cassert>
#include <iostream>
#include <vector>
#include <mutex>
#include <algorithm>
#include "Circular_Buffer.hpp"

template <typename T, class A>
Circular_Buffer<T,A>
::Circular_Buffer (size_t max_buffer_nbr, size_t elt_per_buffer)
:max_buffer_nbr(max_buffer_nbr), 
  elt_per_buffer(elt_per_buffer),
  head_buffer(0), 
  tail_buffer(0),
  cur_buffer_nbr(0),
  lock(nullptr),
  circular_buffer(max_buffer_nbr, nullptr)
{
	assert(max_buffer_nbr > 0);
	assert(elt_per_buffer > 0);
	this->lock = new std::recursive_mutex();
	for (int i =0; i<this->max_buffer_nbr; i++)
	{
		this->circular_buffer[i] = new std::vector<T,A>(elt_per_buffer,T(0));
	}
}

template <typename T, class A>
Circular_Buffer<T,A>
::~Circular_Buffer()
{
	delete(this->lock);
	for (int i =0; i<this->max_buffer_nbr; i++)
	{
		delete(this->circular_buffer[i]);
	}

}

template <typename T, class A>
void Circular_Buffer<T,A>
::reset()
{
	this->head_buffer = 0; 
	this->tail_buffer = 0;
	this->cur_buffer_nbr = 0;
}

template <typename T, class A>
std::vector<T,A>* Circular_Buffer<T,A>
::pop( std::vector<T,A>* elt)
{
	assert(elt->size() == this->elt_per_buffer);

	if (this->cur_buffer_nbr == 0)
		return nullptr;

	this->lock->lock();
	std::vector<T,A>* tmp = this->circular_buffer[this->tail_buffer];
	this->circular_buffer[this->tail_buffer] = elt;

	this->tail_buffer = (this->tail_buffer + 1)%this->max_buffer_nbr;
	this->cur_buffer_nbr--;
	this->lock->unlock();
	return tmp;
}

template <typename T, class A>
std::vector<T,A>* Circular_Buffer<T,A>
::push(std::vector<T,A>* elt)
{
	assert(elt->size() == this->elt_per_buffer);
	if (this->cur_buffer_nbr == this->max_buffer_nbr)
		return nullptr;

	this->lock->lock();
	std::vector<T,A>* tmp=this->circular_buffer[this->head_buffer];
	this->circular_buffer[this->head_buffer] = elt;

	if (this->head_buffer == this->tail_buffer && this->cur_buffer_nbr > 0)
		this->tail_buffer = (this->tail_buffer + 1)%this->max_buffer_nbr;

	this->head_buffer = (this->head_buffer + 1)%this->max_buffer_nbr;
	this->cur_buffer_nbr = std::min(this->cur_buffer_nbr + 1, this->max_buffer_nbr);
	this->lock->unlock();

	return tmp;
}

template <typename T, class A>
bool Circular_Buffer<T,A>
::is_full()
{
	if (this->cur_buffer_nbr == this->max_buffer_nbr)
		return true;
	else
		return false;
}

template <typename T, class A>
bool Circular_Buffer<T,A>
::is_empty()
{
	if (this->cur_buffer_nbr == 0)
		return true;
	else
		return false;
}

template <typename T, class A>
void Circular_Buffer<T,A>
::print()
{
    std::cout << "---------------------------------------" << std::endl;
	std::cout << "Buffer max size : "<< this->max_buffer_nbr << std::endl;
	std::cout << "Buffer current size : "<< this->cur_buffer_nbr << std::endl;
	std::cout << "Head : "<< this->head_buffer << std::endl;
	std::cout << "Tail : "<< this->tail_buffer << std::endl;
	std::cout << "---------------------------------------" << std::endl;
	for (int it = 0 ; it < this->max_buffer_nbr ; it++ )
	{
		for (int elt = 0 ; elt < this->elt_per_buffer ; elt++ ) 
			std::cout << (T)(this->circular_buffer[it]->at(elt)) << "\t ";
    	if (it == this->head_buffer)
			std::cout << "<-H";
    	if (it == this->tail_buffer)
			std::cout << "<-T";			
		std::cout << std::endl;
	}
	std::cout << "---------------------------------------" << std::endl;	
}

template class Circular_Buffer<int8_t>;
template class Circular_Buffer<int16_t>;
template class Circular_Buffer<int32_t>;
template class Circular_Buffer<int64_t>;
template class Circular_Buffer<float>;
template class Circular_Buffer<double>;
