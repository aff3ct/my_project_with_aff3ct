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

#define ADAPTOR

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
#ifdef ADAPTOR
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

namespace aff3ct { namespace tools {
using Monitor_BFER_reduction = Monitor_reduction<module::Monitor_BFER<>>;
} }

struct utils
{
	            std::unique_ptr<tools::Sigma<>               >  noise;       // a sigma noise type
	std::vector<std::unique_ptr<tools::Reporter              >> reporters;   // list of reporters displayed in the terminal
	            std::unique_ptr<tools::Terminal              >  terminal;    // manage the output text in the terminal
	            std::unique_ptr<tools::Monitor_BFER_reduction>  monitor_red; // main monitor object that reduce all the thread monitors
	            std::unique_ptr<tools::Chain                 >  chain_main;
#ifdef ADAPTOR
	            std::unique_ptr<tools::Chain                 >  chain_pre;
	            std::unique_ptr<tools::Chain                 >  chain_post;
#endif
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

#ifdef ADAPTOR
	(*m.adaptor_1_to_n)[adp::sck::push_1      ::in1 ].bind((*m.source        )[src::sck::generate   ::U_K ]);
	(*m.encoder       )[enc::sck::encode      ::U_K ].bind((*m.adaptor_1_to_n)[adp::sck::pull_n     ::out1]);
	(*m.modem         )[mdm::sck::modulate    ::X_N1].bind((*m.encoder       )[enc::sck::encode     ::X_N ]);
	(*m.channel       )[chn::sck::add_noise   ::X_N ].bind((*m.modem         )[mdm::sck::modulate   ::X_N2]);
	(*m.modem         )[mdm::sck::demodulate  ::Y_N1].bind((*m.channel       )[chn::sck::add_noise  ::Y_N ]);
	(*m.decoder       )[dec::sck::decode_siho ::Y_N ].bind((*m.modem         )[mdm::sck::demodulate ::Y_N2]);
	(*m.monitor       )[mnt::sck::check_errors::U   ].bind((*m.adaptor_1_to_n)[adp::sck::pull_n     ::out1]);
	(*m.monitor       )[mnt::sck::check_errors::V   ].bind((*m.decoder       )[dec::sck::decode_siho::V_K ]);
	(*m.adaptor_n_to_1)[adp::sck::push_n      ::in1 ].bind((*m.decoder       )[dec::sck::decode_siho::V_K ]);
	(*m.sink          )[snk::sck::send        ::V   ].bind((*m.adaptor_n_to_1)[adp::sck::pull_1     ::out1]);
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
	for (auto &m : u.chain_main->get_modules<tools::Interface_get_set_noise>())
		m->set_noise(*u.noise);

	// registering to noise updates
	u.noise->record_callback_update([&m](){ m.codec->notify_noise_update(); });
	for (auto &m : u.chain_main->get_modules<tools::Interface_notify_noise_update>())
		u.noise->record_callback_update([m](){ m->notify_noise_update(); });

	// set different seeds in the modules that uses PRNG
	std::mt19937 prng;
	for (auto &m : u.chain_main->get_modules<tools::Interface_set_seed>())
		m->set_seed(prng());

	// display the legend in the terminal
	u.terminal->legend();

#ifdef ADAPTOR
	auto stop_threads = [&u]()
	{
		for (auto &m : u.chain_pre->get_modules<tools::Interface_waiting>())
			m->cancel_waiting();
		for (auto &m : u.chain_main->get_modules<tools::Interface_waiting>())
			m->cancel_waiting();
		for (auto &m : u.chain_post->get_modules<tools::Interface_waiting>())
			m->cancel_waiting();
	};
#endif

	// compute the current sigma for the channel noise
	const auto esn0  = tools::ebn0_to_esn0 (p.ebn0, p.R, p.modem->bps);
	const auto sigma = tools::esn0_to_sigma(esn0, p.modem->cpm_upf );

	u.noise->set_values(sigma, p.ebn0, esn0);

	// display the performance (BER and FER) in real time (in a separate thread)
	u.terminal->start_temp_report();

#ifdef ADAPTOR
	std::vector<std::thread> threads;
	threads.push_back(std::thread([&u, &m, &stop_threads](){
		u.chain_pre->exec([&u, &m]()
		{
			return m.source->is_over() || u.terminal->is_interrupt();
		});
		stop_threads();
	}));

	threads.push_back(std::thread([&u, &m, &stop_threads](){
		u.chain_post->exec([&u, &m]()
		{
			return m.source->is_over() || u.terminal->is_interrupt();
		});
		stop_threads();
	}));
#endif

	u.chain_main->exec([&u, &m]()
	{
		u.monitor_red->is_done();
		return m.source->is_over() || u.terminal->is_interrupt();
	});

#ifdef ADAPTOR
	stop_threads();

	for (auto &t : threads)
		t.join();
#endif

	// final reduction
	u.monitor_red->reduce();

	// display the performance (BER and FER) in the terminal
	u.terminal->final_report();

	// reset the monitor and the terminal for the next SNR
	u.monitor_red->reset();
	u.terminal->reset();

