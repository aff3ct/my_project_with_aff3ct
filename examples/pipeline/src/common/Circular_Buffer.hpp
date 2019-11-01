#ifndef CIRCULAR_BUFFER_HPP
#define CIRCULAR_BUFFER_HPP

#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>

template <typename T, class A = std::allocator<T>>
class Circular_Buffer
{
protected:
	size_t max_buffer_nbr;
	size_t elt_per_buffer;
	size_t head_buffer;
	size_t tail_buffer;

private:
	size_t                         cb_size;
	std::vector<std::vector<T,A>*> circular_buffer;
	std::mutex                     wait_lock;
	std::vector<std::mutex>        lock;
	std::condition_variable        cond;
	bool                           stop_signal;

public:
	Circular_Buffer(size_t max_buffer_nbr = 0, size_t elt_per_buffer = 0);

	virtual ~Circular_Buffer();

	inline size_t get_max_buffer_nbr() const;
	inline size_t get_cur_buffer_nbr() const;

	void stop      (                      );
	bool pop       (std::vector<T,A>** elt);
	bool push      (std::vector<T,A>** elt);
	void wait_pop  (std::vector<T,A>** elt);
	void wait_push (std::vector<T,A>** elt);

	inline bool is_full () const;
	inline bool is_empty() const;

	void print() const;
	void reset();
};

#include "Circular_Buffer.hxx"

#endif //CIRCULAR_BUFFER_HPP
