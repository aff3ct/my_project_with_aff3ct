//
// Created by Romain on 23/05/2018.
//

#ifndef CIRCULAR_BUFFER_HPP
#define CIRCULAR_BUFFER_HPP

#include <cassert>
#include <iostream>
#include <vector>
#include <mutex>
#include <algorithm>
template <typename T, class A = std::allocator<T>>
class Circular_Buffer
{
private:
	size_t max_buffer_nbr;
	size_t elt_per_buffer;

	size_t head_buffer;
	size_t tail_buffer;
	size_t cur_buffer_nbr;

	std::vector<std::vector<T,A> *> circular_buffer;
	std::recursive_mutex* lock;
	//TODO std::vector<std::recursive_mutex*> data_locks;

public:
	Circular_Buffer (size_t max_buffer_nbr = 0, size_t elt_per_buffer = 0);
	virtual ~Circular_Buffer();

	std::vector<T,A>*  pop(std::vector<T,A>* elt);
	std::vector<T,A>* push(std::vector<T,A>* elt);
	void print();
};

template <typename T, class A>
Circular_Buffer<T,A>
::Circular_Buffer(size_t max_buffer_nbr, size_t elt_per_buffer)
: max_buffer_nbr(max_buffer_nbr), 
  elt_per_buffer(elt_per_buffer),
  circular_buffer(max_buffer_nbr, nullptr), 
  head_buffer(0), 
  tail_buffer(0),
  cur_buffer_nbr(0),
  lock(nullptr)
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
std::vector<T,A>* Circular_Buffer<T,A>
::pop( std::vector<T,A>* elt)
{
	assert(elt->size() == elt_per_buffer);

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
	
	//std::swap(this->circular_buffer[this->head_buffer], elt);
	
	if (this->head_buffer == this->tail_buffer && this->cur_buffer_nbr > 0)
		this->tail_buffer = (this->tail_buffer + 1)%this->max_buffer_nbr;

	this->head_buffer = (this->head_buffer + 1)%this->max_buffer_nbr;
	this->cur_buffer_nbr = std::min(this->cur_buffer_nbr + 1, this->max_buffer_nbr);
	this->lock->unlock();

	return tmp;
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
			std::cout << (T)(this->circular_buffer[it]->at(elt)) << " ";
    	if (it == this->head_buffer)
			std::cout << "<-H";
    	if (it == this->tail_buffer)
			std::cout << "<-T";			
		std::cout << std::endl;
	}
	std::cout << "---------------------------------------" << std::endl;	
}

#endif //CIRCULAR_BUFFER_HPP
