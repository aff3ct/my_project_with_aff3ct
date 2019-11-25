#include <string>
#include <sstream>
#include <algorithm>

#include <Tools/Exception/exception.hpp>

#include "Splitter.hpp"

namespace aff3ct
{
namespace module
{

template <typename B>
Task& Splitter<B>
::operator[](const spl::tsk t)
{
	return Module::operator[]((size_t)t);
}

template <typename B>
Socket& Splitter<B>
::operator[](const spl::sck::split s)
{
	return Module::operator[]((size_t)spl::tsk::split)[(size_t)s];
}

template <typename B>
Splitter<B>
::Splitter(const int K, const int n_frames)
: Module(n_frames), K(K)
{
	const std::string name = "Splitter";
	this->set_name(name);
	this->set_short_name(name);

	if (K <= 0)
	{
		std::stringstream message;
		message << "'K' has to be greater than 0 ('K' = " << K << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p = this->create_task("split");
	auto ps_U_K  = this->template create_socket_in <B>(p, "U_K" , this->K);
	auto ps_V_K1 = this->template create_socket_out<B>(p, "V_K1", this->K);
	auto ps_V_K2 = this->template create_socket_out<B>(p, "V_K2", this->K);
	this->create_codelet(p, [ps_U_K, ps_V_K1, ps_V_K2](Module &m, Task &t) -> int
	{
		static_cast<Splitter<B>&>(m).split(static_cast<B*>(t[ps_U_K ].get_dataptr()),
		                                   static_cast<B*>(t[ps_V_K1].get_dataptr()),
		                                   static_cast<B*>(t[ps_V_K2].get_dataptr()));

		return 0;
	});
}

template <typename B>
int Splitter<B>
::get_K() const
{
	return K;
}

template <typename B>
template <class A>
void Splitter<B>
::split(std::vector<B,A>& U_K, std::vector<B,A>& V_K1, std::vector<B,A>& V_K2, const int frame_id)
{
	if (this->K * this->n_frames != (int)U_K.size())
	{
		std::stringstream message;
		message << "'U_K.size()' has to be equal to 'K' * 'n_frames' ('U_K.size()' = " << U_K.size()
		        << ", 'K' = " << this->K << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}
	if (this->K * this->n_frames != (int)V_K1.size())
	{
		std::stringstream message;
		message << "'V_K1.size()' has to be equal to 'K' * 'n_frames' ('V_K1.size()' = " << V_K1.size()
		        << ", 'K' = " << this->K << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}
	if (this->K * this->n_frames != (int)V_K2.size())
	{
		std::stringstream message;
		message << "'V_K2.size()' has to be equal to 'K' * 'n_frames' ('V_K2.size()' = " << V_K2.size()
		        << ", 'K' = " << this->K << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}
	this->split(U_K.data(), V_K1.data(), V_K2.data(), frame_id);
}

template <typename B>
void Splitter<B>
::split(B *U_K, B *V_K1, B *V_K2, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_split(U_K + f * this->K, V_K1 + f * this->K, V_K2 + f * this->K, f);
}

template <typename B>
void Splitter<B>
::_split(B *U_K, B *V_K1, B *V_K2, const int frame_id)
{
	std::copy(U_K, U_K + this->K, V_K1);
	std::copy(U_K, U_K + this->K, V_K2);
}

}
}
