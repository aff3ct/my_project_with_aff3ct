#include <vector>
#include <cassert>
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
	aff3ct::factory::Terminal_BFER   ::parameters p_ter;

	std::vector<aff3ct::factory::Factory::parameters*> params = {&p_src, &p_cdc, &p_mdm, &p_chn, &p_mnt, &p_ter};

	// build the required and optional arguments for the cmd line
	auto args = aff3ct::factory::Factory::get_description(params);

	// parse the argument from the command line
	aff3ct::tools::Argument_handler ahandler(argc, (const char**)argv);
	std::vector<std::string> warnings, errors;
	auto read_args = ahandler.parse_arguments(args.first, args.second, warnings, errors);
	
	// if there is blocking errors
	if (errors.size())
	{
		// create groups of arguments
		auto grps = aff3ct::factory::Factory::create_groups(params);

		// display the command usage and the help (the parameters are ordered by group)
		ahandler.print_help(args.first, args.second, grps);

		// print the errors
		for (size_t e = 0; e < errors.size(); e++)
			std::cerr << aff3ct::tools::format_error(errors[e]) << std::endl;

		// exit the program here
		return 0;
	}

	// write the parameters values in "params" from "read_args"
	aff3ct::factory::Factory::store(params, read_args);

	// display the headers (= print the AFF3CT parameters on the screen)
	std::cout << "#-------------------------------------------------------" << std::endl;
	std::cout << "# This is a basic program using the AFF3CT library."      << std::endl;
	std::cout << "# Feel free to improve it as you want to fit your needs." << std::endl;
	std::cout << "#-------------------------------------------------------" << std::endl;
	std::cout << "#"                                                        << std::endl;
	aff3ct::factory::Header::print_parameters(params);
	std::cout << "#" << std::endl;

	// print the warnings
	for (size_t w = 0; w < warnings.size(); w++)
		std::cerr << aff3ct::tools::format_warning(warnings[w]) << std::endl;

	// create the AFF3CT modules
	auto *source  = p_src.build();
	auto *modem   = p_mdm.build();
	auto *channel = p_chn.build();
	auto *monitor = p_mnt.build();
	auto *codec   = p_cdc.build();
	auto *encoder = codec->get_encoder();
	auto *decoder = codec->get_decoder_siho();

	// configuration of the module tasks
	std::vector<const aff3ct::module::Module*> modules = {source, encoder, modem, channel, decoder, monitor};
	for (auto *m : modules)
		for (auto *t : m->tasks)
		{
			t->set_autoalloc  (true ); // enable the automatic allocation of the data in the tasks
			t->set_autoexec   (false); // disable the auto execution mode of the tasks
			t->set_debug      (false); // disable the debug mode
			t->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
			t->set_stats      (true ); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			if (!t->is_debug() && !t->is_stats())
				t->set_fast(true);
		}

	// sockets binding (connect the sockets of the tasks = fill the input sockets with the output sockets)
	using namespace aff3ct::module;
	(*encoder)[enc::tsk::encode      ][enc::sck::encode      ::U_K ]((*source )[src::tsk::generate   ][src::sck::generate   ::U_K ]);
	(*modem  )[mdm::tsk::modulate    ][mdm::sck::modulate    ::X_N1]((*encoder)[enc::tsk::encode     ][enc::sck::encode     ::X_N ]);
	(*channel)[chn::tsk::add_noise   ][chn::sck::add_noise   ::X_N ]((*modem  )[mdm::tsk::modulate   ][mdm::sck::modulate   ::X_N2]);
	(*modem  )[mdm::tsk::demodulate  ][mdm::sck::demodulate  ::Y_N1]((*channel)[chn::tsk::add_noise  ][chn::sck::add_noise  ::Y_N ]);
	(*decoder)[dec::tsk::decode_siho ][dec::sck::decode_siho ::Y_N ]((*modem  )[mdm::tsk::demodulate ][mdm::sck::demodulate ::Y_N2]);
	(*monitor)[mnt::tsk::check_errors][mnt::sck::check_errors::U   ]((*encoder)[enc::tsk::encode     ][enc::sck::encode     ::U_K ]);
	(*monitor)[mnt::tsk::check_errors][mnt::sck::check_errors::V   ]((*decoder)[dec::tsk::decode_siho][dec::sck::decode_siho::V_K ]);

	// create a Terminal and display the legend
	auto *terminal = p_ter.build(*monitor);
	terminal->legend();

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
		const auto esn0  = aff3ct::tools::ebn0_to_esn0 (ebn0, R, p_mdm.bps);
		const auto sigma = aff3ct::tools::esn0_to_sigma(esn0   , p_mdm.upf);

		// give the current SNR to the terminal
		terminal->set_esn0(esn0);
		terminal->set_ebn0(ebn0);

		// update the sigma of the modem and the channel
		codec  ->set_sigma(sigma);
		modem  ->set_sigma(sigma);
		channel->set_sigma(sigma);

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal->start_temp_report(p_ter.frequency);

		// run the simulation chain
		while (!monitor->fe_limit_achieved())
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

		// reset the monitor for the next SNR
		monitor->reset();
	}
	std::cout << "#" << std::endl;

	// display the statistics of the tasks (if enabled)
	auto ordered = true;
	aff3ct::tools::Stats::show(modules, ordered);

	// delete the aff3ct objects
	delete source; delete modem; delete channel; delete monitor; delete codec; delete terminal;

	std::cout << "# End of the simulation" << std::endl;

	return 0;
}
