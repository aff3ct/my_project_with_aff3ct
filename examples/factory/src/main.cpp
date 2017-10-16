#include <vector>
#include <cassert>
#include <iostream>
#include <aff3ct.hpp>

constexpr float ebn0_min = 0.00f;
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
	aff3ct::factory::arg_map req_args, opt_args;
	for (auto *p : params)
		p->get_description(req_args, opt_args);

	// parse the argument from the command line and store them in the parameter objects
	aff3ct::tools::Arguments_reader areader(argc, (const char**)argv);

	if (areader.parse_arguments(req_args, opt_args))
		for (auto *p : params)
			p->store(areader.get_args());
	else // if some required arguments are missing, display the help and exit the code
	{
		// create groups of arguments
		aff3ct::factory::arg_grp grps;
		for (auto *p : params)
		{
			auto prefixes = p->get_prefixes();
			auto short_names = p->get_short_names();
			assert(prefixes.size() == short_names.size());

			for (size_t i = 0; i < prefixes.size(); i++)
				grps.push_back({prefixes[i], short_names[i] + " parameter(s)"});
		}

		// display the command usage and the help (the parameters are ordered by group)
		areader.print_usage(grps);

		// exit the program here
		return 0;
	}

	// display the headers (= print the AFF3CT parameters on the screen)
	std::cout << "#-------------------------------------------------------" << std::endl;
	std::cout << "# This is a basic program using the AFF3CT library."      << std::endl;
	std::cout << "# Feel free to improve it as you want to fit your needs." << std::endl;
	std::cout << "#-------------------------------------------------------" << std::endl;
	std::cout << "#"                                                        << std::endl;
	for (auto *p : params)
	{
		std::map<std::string,aff3ct::factory::header_list> headers;
		p->get_headers(headers, true);

		auto prefixes = p->get_prefixes();
		auto short_names = p->get_short_names();
		assert(prefixes.size() == short_names.size());

		for (size_t i = 0; i < prefixes.size(); i++)
			if (headers[prefixes[i]].size())
				aff3ct::factory::Header::print_parameters(prefixes[i], short_names[i], headers[prefixes[i]], 25);
	}
	std::cout << "#" << std::endl;

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

	// a loop over the various SNRs
	const float R = (float)p_cdc.enc->K / (float)p_cdc.enc->N_cw;
	for (auto ebn0 = ebn0_min; ebn0 < ebn0_max; ebn0 += 1.f)
	{
		// compute the current sigma for the channel noise
		const auto esn0  = aff3ct::tools::ebn0_to_esn0 (ebn0, R);
		const auto sigma = aff3ct::tools::esn0_to_sigma(esn0   );

		// give the current SNR to the terminal
		terminal->set_esn0(esn0);
		terminal->set_ebn0(ebn0);

		// update the sigma of the modem and the channel
		codec  ->set_sigma(sigma);
		modem  ->set_sigma(sigma);
		channel->set_sigma(sigma);

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal->start_temp_report();

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
	delete source;
	delete modem;
	delete channel;
	delete monitor;
	delete codec;
	delete terminal;

	std::cout << "# End of the simulation" << std::endl;

	return 0;
}
