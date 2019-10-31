#ifndef Splitter_HPP_
#define Splitter_HPP_

#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include "Tools/Exception/exception.hpp"

#include "Module/Module.hpp"

namespace aff3ct
{
namespace module
{
	namespace spl
	{
		namespace tsk
		{
			enum list { split, SIZE };
		}

		namespace sck
		{
			namespace splitter { enum list { U_K, V_K1, V_K2, SIZE }; }
		}
	}

/*!
 * \class Splitter
 *
 * \brief Do something on a message.
 *
 * \tparam B: type of the bits in the Splitter.
 *
 * Please use Splitter for inheritance (instead of Splitter).
 */
template <typename B = int>
class Splitter : public Module
{
protected:
	const int K; /*!< Number of information bits in one frame */

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param K:        number of information bits in the frame.
	 * \param n_frames: number of frames to process in the Splitter.
	 * \param name:     Splitter's name.
	 */
	Splitter(const int K, const int n_frames = 1)
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

		auto &p = this->create_task("splitter");
		auto ps_U_K  = this->template create_socket_in <B>(p, "U_K" , this->K * this->n_frames);
		auto ps_V_K1 = this->template create_socket_out<B>(p, "V_K1", this->K * this->n_frames);
		auto ps_V_K2 = this->template create_socket_out<B>(p, "V_K2", this->K * this->n_frames);
		this->create_codelet(p, [this, ps_U_K, ps_V_K1, ps_V_K2](Task &t) -> int
		{
			this->split(static_cast<B*>(t[ps_U_K].get_dataptr()), static_cast<B*>(t[ps_V_K1].get_dataptr()), static_cast<B*>(t[ps_V_K2].get_dataptr()));

			return 0;
		});
	}

	/*!
	 * \brief Destructor.
	 */
	virtual ~Splitter()
	{
	}

	virtual int get_K() const
	{
		return K;
	}

	/*!
	 * \brief Fulfills a vector with bits.
	 *
	 * \param U_K: a vector of bits to fill.
	 */
	template <class A = std::allocator<B>>
	void split(std::vector<B,A>& U_K, std::vector<B,A>& V_K1, std::vector<B,A>& V_K2, const int frame_id = -1)
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

	virtual void split(B *U_K, B *V_K1, B *V_K2, const int frame_id = -1)
	{
		const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
		const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

		for (auto f = f_start; f < f_stop; f++)
			this->_split(U_K + f * this->K, V_K1 + f * this->K, V_K2 + f * this->K, f);
	}

protected:
	void _split(B *U_K, B *V_K1, B *V_K2, const int frame_id)
	{
		std::copy(U_K, U_K + this->K, V_K1);
		std::copy(U_K, U_K + this->K, V_K2);
	}
};
}
}
#endif /* Splitter_HPP_ */
