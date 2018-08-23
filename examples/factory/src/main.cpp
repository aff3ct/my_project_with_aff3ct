#include <vector>
#include <cassert>
#include <memory>
#include <iostream>
#include <aff3ct.hpp>

constexpr float ebn0_min =  0.0f;
constexpr float ebn0_max = 10.1f;

int main(int argc, char** argv)
{
	// declare the parameters objects
	aff3ct::factory::Source          ::parameters p_src;
	aff3ct::factory::Codec_repetition::parameters p_cdc;
	aff3ct::factory::Modem           ::parameters p_mdm;
	aff3ct::factory::Channel         ::parameters p_chn;
	aff3ct::factory::Monitor_BFER    ::parameters p_mnt;
	aff3ct::factory::Terminal        ::parameters p_ter;

	std::vector<aff3ct::factory::Factory::parameters*> params = {&p_src, &p_cdc, &p_mdm, &p_chn, &p_mnt, &p_ter};

	aff3ct::factory::Command_parser cp(argc, argv, params, true);

	// parse the command for the given parameters and fill them
	if (cp.parsing_failed())
	{
		cp.print_help    ();
		cp.print_warnings();
		cp.print_errors  ();

		return EXIT_FAILURE;
	}

	// display the headers (= print the AFF3CT parameters on the screen)
	std::cout << "#-------------------------------------------------------" << std::endl;
	std::cout << "# This is a basic program using the AFF3CT library."      << std::endl;
	std::cout << "# Feel free to improve it as you want to fit your needs." << std::endl;
	std::cout << "#-------------------------------------------------------" << std::endl;
	std::cout << "#"                                                        << std::endl;
	aff3ct::factory::Header::print_parameters(params);
	std::cout << "#" << std::endl;

	cp.print_warnings();

	// create the AFF3CT modules
	auto* source  = p_src.build();
	auto* modem   = p_mdm.build();
	auto* channel = p_chn.build();
	auto* monitor = p_mnt.build();
	auto* codec   = p_cdc.build();
	auto* encoder = codec->get_encoder();
	auto* decoder = codec->get_decoder_siho();

	// create reporters to display results in terminal
	aff3ct::tools::Sigma<float>                  noise;
	aff3ct::tools::Reporter_noise<float>         rep_noise(noise   ); // reporter of the noise value
	aff3ct::tools::Reporter_BFER <int>           rep_er   (*monitor); // reporter of the bit/frame error rate
	aff3ct::tools::Reporter_throughput<uint64_t> rep_thr  (*monitor); // reporter of the throughput of the simulation
	std::vector<aff3ct::tools::Reporter*> reporters{&rep_noise, &rep_er, &rep_thr};

	// create a Terminal and display the legend
	auto *terminal = p_ter.build(reporters);
	terminal->legend();

	// configuration of the module tasks
	std::vector<const aff3ct::module::Module*> modules = {source, encoder, modem, channel, decoder, monitor};
	for (auto *m : modules)
		for (auto *t : m->tasks)
		{
			t->set_autoalloc  (true ); // enable the automatic allocation of the data in the tasks
			t->set_autoexec   (false); // disable the auto execution mode of the tasks
			t->set_debug      (false); // disable the debug mode
			t->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
			t->set_stats      (false); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			t->set_fast(!t->is_debug() && !t->is_stats());
		}

	// sockets binding (connect the sockets of the tasks = fill the input sockets with the output sockets)
	using namespace aff3ct::module;
	(*encoder)[enc::sck::encode      ::U_K ].bind((*source )[src::sck::generate   ::U_K ]);
	(*modem  )[mdm::sck::modulate    ::X_N1].bind((*encoder)[enc::sck::encode     ::X_N ]);
	(*channel)[chn::sck::add_noise   ::X_N ].bind((*modem  )[mdm::sck::modulate   ::X_N2]);
	(*modem  )[mdm::sck::demodulate  ::Y_N1].bind((*channel)[chn::sck::add_noise  ::Y_N ]);
	(*decoder)[dec::sck::decode_siho ::Y_N ].bind((*modem  )[mdm::sck::demodulate ::Y_N2]);
	(*monitor)[mnt::sck::check_errors::U   ].bind((*encoder)[enc::sck::encode     ::U_K ]);
	(*monitor)[mnt::sck::check_errors::V   ].bind((*decoder)[dec::sck::decode_siho::V_K ]);


	// reset the memory of the decoder after the end of each communication
	monitor->add_handler_check(std::bind(&aff3ct::module::Decoder::reset, decoder));

	// initialize the interleaver if this code use an interleaver
	try
	{
		auto interleaver = codec->get_interleaver();
		interleaver->init();
	}
	catch (const std::exception&) { /* do nothing if there is no interleaver */ }

	// a loop over the various SNRs
	const float R = (float)p_cdc.enc->K / (float)p_cdc.enc->N_cw; // compute the code rate
	for (auto ebn0 = ebn0_min; ebn0 < ebn0_max; ebn0 += 1.f)
	{
		// compute the current sigma for the channel noise
		const auto esn0  = aff3ct::tools::ebn0_to_esn0 (ebn0, R);
		const auto sigma = aff3ct::tools::esn0_to_sigma(esn0   );

		noise.set_noise(sigma, ebn0, esn0);

		// update the sigma of the modem and the channel
		codec  ->set_noise(noise);
		modem  ->set_noise(noise);
		channel->set_noise(noise);

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal->start_temp_report(p_ter.frequency);

		// run the simulation chain
		while (!monitor->fe_limit_achieved() && !aff3ct::tools::Terminal::is_interrupt())
		{
			(*source )[src::tsk::generate    ].exec();
			(*encoder)[enc::tsk::encode      ].exec();
			(*modem  )[mdm::tsk::modulate    ].exec();
			(*channel)[chn::tsk::add_noise   ].exec();
			(*modem  )[mdm::tsk::demodulate  ].exec();
			(*decoder)[dec::tsk::decode_siho ].exec();
			(*monitor)[mnt::tsk::check_errors].exec();
		}

		// display the performance (BER and FER) in the terminal
		terminal->final_report();

		if (aff3ct::tools::Terminal::is_over())
			break;

		// reset the monitor and the terminal for the next SNR
		monitor->reset();
		aff3ct::tools::Terminal::reset();
	}
	std::cout << "#" << std::endl;

	// display the statistics of the tasks (if enabled)
	auto ordered = true;
	aff3ct::tools::Stats::show(modules, ordered);

	// delete the aff3ct objects
	std::cout << "# End of the simulation" << std::endl;

	delete source; delete modem; delete channel; delete monitor; delete codec; delete terminal;

	return EXIT_SUCCESS;
}
