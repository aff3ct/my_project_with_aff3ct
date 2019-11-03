#ifndef CIRCULAR_BUFFER_HPP
#define CIRCULAR_BUFFER_HPP

#include <memory>
#include <vector>
#include <mutex>

namespace aff3ct
{
namespace tools
{
template <typename T>
class Circular_buffer
{
protected:
	size_t head_buffer;
	size_t tail_buffer;
	std::mutex mtx;

private:
	std::vector<T*> circular_buffer;
	std::vector<T*> circular_buffer_cpy;

public:
	Circular_buffer(const size_t buffer_size, const size_t n_elmts);
	virtual ~Circular_buffer();

	inline size_t get_cur_buffer_nbr() const;
	inline bool   is_full           () const;
	inline bool   is_empty          () const;

	T* try_pop (T* data_ptr);
	T* try_push(T* data_ptr);

	void reset();
};
}
}

#include "Circular_buffer.hxx"

#endif //CIRCULAR_BUFFER_HPP
