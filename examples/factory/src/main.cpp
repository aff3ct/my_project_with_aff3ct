#include <vector>
#include <memory>
#include <cassert>
#include <iostream>

#include <aff3ct.hpp>
using namespace aff3ct;

constexpr float ebn0_min =  0.0f;
constexpr float ebn0_max = 10.1f;

int main(int argc, char** argv)
{
	// declare the parameters objects
	factory::Source          ::parameters p_src;
	factory::Codec_repetition::parameters p_cdc;
	factory::Modem           ::parameters p_mdm;
	factory::Channel         ::parameters p_chn;
	factory::Monitor_BFER    ::parameters p_mnt;
	factory::Terminal        ::parameters p_ter;

	std::vector<factory::Factory::parameters*> params = {&p_src, &p_cdc, &p_mdm, &p_chn, &p_mnt, &p_ter};

	factory::Command_parser cp(argc, argv, params, true);

	// parse the command for the given parameters and fill them
	if (cp.parsing_failed())
	{
		cp.print_help    ();
		cp.print_warnings();
		cp.print_errors  ();

		return EXIT_FAILURE;
	}

	// get the AFF3CT version
	const std::string v = "v" + std::to_string(version_major()) + "." +
	                            std::to_string(version_minor()) + "." +
	                            std::to_string(version_release());

	std::cout << "#----------------------------------------------------------"      << std::endl;
	std::cout << "# This is a basic program using the AFF3CT library (" << v << ")" << std::endl;
	std::cout << "# Feel free to improve it as you want to fit your needs."         << std::endl;
	std::cout << "#----------------------------------------------------------"      << std::endl;
	std::cout << "#"                                                                << std::endl;
	std::cout << "# Simulation parameters: "                                        << std::endl;
	// display the headers (= print the AFF3CT parameters on the screen)
	factory::Header::print_parameters(params);
	std::cout << "#" << std::endl;

	cp.print_warnings();

	// create the AFF3CT modules
	std::unique_ptr<module::Source<>>           source (p_src.build());
	std::unique_ptr<module::Modem<>>            modem  (p_mdm.build());
	std::unique_ptr<module::Channel<>>          channel(p_chn.build());
	std::unique_ptr<module::Monitor_BFER<>>     monitor(p_mnt.build());
	std::unique_ptr<module::Codec_repetition<>> codec  (p_cdc.build());
	auto& encoder = codec->get_encoder();
	auto& decoder = codec->get_decoder_siho();

	// create reporters to display results in terminal
	tools::Sigma<> noise;
	std::vector<std::unique_ptr<tools::Reporter>> reporters;

	// reporter of the noise value
	auto reporter_noise = new tools::Reporter_noise<>(noise);
	reporters.push_back(std::unique_ptr<tools::Reporter_noise<>>(reporter_noise));
	// reporter of the bit/frame error rate
	auto reporter_BFER = new tools::Reporter_BFER<>(*monitor);
	reporters.push_back(std::unique_ptr<tools::Reporter_BFER<>>(reporter_BFER));
	// reporter of the throughput of the simulation
	auto reporter_thr = new tools::Reporter_throughput<>(*monitor);
	reporters.push_back(std::unique_ptr<tools::Reporter_throughput<>>(reporter_thr));

	// create a terminal that will display the collected data from the reporters
	std::unique_ptr<tools::Terminal> terminal(p_ter.build(reporters));

	// display the legend in the terminal
	terminal->legend();

	// configuration of the module tasks
	std::vector<const module::Module*> modules{source.get(), encoder.get(), modem.get(), channel.get(), decoder.get(), monitor.get()};
	for (auto& m : modules)
		for (auto& t : m->tasks)
		{
			t->set_autoalloc  (true ); // enable the automatic allocation of the data in the tasks
			t->set_autoexec   (false); // disable the auto execution mode of the tasks
			t->set_debug      (false); // disable the debug mode
			t->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
			t->set_stats      (true ); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			t->set_fast(!t->is_debug() && !t->is_stats());
		}

	// sockets binding (connect the sockets of the tasks = fill the input sockets with the output sockets)
	using namespace module;
	(*encoder)[enc::sck::encode      ::U_K ].bind((*source )[src::sck::generate   ::U_K ]);
	(*modem  )[mdm::sck::modulate    ::X_N1].bind((*encoder)[enc::sck::encode     ::X_N ]);
	(*channel)[chn::sck::add_noise   ::X_N ].bind((*modem  )[mdm::sck::modulate   ::X_N2]);
	(*modem  )[mdm::sck::demodulate  ::Y_N1].bind((*channel)[chn::sck::add_noise  ::Y_N ]);
	(*decoder)[dec::sck::decode_siho ::Y_N ].bind((*modem  )[mdm::sck::demodulate ::Y_N2]);
	(*monitor)[mnt::sck::check_errors::U   ].bind((*encoder)[enc::sck::encode     ::U_K ]);
	(*monitor)[mnt::sck::check_errors::V   ].bind((*decoder)[dec::sck::decode_siho::V_K ]);

	// reset the memory of the decoder after the end of each communication
	monitor->add_handler_check(std::bind(&module::Decoder::reset, decoder));

	// initialize the interleaver if this code use an interleaver
	try
	{
		auto& interleaver = codec->get_interleaver();
		interleaver->init();
	}
	catch (const std::exception&) { /* do nothing if there is no interleaver */ }

	// a loop over the various SNRs
	const float R = (float)p_cdc.enc->K / (float)p_cdc.enc->N_cw; // compute the code rate
	for (auto ebn0 = ebn0_min; ebn0 < ebn0_max; ebn0 += 1.f)
	{
		// compute the current sigma for the channel noise
		const auto esn0  = tools::ebn0_to_esn0 (ebn0, R);
		const auto sigma = tools::esn0_to_sigma(esn0   );

		noise.set_noise(sigma, ebn0, esn0);

		// update the sigma of the modem and the channel
		codec  ->set_noise(noise);
		modem  ->set_noise(noise);
		channel->set_noise(noise);

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal->start_temp_report(p_ter.frequency);

		// run the simulation chain
		while (!monitor->fe_limit_achieved() && !tools::Terminal::is_interrupt())
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

		if (tools::Terminal::is_over())
			break;

		// reset the monitor and the terminal for the next SNR
		monitor->reset();
		tools::Terminal::reset();
	}
	std::cout << "#" << std::endl;

	// display the statistics of the tasks (if enabled)
	auto ordered = true;
	tools::Stats::show(modules, ordered);

	// delete the aff3ct objects
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}