	// display the statistics of the tasks (if enabled)
#ifdef ADAPTOR
	std::cout << "#" << std::endl << "# Chain pre: " << std::endl;
	tools::Stats::show(u.chain_pre->get_tasks_per_types(), true);
#endif
	std::cout << "#" << std::endl << "# Chain main: " << std::endl;
	tools::Stats::show(u.chain_main->get_tasks_per_types(), true);
#ifdef ADAPTOR
	std::cout << "#" << std::endl << "# Chain post: " << std::endl;
	tools::Stats::show(u.chain_post->get_tasks_per_types(), true);
#endif
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
	                                               p.terminal.get()                                 };

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

#ifdef ADAPTOR
	m.adaptor_1_to_n = std::unique_ptr<module::Adaptor_1_to_n>(new module::Adaptor_1_to_n(p.codec->enc->K,
	                                                                                      typeid(int),
	                                                                                      1,
	                                                                                      false,
	                                                                                      p.source->n_frames));

	m.adaptor_n_to_1 = std::unique_ptr<module::Adaptor_n_to_1>(new module::Adaptor_n_to_1(p.codec->enc->K,
	                                                                                      typeid(int),
	                                                                                      1,
	                                                                                      false,
	                                                                                      p.source->n_frames));

	m.adaptor_1_to_n->set_custom_name("Adp_1_to_n");
	m.adaptor_n_to_1->set_custom_name("Adp_n_to_1");
#endif
}

void init_utils(const params &p, const modules &m, utils &u)
{
#ifdef ADAPTOR
	u.chain_pre = std::unique_ptr<tools::Chain>(new tools::Chain((*m.source        )[module::src::tsk::generate],
	                                                             (*m.adaptor_1_to_n)[module::adp::tsk::push_1  ],
	                                                             1));

	std::ofstream f_pre("chain_pre.dot");
	u.chain_pre->export_dot(f_pre);

	u.chain_main = std::unique_ptr<tools::Chain>(new tools::Chain((*m.adaptor_1_to_n)[module::adp::tsk::pull_n],
	                                                              (*m.adaptor_n_to_1)[module::adp::tsk::push_n],
	                                                              p.n_threads ? p.n_threads : 1));

	std::ofstream f_main("chain_main.dot");
	u.chain_main->export_dot(f_main);

	u.chain_post = std::unique_ptr<tools::Chain>(new tools::Chain((*m.adaptor_n_to_1)[module::adp::tsk::pull_1],
	                                                              (*m.sink          )[module::snk::tsk::send  ],
	                                                              1));

	std::ofstream f_post("chain_post.dot");
	u.chain_post->export_dot(f_post);
#else
	u.chain_main = std::unique_ptr<tools::Chain>(new tools::Chain((*m.source)[module::src::tsk::generate],
	                                                              (*m.sink  )[module::snk::tsk::send    ],
	                                                              p.n_threads ? p.n_threads : 1));

	std::ofstream f_main("chain_main.dot");
	u.chain_main->export_dot(f_main);
#endif

	// allocate a common monitor module to reduce all the monitors
	u.monitor_red = std::unique_ptr<tools::Monitor_BFER_reduction>(new tools::Monitor_BFER_reduction(
		u.chain_main->get_modules<module::Monitor_BFER<>>()));
	u.monitor_red->set_reduce_frequency(std::chrono::milliseconds(500));
	// create a sigma noise type
	u.noise = std::unique_ptr<tools::Sigma<>>(new tools::Sigma<>());
	// report the noise values (Es/N0 and Eb/N0)
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise<>(*u.noise)));
	// report the bit/frame error rates
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER<>(*u.monitor_red)));
	// report the simulation throughputs
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_throughput<>(*u.monitor_red)));
	// create a terminal that will display the collected data from the reporters
	u.terminal = std::unique_ptr<tools::Terminal>(p.terminal->build(u.reporters));

	// configuration of the chain tasks
	for (auto& type : u.chain_main->get_tasks_per_types()) for (auto& tsk : type)
	{
		tsk->set_debug      (false); // disable the debug mode
		tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
		tsk->set_stats      (true ); // enable the statistics

		// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
		if (!tsk->is_debug() && !tsk->is_stats())
			tsk->set_fast(true);
	}

#ifdef ADAPTOR
	// configuration of the chain tasks
	for (auto& type : u.chain_pre->get_tasks_per_types()) for (auto& tsk : type)
	{
		tsk->set_debug      (false); // disable the debug mode
		tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
		tsk->set_stats      (true ); // enable the statistics

		// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
		if (!tsk->is_debug() && !tsk->is_stats())
			tsk->set_fast(true);
	}

	// configuration of the chain tasks
	for (auto& type : u.chain_post->get_tasks_per_types()) for (auto& tsk : type)
	{
		tsk->set_debug      (false); // disable the debug mode
		tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
		tsk->set_stats      (true ); // enable the statistics

		// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
		if (!tsk->is_debug() && !tsk->is_stats())
			tsk->set_fast(true);
	}
#endif
}