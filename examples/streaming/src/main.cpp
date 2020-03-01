#include <type_traits>
#include <functional>
#include <exception>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <random>

#include <aff3ct.hpp>
using namespace aff3ct;

#define MULTI_THREADED

struct params
{
#ifdef MULTI_THREADED
	size_t n_threads = std::thread::hardware_concurrency();
#else
	size_t n_threads = 1;
#endif
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
#ifdef MULTI_THREADED
	std::unique_ptr<module::Adaptor_1_to_n> adaptor_1_to_n;
	std::unique_ptr<module::Adaptor_n_to_1> adaptor_n_to_1;
#endif
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
	std::vector<std::unique_ptr<tools::Chain>   > chains;
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

#ifdef MULTI_THREADED
	(*m.adaptor_1_to_n)[adp::sck::push_1      ::in1 ].bind((*m.source        )[src::sck::generate   ::U_K ]);
	(*m.encoder       )[enc::sck::encode      ::U_K ].bind((*m.adaptor_1_to_n)[adp::sck::pull_n     ::out1]);
	(*m.adaptor_n_to_1)[adp::sck::push_n      ::in1 ].bind((*m.adaptor_1_to_n)[adp::sck::pull_n     ::out1]);
	(*m.modem         )[mdm::sck::modulate    ::X_N1].bind((*m.encoder       )[enc::sck::encode     ::X_N ]);
	(*m.channel       )[chn::sck::add_noise   ::X_N ].bind((*m.modem         )[mdm::sck::modulate   ::X_N2]);
	(*m.modem         )[mdm::sck::demodulate  ::Y_N1].bind((*m.channel       )[chn::sck::add_noise  ::Y_N ]);
	(*m.decoder       )[dec::sck::decode_siho ::Y_N ].bind((*m.modem         )[mdm::sck::demodulate ::Y_N2]);
	(*m.adaptor_n_to_1)[adp::sck::push_n      ::in2 ].bind((*m.decoder       )[dec::sck::decode_siho::V_K ]);
	(*m.monitor       )[mnt::sck::check_errors::U   ].bind((*m.adaptor_n_to_1)[adp::sck::pull_1     ::out1]);
	(*m.monitor       )[mnt::sck::check_errors::V   ].bind((*m.adaptor_n_to_1)[adp::sck::pull_1     ::out2]);
	(*m.sink          )[snk::sck::send        ::V   ].bind((*m.adaptor_n_to_1)[adp::sck::pull_1     ::out2]);
#else
	(*m.encoder)[enc::sck::encode      ::U_K ].bind((*m.source )[src::sck::generate   ::U_K ]);
	(*m.modem  )[mdm::sck::modulate    ::X_N1].bind((*m.encoder)[enc::sck::encode     ::X_N ]);
	(*m.channel)[chn::sck::add_noise   ::X_N ].bind((*m.modem  )[mdm::sck::modulate   ::X_N2]);
	(*m.modem  )[mdm::sck::demodulate  ::Y_N1].bind((*m.channel)[chn::sck::add_noise  ::Y_N ]);
	(*m.decoder)[dec::sck::decode_siho ::Y_N ].bind((*m.modem  )[mdm::sck::demodulate ::Y_N2]);
	(*m.monitor)[mnt::sck::check_errors::U   ].bind((*m.source )[src::sck::generate   ::U_K ]);
	(*m.monitor)[mnt::sck::check_errors::V   ].bind((*m.decoder)[dec::sck::decode_siho::V_K ]);
	(*m.sink   )[snk::sck::send        ::V   ].bind((*m.decoder)[dec::sck::decode_siho::V_K ]);
#endif

	utils u; init_utils(p, m, u); // create and initialize the utils

	// set the noise
	m.codec->set_noise(*u.noise);
	for (auto &chain : u.chains)
		for (auto &m : chain->get_modules<tools::Interface_get_set_noise>())
			m->set_noise(*u.noise);

	// registering to noise updates
	u.noise->record_callback_update([&m](){ m.codec->notify_noise_update(); });
	for (auto &chain : u.chains)
		for (auto &m : chain->get_modules<tools::Interface_notify_noise_update>())
			u.noise->record_callback_update([m](){ m->notify_noise_update(); });

