#include <type_traits>
#include <functional>
#include <exception>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <random>

#include <aff3ct.hpp>
using namespace aff3ct;
using namespace aff3ct::module;

namespace aff3ct { namespace tools {
using Monitor_BFER_reduction = Monitor_reduction<module::Monitor_BFER<>>;
} }

int main(int argc, char** argv)
{
	// get the AFF3CT version
	const std::string v = "v" + std::to_string(tools::version_major()) + "." +
	                            std::to_string(tools::version_minor()) + "." +
	                            std::to_string(tools::version_release());

	std::cout << "#----------------------------------------------------------"      << std::endl;
	std::cout << "# This is a basic program using the AFF3CT library (" << v << ")" << std::endl;
	std::cout << "# Feel free to improve it as you want to fit your needs."         << std::endl;
	std::cout << "#----------------------------------------------------------"      << std::endl;
	std::cout << "#"                                                                << std::endl;

	unsigned K = 1000;
	unsigned N_ = 2 * K +4;
	unsigned N = 2 * N_ +4;
	unsigned I = 4;
	unsigned FE = 1000;
	unsigned nthreads = std::thread::hardware_concurrency();

	float R = (K * 1.f) / (N * 1.f);
	float ebn0_min = 2.f;
	float ebn0_max = 5.01f;
	float ebn0_step = 0.25f;

	Source_random_fast<> src(K, 12);

	// Build DVBS-RCS2 Turbo encoder.
	Encoder_RSC_generic_sys<> enc_n(K, N_);
	Encoder_RSC_generic_sys<> enc_i(N_,N);

	// Build DVBS-RCS2 Interleaver.
	tools::Interleaver_core_random<> itl_core(N_);
	Interleaver<int32_t> itl_bit(itl_core);
	Interleaver<float> itl_llr(itl_core);

	// Build DVBS-RCS2 Trubo decoder.
	auto trellis_n = enc_n.get_trellis();
	auto trellis_i = enc_i.get_trellis();

	Decoder_RSC_BCJR_seq_generic_std<> dec_n(K,trellis_n);
	dec_n.set_custom_name("dec_n");
	Decoder_RSC_BCJR_seq_generic_std<> dec_i(N_,trellis_i);
	dec_i.set_custom_name("dec_i");

	Modem_BPSK_fast<> mdm(N);
	Extractor_RSC<> ext(N_, N, (N-N_)/2);
	Channel_AWGN_LLR<> chn(N, aff3ct::tools::Gaussian_noise_generator_implem::FAST);
	Switcher swi(2, N_, typeid(float));

	Iterator cnt(I);
	Initializer<float> zeros(N_);
	zeros.set_init_data(std::vector<float>(N_, 0.f));

	Monitor_BFER<> mnt(K, FE);

	std::vector<float> sigma(1);

	enc_n  [enc::sck::encode             ::U_K  ] = src    [src::sck::generate           ::U_K   ];
	itl_bit[itl::sck::interleave         ::nat  ] = enc_n  [enc::sck::encode             ::X_N   ];
	enc_i  [enc::sck::encode             ::U_K  ] = itl_bit[itl::sck::interleave         ::itl   ];
	mdm    [mdm::sck::modulate           ::X_N1 ] = enc_i  [enc::sck::encode             ::X_N   ];
	chn    [chn::sck::add_noise          ::X_N  ] = mdm    [mdm::sck::modulate           ::X_N2  ];
	mdm    [mdm::sck::demodulate         ::Y_N1 ] = chn    [chn::sck::add_noise          ::Y_N   ];
	zeros  [ini::tsk::initialize                ] = mdm    [mdm::sck::demodulate         ::Y_N2  ];
	swi    [swi::tsk::select             ][1    ] = zeros  [ini::sck::initialize         ::out   ];
	ext    [ext::sck::add_sys_and_ext_llr::ext  ] = swi    [swi::tsk::select             ][2     ];
	ext    [ext::sck::add_sys_and_ext_llr::Y_N1 ] = mdm    [mdm::sck::demodulate         ::Y_N2  ];
	dec_i  [dec::sck::decode_siso        ::Y_N1 ] = ext    [ext::sck::add_sys_and_ext_llr::Y_N2  ];
	ext    [ext::sck::get_sys_llr        ::Y_N  ] = dec_i  [dec::sck::decode_siso        ::Y_N2  ];
	itl_llr[itl::sck::deinterleave       ::itl  ] = ext    [ext::sck::get_sys_llr        ::Y_K   ];
	swi    [swi::tsk::commute            ][1    ] = cnt    [ite::sck::iterate            ::out   ];
	swi    [swi::tsk::commute            ][0    ] = itl_llr[itl::sck::deinterleave       ::nat   ];
	dec_n  [dec::sck::decode_siso        ::Y_N1 ] = swi    [swi::tsk::commute            ][2     ];
	itl_llr[itl::sck::interleave         ::nat  ] = dec_n  [dec::sck::decode_siso        ::Y_N2  ];
	swi    [swi::tsk::select             ][0    ] = itl_llr[itl::sck::interleave         ::itl   ];
	dec_n  [dec::sck::decode_siho        ::Y_N  ] = swi    [swi::tsk::commute            ][3     ];
	mnt    [mnt::sck::check_errors       ::U    ] = src    [src::sck::generate           ::U_K   ];
	mnt    [mnt::sck::check_errors       ::V    ] = dec_n  [dec::sck::decode_siho        ::V_K   ];
	cnt    [ite::tsk::iterate                   ] = ext    [ext::sck::add_sys_and_ext_llr::status];
	chn    [chn::sck::add_noise          ::CP   ] = sigma;
	mdm    [mdm::sck::demodulate         ::CP   ] = sigma;

	tools::Sequence sequence(src[src::tsk::generate], nthreads);

	// std::ofstream sequence_dot("sequence.dot");
	// sequence.export_dot(sequence_dot);
	// sequence.set_no_copy_mode(false);

	// configuration of the sequence tasks
	for (auto& mod : sequence.get_modules<module::Module>(false))
		for (auto& tsk : mod->tasks)
		{
			tsk->set_debug      (false); // disable the debug mode
			tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
			tsk->set_stats      (true ); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			if (!tsk->is_debug() && !tsk->is_stats())
				tsk->set_fast(true);
		}

	tools::Monitor_BFER_reduction monitor_red(sequence.get_modules<aff3ct::module::Monitor_BFER<>>());
	tools::Sigma<> noise;
	tools::Reporter_noise<> rep_noise(noise, true);
	tools::Reporter_BFER<> rep_bfer(monitor_red);
	tools::Reporter_throughput<> rep_thr(monitor_red);
	tools::Terminal_std terminal({&rep_noise, &rep_bfer, &rep_thr});

	// set different seeds in the modules that uses PRNG
	std::mt19937 prng;
	for (auto &m : sequence.get_modules<tools::Interface_set_seed>())
		m->set_seed(prng());

	// display the legend in the terminal
	terminal.legend();

	// loop over the various SNRs
	for (auto ebn0 = ebn0_min; ebn0 < ebn0_max; ebn0 += ebn0_step)
	{
		// compute the current sigma for the channel noise
		const auto esn0 = tools::ebn0_to_esn0(ebn0, R, 1);
		std::fill(sigma.begin(), sigma.end(), tools::esn0_to_sigma(esn0, 1));

		noise.set_values(sigma[0], ebn0, esn0);

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal.start_temp_report();

		// execute the simulation sequence (multi-threaded)
		sequence.exec([&monitor_red, &terminal]() { return monitor_red.is_done() || terminal.is_interrupt(); });

		// final reduction
		monitor_red.reduce();

		// display the performance (BER and FER) in the terminal
		terminal.final_report();

		// reset the monitor and the terminal for the next SNR
		monitor_red.reset();
		terminal.reset();

		// if user pressed Ctrl+c twice, exit the SNRs loop
		if (terminal.is_over()) break;
	}

	// display the statistics of the tasks (if enabled)
	std::cout << "#" << std::endl;
	tools::Stats::show(sequence.get_modules_per_types(), true);
	std::cout << "# End of the simulation" << std::endl;

	return 0;
}