#ifndef SPLITTER_HPP_
#define SPLITTER_HPP_

#include <vector>
#include <memory>

#include <Module/Module.hpp>

namespace aff3ct
{
namespace module
{
	namespace spl
	{
		enum class tsk : size_t { split, SIZE };

		namespace sck
		{
			enum class split : size_t { U_K, V_K1, V_K2, SIZE };
		}
	}

template <typename B = int>
class Splitter : public Module
{
public:
	inline Task&   operator[](const spl::tsk        t);
	inline Socket& operator[](const spl::sck::split s);

protected:
	const int K;

public:
	Splitter(const int K, const int n_frames = 1);

	virtual ~Splitter() = default;

	virtual int get_K() const;

	template <class A = std::allocator<B>>
	void split(std::vector<B,A>& U_K, std::vector<B,A>& V_K1, std::vector<B,A>& V_K2, const int frame_id = -1);

	virtual void split(B *U_K, B *V_K1, B *V_K2, const int frame_id = -1);

protected:
	void _split(B *U_K, B *V_K1, B *V_K2, const int frame_id);
};
}
}

#include "Splitter.hxx"

#endif /* SPLITTER_HPP_ */
