#include <type_traits>
#include <functional>
#include <exception>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <random>

#include <aff3ct.hpp>
using namespace aff3ct;

struct params
{
	size_t n_threads = std::thread::hardware_concurrency();
	float  ebn0      = 20.00f; // SNR value
	float  R;                  // code rate (R=K/N)

	std::unique_ptr<factory::Source          > source;
	std::unique_ptr<factory::Codec_repetition> codec;
	std::unique_ptr<factory::Modem           > modem;
	std::unique_ptr<factory::Channel         > channel;
	std::unique_ptr<factory::Monitor_BFER    > monitor;
	std::unique_ptr<factory::Sink            > sink;
	std::unique_ptr<factory::Terminal        > terminal;
};
void init_params(int argc, char** argv, params &p);

struct modules
{
	std::unique_ptr<module::Source<>>       source;
	std::unique_ptr<module::Modem<>>        modem;
	std::unique_ptr<module::Channel<>>      channel;
	std::unique_ptr<module::Monitor_BFER<>> monitor;
	std::unique_ptr<module::Sink<>>         sink;
	std::unique_ptr<tools ::Codec_SIHO<>>   codec;
	                module::Encoder<>*      encoder;
	                module::Decoder_SIHO<>* decoder;
};
void init_modules(const params &p, modules &m);

struct utils
{
	            std::unique_ptr<tools::Sigma<>  > noise;     // a sigma noise type
	std::vector<std::unique_ptr<tools::Reporter>> reporters; // list of reporters displayed in the terminal
	            std::unique_ptr<tools::Terminal > terminal;  // manage the output text in the terminal
	            std::unique_ptr<tools::Pipeline>  pipeline;
};
void init_utils(const params &p, const modules &m, utils &u);

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

	params  p; init_params (argc, argv, p); // create and initialize the parameters from the command line with factories
	modules m; init_modules(p, m         ); // create and initialize the modules

	// sockets binding (connect the sockets of the tasks = fill the input sockets with the output sockets)
	using namespace module;
	(*m.encoder)[enc::sck::encode      ::U_K ].bind((*m.source )[src::sck::generate   ::U_K ]);
	(*m.modem  )[mdm::sck::modulate    ::X_N1].bind((*m.encoder)[enc::sck::encode     ::X_N ]);
	(*m.channel)[chn::sck::add_noise   ::X_N ].bind((*m.modem  )[mdm::sck::modulate   ::X_N2]);
	(*m.modem  )[mdm::sck::demodulate  ::Y_N1].bind((*m.channel)[chn::sck::add_noise  ::Y_N ]);
	(*m.decoder)[dec::sck::decode_siho ::Y_N ].bind((*m.modem  )[mdm::sck::demodulate ::Y_N2]);
	(*m.monitor)[mnt::sck::check_errors::U   ].bind((*m.source )[src::sck::generate   ::U_K ]);
	(*m.monitor)[mnt::sck::check_errors::V   ].bind((*m.decoder)[dec::sck::decode_siho::V_K ]);
	(*m.sink   )[snk::sck::send        ::V   ].bind((*m.decoder)[dec::sck::decode_siho::V_K ]);

	std::vector<float> sigma(1);
	(*m.channel)[chn::sck::add_noise ::CP].bind(sigma);
	(*m.modem  )[mdm::sck::demodulate::CP].bind(sigma);

	utils u; init_utils(p, m, u); // create and initialize the utils

	// set the noise
	m.codec->set_noise(*u.noise);
	for (auto &m : u.pipeline->get_modules<tools::Interface_get_set_noise>())
		m->set_noise(*u.noise);

	// registering to noise updates
	u.noise->record_callback_update([&m](){ m.codec->notify_noise_update(); });
	for (auto &m : u.pipeline->get_modules<tools::Interface_notify_noise_update>())
		u.noise->record_callback_update([m](){ m->notify_noise_update(); });

	// set different seeds in the modules that uses PRNG
	std::mt19937 prng;
	for (auto &m : u.pipeline->get_modules<tools::Interface_set_seed>())
		m->set_seed(prng());

	// compute the current sigma for the channel noise
	const auto esn0 = tools::ebn0_to_esn0(p.ebn0, p.R, p.modem->bps);
	std::fill(sigma.begin(), sigma.end(), tools::esn0_to_sigma(esn0, p.modem->cpm_upf));

	u.noise->set_values(sigma[0], p.ebn0, esn0);

	// display the performance (BER and FER) in real time (in a separate thread)
	u.terminal->legend();
	u.terminal->start_temp_report();

	u.pipeline->exec([&u, &m]() { return m.source->is_over() || u.terminal->is_interrupt(); });

	// display the performance (BER and FER) in the terminal
	u.terminal->final_report();

	// display the statistics of the tasks (if enabled)
	auto stages = u.pipeline->get_stages();
	for (size_t s = 0; s < stages.size(); s++)
	{
		const int n_threads = stages[s]->get_n_threads();
		std::cout << "#" << std::endl << "# Pipeline stage " << s << " (" << n_threads << " thread(s)): " << std::endl;
		tools::Stats::show(stages[s]->get_tasks_per_types(), true);
	}
	std::cout << "#" << std::endl << "# End of the simulation" << std::endl;

	return 0;
}

