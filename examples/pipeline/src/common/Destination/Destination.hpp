#ifndef DESTINATION_HPP_
#define DESTINATION_HPP_

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
	namespace dst
	{
		namespace tsk
		{
			enum list { consume, SIZE };
		}

		namespace sck
		{
			namespace consume { enum list { U_K, SIZE }; }
		}
	}

/*!
 * \class Destination
 *
 * \brief Consumes a message.
 *
 * \tparam B: type of the bits in the Destination.
 *
 * Please use Destination for inheritance (instead of Destination).
 */
template <typename B = int>
class Destination : public Module
{
protected:
	const int K; /*!< Number of information bits in one frame */

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param K:        number of information bits in the frame.
	 * \param n_frames: number of frames to process in the Destination.
	 * \param name:     Destination's name.
	 */
	Destination(const int K, const int n_frames = 1)
	: Module(n_frames), K(K)
	{
		const std::string name = "Destination";
		this->set_name(name);
		this->set_short_name(name);

		if (K <= 0)
		{
			std::stringstream message;
			message << "'K' has to be greater than 0 ('K' = " << K << ").";
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
		}

		auto &p = this->create_task("consume");
		auto ps_U_K = this->template create_socket_in<B>(p, "U_K", this->K * this->n_frames);
		this->create_codelet(p, [this, &ps_U_K](Task &t) -> int
		{
			this->consume(static_cast<B*>(t[ps_U_K].get_dataptr()));

			return 0;
		});
	}

	/*!
	 * \brief Destructor.
	 */
	virtual ~Destination()
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
	void consume(std::vector<B,A>& U_K, const int frame_id = -1)
	{
		if (this->K * this->n_frames != (int)U_K.size())
		{
			std::stringstream message;
			message << "'U_K.size()' has to be equal to 'K' * 'n_frames' ('U_K.size()' = " << U_K.size()
			        << ", 'K' = " << this->K << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		this->consume(U_K.data(), frame_id);
	}

	virtual void consume(B *U_K, const int frame_id = -1)
	{
		const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
		const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

		for (auto f = f_start; f < f_stop; f++)
			this->_consume(U_K + f * this->K, f);
	}

protected:
	virtual void _consume(B *U_K, const int frame_id)
	{
		throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
	}
};
}
}

#endif /* DESTINATION_HPP_ */
