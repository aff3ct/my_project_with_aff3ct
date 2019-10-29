	 #include <vector>
 #include <iostream>
 #include <thread>
 #include <mutex>
 #include <algorithm>
 #include <aff3ct.hpp>
 
 #include "Block.hpp"
 #include "Circular_Buffer.hpp"
 #include "Destination/NO/Destination_NO.hpp"
 #include "Splitter/Splitter.hpp"

#define TEST 0

int main(int argc, char** argv)
{
	std::cout << "#-------------------------------------------------------" << std::endl;
	std::cout << "# This is a basic program using the AFF3CT library."      << std::endl;
	std::cout << "# Feel free to improve it as you want to fit your needs." << std::endl;
	std::cout << "#-------------------------------------------------------" << std::endl;
	std::cout << "#"                                                        << std::endl;

	const int   fe       = 100;
	const int   seed     = argc >= 2 ? std::atoi(argv[1]) : 0;
	const int   K        = 32;
	const int   N        = 128;
	const float R        = (float)K / (float)N;
	const float ebn0_min = 30.00f;
	const float ebn0_max = 30.1f;
	const int buf_length = 100000;

	std::cout << "# * Simulation parameters: "           << std::endl;
	std::cout << "#    ** Frame errors   = " << fe       << std::endl;
	std::cout << "#    ** Noise seed     = " << seed     << std::endl;
	std::cout << "#    ** Info. bits (K) = " << K        << std::endl;
	std::cout << "#    ** Frame size (N) = " << N        << std::endl;
	std::cout << "#    ** Code rate  (R) = " << R        << std::endl;
	std::cout << "#    ** SNR min   (dB) = " << ebn0_min << std::endl;
	std::cout << "#    ** SNR max   (dB) = " << ebn0_max << std::endl;
	std::cout << "#"                                     << std::endl;

	// create the AFF3CT modules
	aff3ct::module::Source_random<>          source      (K      );
	aff3ct::module::Encoder_repetition_sys<> encoder     (K, N   );
	aff3ct::module::Modem_BPSK<>             modulator   (N      );
	aff3ct::module::Modem_BPSK<>             demodulator (N      );
	aff3ct::module::Channel_AWGN_LLR<>       channel     (N, seed);
	aff3ct::module::Decoder_repetition_std<> decoder     (K, N   );
	aff3ct::module::Splitter<>               splitter    (K      );
	aff3ct::module::Monitor_BFER<>           monitor     (K, fe  );
	
	// configuration of the module tasks
	std::vector<const aff3ct::module::Module*> modules = {&source, &encoder, &modulator, &channel, &demodulator, &decoder, &splitter, &monitor};
	for (auto *m : modules)
		for (auto *t : m->tasks)
		{
			t->set_autoalloc  (false); // enable the automatic allocation of the data in the tasks
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
	
	#if TEST == 1  
	Circular_Buffer<int32_t> cb(4,10);

	std::mutex print_lock;

	std::thread t1([&cb, &print_lock]() {
		int32_t a = 0;
		std::vector<int32_t>* v = new std::vector<int32_t>(10,a);

		while(true)
		{
			
			for (auto &elt : *v)
				elt = a++%100;
						
			print_lock.lock();
			
			std::cout << "Thread 1 push : ";
			std::cout << "[ ";
			for (auto const &elt : *v)
				std::cout << std::setw(2) << elt << " ";
			std::cout << "]" << std::endl;	
			
			print_lock.unlock();
			cb.wait_push(&v);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	});

	std::thread t2([&cb, &print_lock]() {
		std::vector<int32_t>* v = new std::vector<int32_t>(10,0);

		while(true)
		{
			cb.wait_pop(&v);
			print_lock.lock();
			std::cout << "Thread 2 pop :                                      ";
			std::cout << "[ ";
			for (auto const &elt : *v)
				std::cout << std::setw(2) << elt << " ";
			std::cout << "]" << std::endl;	
			print_lock.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	});

	t1.join();
	t2.join();
	
	
	#else
	Block bl_source      (&source      [src::tsk::generate    ] , buf_length, 1);
	Block bl_encoder     (&encoder     [enc::tsk::encode      ] , buf_length, 1);
	Block bl_modulator   (&modulator   [mdm::tsk::modulate    ] , buf_length, 1);
	Block bl_channel     (&channel     [chn::tsk::add_noise   ] , buf_length, 2);
	Block bl_demodulator (&demodulator [mdm::tsk::demodulate  ] , buf_length, 1);
	Block bl_decoder     (&decoder     [dec::tsk::decode_siho ] , buf_length, 1);
	Block bl_splitter    (&splitter    [spl::tsk::split       ] , buf_length, 1);
	Block bl_monitor     (&monitor     [mnt::tsk::check_errors] , buf_length, 1);
	
	bl_splitter   .bind ("U_K" , bl_source     , "U_K" );
	bl_encoder    .bind ("U_K" , bl_splitter   , "V_K1");
	bl_modulator  .bind ("X_N1", bl_encoder    , "X_N" );
	bl_channel    .bind ("X_N" , bl_modulator  , "X_N2");
	bl_demodulator.bind ("Y_N1", bl_channel    , "Y_N" );
	bl_decoder    .bind ("Y_N" , bl_demodulator, "Y_N2");
	
	bl_monitor    .bind ("U" , bl_splitter,  "V_K2" );
	bl_monitor    .bind ("V" , bl_decoder,   "V_K"  );
	
	aff3ct::tools::Terminal_BFER<> terminal(monitor);
	terminal.legend();
	terminal.start_temp_report();

	// a loop over the various SNRs
	for (auto ebn0 = ebn0_min; ebn0 < ebn0_max; ebn0 += 1.f)
	{
		// compute the current sigma for the channel noise
		const auto esn0  = aff3ct::tools::ebn0_to_esn0 (ebn0, R);
		const auto sigma = aff3ct::tools::esn0_to_sigma(esn0   );

		// give the current SNR to the terminal
		terminal.set_esn0(esn0);
		terminal.set_ebn0(ebn0);

		// update the sigma of the modem and the channel
		demodulator.set_sigma(sigma);
		channel    .set_sigma(sigma);

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal.start_temp_report();
		
		bool isDone = false;
		std::thread th_done_verif([&isDone, &monitor]() 
		{
			while(!monitor.fe_limit_achieved())
			{
			}
			isDone = true;
		});

		// run the simulation chain
		bl_source     .run(&isDone);
		bl_splitter   .run(&isDone);
		bl_encoder    .run(&isDone);
		bl_modulator  .run(&isDone);
		bl_channel    .run(&isDone);
		bl_demodulator.run(&isDone);
		bl_decoder    .run(&isDone);
		bl_monitor    .run(&isDone);

		th_done_verif .join();
		bl_source     .join();
		bl_splitter   .join();
		bl_encoder    .join();
		bl_modulator  .join();
		bl_channel    .join();
		bl_demodulator.join();
		bl_decoder    .join();
		bl_monitor    .join();

		bl_source     .reset();
		bl_splitter   .reset();
		bl_encoder    .reset();
		bl_modulator  .reset();
		bl_channel    .reset();
		bl_demodulator.reset();
		bl_decoder    .reset();
		bl_monitor    .reset();

		// display the performance (BER and FER) in the terminal
		terminal.final_report();

		// reset the monitor for the next SNR
		monitor.reset();
	}
	std::cout << "#" << std::endl;

	// display the statistics of the tasks (if enabled)
	auto ordered = true;
	aff3ct::tools::Stats::show(modules, ordered);
	
	#endif

	std::cout << "# End of the simulation" << std::endl;

	return 0;
}
