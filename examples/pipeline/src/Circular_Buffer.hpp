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
	bool full_flag;
	bool empty_flag;
	std::recursive_mutex* lock;

private:
	std::vector<std::vector<T,A> *> circular_buffer;

public:
	Circular_Buffer (size_t max_buffer_nbr = 0, size_t elt_per_buffer = 0);
	virtual ~Circular_Buffer();

	inline int get_max_buffer_nbr() const {return (int)this->max_buffer_nbr;};
	inline int get_cur_buffer_nbr() const {return (int)this->cur_buffer_nbr;};
	std::vector<T,A>*  pop(std::vector<T,A>* elt);
	std::vector<T,A>* push(std::vector<T,A>* elt);
	inline bool is_full() const {return this->full_flag;};
	inline bool is_empty() const {return this->empty_flag;};
	void print();
	void reset();
};

#endif //CIRCULAR_BUFFER_HPP
