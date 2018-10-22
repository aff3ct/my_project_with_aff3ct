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
protected:
	size_t max_buffer_nbr;
	size_t elt_per_buffer;

	size_t head_buffer;
	size_t tail_buffer;
	size_t cur_buffer_nbr;

	std::recursive_mutex* lock;

private:
	std::vector<std::vector<T,A> *> circular_buffer;
	//TODO std::vector<std::recursive_mutex*> data_locks;

public:
	Circular_Buffer (size_t max_buffer_nbr = 0, size_t elt_per_buffer = 0);
	virtual ~Circular_Buffer();

	std::vector<T,A>*  pop(std::vector<T,A>* elt);
	std::vector<T,A>* push(std::vector<T,A>* elt);
	void print();
};

#endif //CIRCULAR_BUFFER_HPP