void init_params(int argc, char** argv, params &p)
{
	p.source   = std::unique_ptr<factory::Source          >(new factory::Source          ());
	p.codec    = std::unique_ptr<factory::Codec_repetition>(new factory::Codec_repetition());
	p.modem    = std::unique_ptr<factory::Modem           >(new factory::Modem           ());
	p.channel  = std::unique_ptr<factory::Channel         >(new factory::Channel         ());
	p.monitor  = std::unique_ptr<factory::Monitor_BFER    >(new factory::Monitor_BFER    ());
	p.sink     = std::unique_ptr<factory::Sink            >(new factory::Sink            ());
	p.terminal = std::unique_ptr<factory::Terminal        >(new factory::Terminal        ());

	std::vector<factory::Factory*> params_list = { p.source  .get(), p.codec  .get(), p.modem.get(),
	                                               p.channel .get(), p.monitor.get(), p.sink .get(),
	                                               p.terminal.get()                                  };

	// parse the command for the given parameters and fill them
	tools::Command_parser cp(argc, argv, params_list, true);
	if (cp.parsing_failed())
	{
		cp.print_help    ();
		cp.print_warnings();
		cp.print_errors  ();
		std::exit(1);
	}

	std::cout << "# Simulation parameters: " << std::endl;
	tools::Header::print_parameters(params_list); // display the headers (= print the AFF3CT parameters on the screen)
	std::cout << "#" << std::endl;
	cp.print_warnings();

	p.R = (float)p.codec->enc->K / (float)p.codec->enc->N_cw; // compute the code rate
}

void init_modules(const params &p, modules &m)
{
	m.source  = std::unique_ptr<module::Source      <>>(p.source ->build());
	m.codec   = std::unique_ptr<tools ::Codec_SIHO  <>>(p.codec  ->build());
	m.modem   = std::unique_ptr<module::Modem       <>>(p.modem  ->build());
	m.channel = std::unique_ptr<module::Channel     <>>(p.channel->build());
	m.monitor = std::unique_ptr<module::Monitor_BFER<>>(p.monitor->build());
	m.sink    = std::unique_ptr<module::Sink        <>>(p.sink   ->build());
	m.encoder = &m.codec->get_encoder();
	m.decoder = &m.codec->get_decoder_siho();
}

void init_utils(const params &p, const modules &m, utils &u)
{
	u.pipeline.reset(new tools::Pipeline((*m.source)[module::src::tsk::generate], // first task of the sequence
	                                     { // pipeline stage 0
	                                       { { &(*m.source )[module::src::tsk::generate    ] },   // first tasks of stage 0
	                                         { &(*m.source )[module::src::tsk::generate    ] } }, // last  tasks of stage 0
	                                       // pipeline stage 1
	                                       { { &(*m.encoder)[module::enc::tsk::encode      ] },   // first tasks of stage 1
	                                         { &(*m.decoder)[module::dec::tsk::decode_siho ] } }, // last  tasks of stage 1
	                                       // pipeline stage 2
	                                       { { &(*m.monitor)[module::mnt::tsk::check_errors],     // first tasks of stage 2
	                                           &(*m.sink   )[module::snk::tsk::send        ] },
	                                         { /* empty vector of last tasks */              } }, // last  tasks of stage 2
	                                     },
	                                     {
	                                       1,                             // number of threads in the stage 0
	                                       p.n_threads ? p.n_threads : 1, // number of threads in the stage 1
	                                       1                              // number of threads in the stage 2
	                                     },
	                                     {
	                                       1024, // synchronization buffer size between stages 0 and 1
	                                       1024, // synchronization buffer size between stages 1 and 2
	                                     },
	                                     {
	                                       false, // type of waiting between stages 0 and 1 (true = active, false = passive)
	                                       false, // type of waiting between stages 1 and 2 (true = active, false = passive)
	                                     }));

	std::ofstream f("pipeline.dot");
	u.pipeline->export_dot(f);

	// create a sigma noise type
	u.noise = std::unique_ptr<tools::Sigma<>>(new tools::Sigma<>());
	// report the noise values (Es/N0 and Eb/N0)
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise<>(*u.noise)));
	// report the bit/frame error rates
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER<>(*m.monitor)));
	// report the simulation throughputs
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_throughput<>(*m.monitor)));
	// create a terminal that will display the collected data from the reporters
	u.terminal = std::unique_ptr<tools::Terminal>(p.terminal->build(u.reporters));

	// configuration of the pipeline tasks
	for (auto& type : u.pipeline->get_tasks_per_types()) for (auto& tsk : type)
	{
		tsk->set_debug      (false); // disable the debug mode
		tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
		tsk->set_stats      (true ); // enable the statistics

		// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
		if (!tsk->is_debug() && !tsk->is_stats())
			tsk->set_fast(true);
	}
}