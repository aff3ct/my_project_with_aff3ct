#include <iostream>

// AFF3CT header
#include <aff3ct.hpp>

int main(int argc, char** argv)
{
	std::cout << "-------------------------------------------------------" << std::endl;
	std::cout << " This is a basic program using the AFF3CT library."      << std::endl;
	std::cout << " Feel free to improve it as you want to fit your needs." << std::endl;
	std::cout << "-------------------------------------------------------" << std::endl;
	std::cout <<                                                              std::endl;
	
	const int   K        = 16;    // frame size
	const int   fe       = 100;   // frame errors
	const float ebn0_min = 1.00f; // dB
	const float ebn0_max = 10.1f; // dB

	auto ebn0  = ebn0_min;  
	auto esn0  = aff3ct::tools::ebn0_to_esn0 (ebn0);
	auto sigma = aff3ct::tools::esn0_to_sigma(esn0);
	
	// buffers to store the data
	mipp::vector<int  > refs (K);
	mipp::vector<int  > bits (K);
	mipp::vector<float> symbs(K);
	mipp::vector<float> LLRs (K);

	// create AFF3CT objects
	aff3ct::module::Source_random<>    source  (K                       );
	aff3ct::module::Encoder_NO<>       encoder (K                       );
	aff3ct::module::Modem_BPSK<>       modem   (K, sigma                );
	aff3ct::module::Channel_AWGN_LLR<> channel (K, sigma                );
	aff3ct::module::Decoder_NO<>       decoder (K                       );
	aff3ct::module::Monitor_std<>      monitor (K, fe                   );
	aff3ct::tools ::Terminal_BFER<>    terminal(K, monitor, &esn0, &ebn0);

	// display the legend in the terminal
	terminal.legend();

	// a loop over the various SNRs
	for (ebn0 = ebn0_min; ebn0 < ebn0_max; ebn0 += 1.f)
	{
		// compute the current sigma for the channel noise
		esn0  = aff3ct::tools::ebn0_to_esn0 (ebn0);
		sigma = aff3ct::tools::esn0_to_sigma(esn0);

		// update the sigma of the modem and the channel
		modem  .set_sigma(sigma);
		channel.set_sigma(sigma);

		// run a small simulation chain
		while (!monitor.fe_limit_achieved())
		{
			source .generate    (       refs );
			encoder.encode      (refs,  bits );
			modem  .modulate    (bits,  symbs);
			channel.add_noise   (symbs, LLRs );
			modem  .demodulate  (LLRs,  LLRs );
			decoder.hard_decode (LLRs,  bits );
			monitor.check_errors(bits,  refs );
		}

		// diplay the performance (BER and FER) in the terminal
		terminal.final_report();

		// reset the monitor for the next SNR
		monitor.reset();
	}

	return 0;
}