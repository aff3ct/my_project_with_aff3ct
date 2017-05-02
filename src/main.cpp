#include <iostream>

// AFF3CT header
#include <aff3ct.hpp>

int main(int argc, char** argv)
{
	constexpr int    size  = 16;
	constexpr double sigma = 1.f;
	
	// buffers
	mipp::vector<int  > refs (size);
	mipp::vector<int  > bits (size);
	mipp::vector<float> symbs(size);
	mipp::vector<float> LLRs (size);

	// AFF3CT objects
	aff3ct::module::Source_random<>        source   (size       );
	aff3ct::module::Encoder_NO<>           encoder  (size, size );
	aff3ct::module::Modulator_BPSK<>       modulator(size, sigma);
	aff3ct::module::Channel_AWGN_std_LLR<> channel  (size, sigma);
	aff3ct::module::Decoder_NO<>           decoder  (size, size );
	aff3ct::tools ::Frame_trace<>          trace    (size       );

	// run a small simulation chain
	source   .generate   (       refs );
	encoder  .encode     (refs,  bits );
	modulator.modulate   (bits,  symbs);
	channel  .add_noise  (symbs, LLRs );
	modulator.demodulate (LLRs,  LLRs );
	decoder  .hard_decode(LLRs,  bits );

	// display the resulting bits
	std::cout << "AFF3CT Hello World!" << std::endl;

	std::cout << "Input bits:" << std::endl;
	trace.display_bit_vector(refs);

	std::cout << "Output bits:" << std::endl;
	trace.display_bit_vector(bits, refs);

	return 0;
}