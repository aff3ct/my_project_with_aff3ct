 #include <vector>
 #include <iostream>
 #include <aff3ct.hpp>

int old_main(int argc, char** argv)
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
	const float ebn0_min = 0.00f;
	const float ebn0_max = 10.1f;

	std::cout << "# * Simulation parameters: "           << std::endl;
	std::cout << "#    ** Frame errors   = " << fe       << std::endl;
	std::cout << "#    ** Noise seed     = " << seed     << std::endl;
	std::cout << "#    ** Info. bits (K) = " << K        << std::endl;
	std::cout << "#    ** Frame size (N) = " << N        << std::endl;
	std::cout << "#    ** Code rate  (R) = " << R        << std::endl;
	std::cout << "#    ** SNR min   (dB) = " << ebn0_min << std::endl;
	std::cout << "#    ** SNR max   (dB) = " << ebn0_max << std::endl;
	std::cout << "#"                                     << std::endl;

	// buffers to store the data
	std::vector<int  > ref_bits     (K);
	std::vector<int  > enc_bits     (N);
	std::vector<float> symbols      (N);
	std::vector<float> noisy_symbols(N);
	std::vector<float> LLRs         (N);
	std::vector<int  > dec_bits     (K);

	// create the AFF3CT objects
	aff3ct::module::Source_random<>          source  (K      );
	aff3ct::module::Encoder_repetition_sys<> encoder (K, N   );
	aff3ct::module::Modem_BPSK<>             modem   (N      );
	aff3ct::module::Channel_AWGN_LLR<>       channel (N, seed);
	aff3ct::module::Decoder_repetition_std<> decoder (K, N   );
	aff3ct::module::Monitor_BFER<>           monitor (K, fe  );
	aff3ct::tools ::Terminal_BFER<>          terminal(monitor);

	// display the legend in the terminal
	terminal.legend();

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
		modem  .set_sigma(sigma);
		channel.set_sigma(sigma);

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal.start_temp_report();

		// run a small simulation chain
		while (!monitor.fe_limit_achieved())
		{
			source .generate    (               ref_bits     );
			encoder.encode      (ref_bits,      enc_bits     );
			modem  .modulate    (enc_bits,      symbols      );
			channel.add_noise   (symbols,       noisy_symbols);
			modem  .demodulate  (noisy_symbols, LLRs         );
			decoder.decode_siho (LLRs,          dec_bits     );
			monitor.check_errors(dec_bits,      ref_bits     );
		}

		// display the performance (BER and FER) in the terminal
		terminal.final_report();

		// reset the monitor for the next SNR
		monitor.reset();
	}

	return 0;
}

int new_main(int argc, char** argv)
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
	const float ebn0_min = 0.00f;
	const float ebn0_max = 10.1f;

	std::cout << "# * Simulation parameters: "           << std::endl;
	std::cout << "#    ** Frame errors   = " << fe       << std::endl;
	std::cout << "#    ** Noise seed     = " << seed     << std::endl;
	std::cout << "#    ** Info. bits (K) = " << K        << std::endl;
	std::cout << "#    ** Frame size (N) = " << N        << std::endl;
	std::cout << "#    ** Code rate  (R) = " << R        << std::endl;
	std::cout << "#    ** SNR min   (dB) = " << ebn0_min << std::endl;
	std::cout << "#    ** SNR max   (dB) = " << ebn0_max << std::endl;
	std::cout << "#"                                     << std::endl;

	// create the AFF3CT objects
	aff3ct::module::Source_random<>          source  (K      );
	aff3ct::module::Encoder_repetition_sys<> encoder (K, N   );
	aff3ct::module::Modem_BPSK<>             modem   (N      );
	aff3ct::module::Channel_AWGN_LLR<>       channel (N, seed);
	aff3ct::module::Decoder_repetition_std<> decoder (K, N   );
	aff3ct::module::Monitor_BFER<>           monitor (K, fe  );
	aff3ct::tools ::Terminal_BFER<>          terminal(monitor);

	std::vector<aff3ct::module::Module*> modules;
	modules.push_back(&source);
	modules.push_back(&encoder);
	modules.push_back(&modem);
	modules.push_back(&channel);
	modules.push_back(&decoder);
	modules.push_back(&monitor);

	// configuration of the tasks
	for (auto *m : modules)
		for (auto &t : m->tasks)
		{
			t.second->set_autoalloc(true ); // enable automatic allocation of the data in the tasks
			t.second->set_autoexec (true ); // enable the auto execution mode of the tasks
			t.second->set_debug    (false); // disable the debug mode
			t.second->set_stats    (true ); // enable the statistics
		}

	// display the legend in the terminal
	terminal.legend();

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
		modem  .set_sigma(sigma);
		channel.set_sigma(sigma);

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal.start_temp_report();

		// run a small simulation chain
		while (!monitor.fe_limit_achieved())
		{
			source ["generate"    ].exec();
			encoder["encode"      ]["U_K" ].bind(source ["generate"   ]["U_K" ]);
			modem  ["modulate"    ]["X_N1"].bind(encoder["encode"     ]["X_N" ]);
			channel["add_noise"   ]["X_N" ].bind(modem  ["modulate"   ]["X_N2"]);
			modem  ["demodulate"  ]["Y_N1"].bind(channel["add_noise"  ]["Y_N" ]);
			decoder["decode_siho" ]["Y_N" ].bind(modem  ["demodulate" ]["Y_N2"]);
			monitor["check_errors"]["U"   ].bind(encoder["encode"     ]["U_K" ]);
			monitor["check_errors"]["V"   ].bind(decoder["decode_siho"]["V_K" ]);
		}

		// display the performance (BER and FER) in the terminal
		terminal.final_report();

		// reset the monitor for the next SNR
		monitor.reset();
	}
	std::cout << "#" << std::endl;

	// display the statistics of the tasks (if enabled)
	aff3ct::tools::Stats::show(modules);

	std::cout << "#" << std::endl;
	std::cout << "# Powered by AFF3CT (v" << aff3ct::version_major() << "." << aff3ct::version_minor() << "."
	          << aff3ct::version_release() << ")." << std::endl;

	return 0;
}

int main(int argc, char** argv)
{
	return new_main(argc, argv);
}