	// set different seeds in the modules that uses PRNG
	std::mt19937 prng;
	for (auto &chain : u.chains)
		for (auto &m : chain->get_modules<tools::Interface_set_seed>())
			m->set_seed(prng());

	auto stop_threads = [&u]()
	{
		for (auto &chain : u.chains)
			for (auto &m : chain->get_modules<tools::Interface_waiting>())
				m->cancel_waiting();
	};

	// compute the current sigma for the channel noise
	const auto esn0  = tools::ebn0_to_esn0 (p.ebn0, p.R, p.modem->bps);
	const auto sigma = tools::esn0_to_sigma(esn0, p.modem->cpm_upf );

	u.noise->set_values(sigma, p.ebn0, esn0);

	// display the performance (BER and FER) in real time (in a separate thread)
	u.terminal->legend();
	u.terminal->start_temp_report();

	std::vector<std::thread> threads;
	for (auto &chain : u.chains)
		threads.push_back(std::thread([&chain, &u, &m, &stop_threads](){
			chain->exec([&u, &m]()
			{
				return m.source->is_over() || u.terminal->is_interrupt();
			});
			stop_threads();
		}));

	for (auto &t : threads)
		t.join();

	// display the performance (BER and FER) in the terminal
	u.terminal->final_report();

	// display the statistics of the tasks (if enabled)
	for (size_t c = 0; c < u.chains.size(); c++)
	{
		const int n_threads = u.chains[c]->get_n_threads();
		std::cout << "#" << std::endl << "# Chain stage " << c << " (" << n_threads << " thread(s)): " << std::endl;
		tools::Stats::show(u.chains[c]->get_tasks_per_types(), true);
		std::cout << "#" << std::endl;
	}
	std::cout << "# End of the simulation" << std::endl;

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

#ifdef MULTI_THREADED
	m.adaptor_1_to_n = std::unique_ptr<module::Adaptor_1_to_n>(new module::Adaptor_1_to_n(p.codec->enc->K,
	                                                                                      typeid(int),
	                                                                                      1024,
	                                                                                      false,
	                                                                                      p.source->n_frames));

	m.adaptor_n_to_1 = std::unique_ptr<module::Adaptor_n_to_1>(new module::Adaptor_n_to_1({(size_t)p.codec->enc->K, (size_t)p.codec->enc->K},
	                                                                                      {typeid(int), typeid(int)},
	                                                                                      1024,
	                                                                                      false,
	                                                                                      p.source->n_frames));

	m.adaptor_1_to_n->set_custom_name("Adp_1_to_n");
	m.adaptor_n_to_1->set_custom_name("Adp_n_to_1");
#endif
}

void init_utils(const params &p, const modules &m, utils &u)
{
#ifdef MULTI_THREADED
	u.chains.resize(3);
	u.chains[0].reset(new tools::Chain((*m.source        )[module::src::tsk::generate],                             1));
	u.chains[1].reset(new tools::Chain((*m.adaptor_1_to_n)[module::adp::tsk::pull_n  ], p.n_threads ? p.n_threads : 1));
	u.chains[2].reset(new tools::Chain((*m.adaptor_n_to_1)[module::adp::tsk::pull_1  ],                             1));
#else
	u.chains.resize(1);
	u.chains[0].reset(new tools::Chain((*m.source)[module::src::tsk::generate], p.n_threads ? p.n_threads : 1));
#endif

	for (size_t c = 0; c < u.chains.size(); c++)
	{
		std::ofstream f("chain_stage_" + std::to_string(c) + ".dot");
		u.chains[c]->export_dot(f);
	}

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

	// configuration of the chain tasks
	for (auto &chain : u.chains)
		for (auto& type : chain->get_tasks_per_types()) for (auto& tsk : type)
		{
			tsk->set_debug      (false); // disable the debug mode
			tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
			tsk->set_stats      (true ); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			if (!tsk->is_debug() && !tsk->is_stats())
				tsk->set_fast(true);
		}
}