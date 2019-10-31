#include "Destination_NO.hpp"

using namespace aff3ct::module;

template <typename B>
Destination_NO<B>
::Destination_NO(const int K, const int n_frames)
: Destination<B>(K, n_frames)
{
	const std::string name = "Destination_NO";
	this->set_name(name);
}

template <typename B>
Destination_NO<B>
::~Destination_NO()
{
}

template <typename B>
void Destination_NO<B>
::_consume(B *U_K, const int frame_id)
{
}

// ==================================================================================== explicit template instantiation 
#include "Tools/types.h"
#ifdef MULTI_PREC
template class aff3ct::module::Destination_NO<B_8>;
template class aff3ct::module::Destination_NO<B_16>;
template class aff3ct::module::Destination_NO<B_32>;
template class aff3ct::module::Destination_NO<B_64>;
#else
template class aff3ct::module::Destination_NO<B>;
#endif
// ==================================================================================== explicit template instantiation
