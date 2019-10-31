#ifndef Destination_NO_HPP_
#define Destination_NO_HPP_

#include <random>
#include <vector>
#include <mipp.h>

#include "../Destination.hpp"

namespace aff3ct
{
namespace module
{
template <typename B = int>
class Destination_NO : public Destination<B>
{
public:
	Destination_NO(const int K, const int n_frames = 1);

	virtual ~Destination_NO();

protected:
	void _consume(B *U_K, const int frame_id);
};
}
}

#endif /* Destination_NO_HPP_ */
