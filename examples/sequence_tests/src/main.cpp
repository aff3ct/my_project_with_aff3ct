#include <iostream>
#include <cstdlib>
#include <memory>
#include <vector>
#include <string>
#include <atomic>

#include <aff3ct.hpp>
using namespace aff3ct;

int main(int argc, char** argv)
{
	size_t n_threads = std::thread::hardware_concurrency();
	if (argc > 1)
		n_threads = std::atoi(argv[1]);
	std::cout << "n_threads = " << n_threads << std::endl;
	size_t n_inter_frames = 1;
	std::cout << "n_inter_frames = " << n_inter_frames << std::endl;

	size_t sleep_time_ns = 5000;
	std::cout << "sleep_time_ns = " << sleep_time_ns << std::endl;
	size_t data_length = 2048;
	std::cout << "data_length = " << data_length << std::endl;

	bool no_copy_mode = true;
	std::cout << "no_copy_mode = " << no_copy_mode << std::endl;

	bool stats = true;
	std::cout << "stats = " << stats << std::endl;

	std::cout << "###############################################" << std::endl;
	std::cout << "# Micro-benchmark 1: Simple chain             #" << std::endl;
	std::cout << "###############################################" << std::endl;

	unsigned int limit = 100000 * n_threads;
	// limit = 1;
	std::cout << "limit = " << limit << std::endl;

	// modules creation
	module::Initializer<> initializer(data_length);
	module::Finalizer  <> finalizer  (data_length);

	std::vector<std::shared_ptr<module::Incrementer<>>> incs(6);
	for (size_t s = 0; s < incs.size(); s++)
	{
		incs[s].reset(new module::Incrementer<>(data_length));
		incs[s]->set_ns(sleep_time_ns);
		incs[s]->set_custom_name("Inc" + std::to_string(s));
	}

	// sockets binding
	(*incs[0])[module::inc::sck::increment::in].bind(initializer[module::ini::sck::initialize::out]);
	for (size_t s = 0; s < incs.size() -1; s++)
		(*incs[s+1])[module::inc::sck::increment::in].bind((*incs[s])[module::inc::sck::increment::out]);
	finalizer[module::fin::sck::finalize::in].bind((*incs[incs.size()-1])[module::inc::sck::increment::out]);

	tools::Sequence sequence_chain(initializer[module::ini::tsk::initialize], n_threads);
	sequence_chain.set_n_frames(n_inter_frames);
	sequence_chain.set_no_copy_mode(no_copy_mode);

	auto tid = 0;
	for (auto cur_initializer : sequence_chain.get_cloned_modules<module::Initializer<>>(initializer))
	{
		std::vector<std::vector<int>> init_data(n_inter_frames, std::vector<int>(data_length, 0));
		for (size_t f = 0; f < n_inter_frames; f++)
			std::fill(init_data[f].begin(), init_data[f].end(), tid * n_inter_frames +f);
		cur_initializer->set_init_data(init_data);
		tid++;
	}

	std::ofstream file("sequence_chain.dot");
	sequence_chain.export_dot(file);

	// configuration of the sequence tasks
	for (auto& mod : sequence_chain.get_modules<module::Module>(false)) for (auto& tsk : mod->tasks)
	{
		tsk->reset          (     );
		tsk->set_debug      (false); // disable the debug mode
		tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
		tsk->set_stats      (stats); // enable the statistics
		tsk->set_fast       (true ); // enable the fast mode (= disable the useless verifs in the tasks)
	}

	std::atomic<unsigned int> counter(0);
	// execute the simulation sequence (multi-threaded)
	auto t_start = std::chrono::steady_clock::now();
	sequence_chain.exec([&counter, limit]() { return ++counter >= limit; });
	std::chrono::nanoseconds duration = std::chrono::steady_clock::now() - t_start;

	auto elapsed_time = duration.count() / 1000.f / 1000.f;
	std::cout << "Sequence elapsed time: " << elapsed_time << " ms" << std::endl;

	size_t chain_sleep_time = 0;
	for (auto &inc : incs)
		chain_sleep_time += inc->get_ns();

	auto theoretical_time = (chain_sleep_time * limit * n_inter_frames) / 1000.f / 1000.f / n_threads;
	std::cout << "Sequence theoretical time: " << theoretical_time << " ms" << std::endl;

	// verification of the sequence execution
	bool tests_passed = true;
	tid = 0;
	for (auto cur_finalizer : sequence_chain.get_cloned_modules<module::Finalizer<>>(finalizer))
	{
		for (size_t f = 0; f < n_inter_frames; f++)
		{
			const auto &final_data = cur_finalizer->get_final_data()[f];
			for (size_t d = 0; d < final_data.size(); d++)
			{
				auto expected = (int)incs.size() + (tid * n_inter_frames +f);
				if (final_data[d] != expected)
				{
					std::cout << "expected = " << expected << " - obtained = "
					          << final_data[d] << " (d = " << d << ", tid = " << tid << ")" << std::endl;
					tests_passed = false;
				}
			}
		}
		tid++;
	}
	std::cout << (tests_passed ? "Tests passed!" : "Tests failed :-(") << std::endl;

	// display the statistics of the tasks (if enabled)
	tools::Stats::show(sequence_chain.get_modules_per_types(), true);

	// sockets unbinding
	sequence_chain.set_n_frames(1);
	(*incs[0])[module::inc::sck::increment::in].unbind(initializer[module::ini::sck::initialize::out]);
	for (size_t s = 0; s < incs.size() -1; s++)
		(*incs[s+1])[module::inc::sck::increment::in].unbind((*incs[s])[module::inc::sck::increment::out]);
	finalizer[module::fin::sck::finalize::in].unbind((*incs[incs.size()-1])[module::inc::sck::increment::out]);

	std::cout << std::endl;
	std::cout << "###############################################" << std::endl;
	std::cout << "# Micro-benchmark 2: For loop (or while loop) #" << std::endl;
	std::cout << "###############################################" << std::endl;

	module::Switcher switcher(2, data_length, typeid(int));
	module::Iterator iterator(10);

	std::cout << "iterator.get_limit() = " << iterator.get_limit() << std::endl;
	limit = (100000 * n_threads) / iterator.get_limit();
	// limit = 1;
	std::cout << "limit = " << limit << std::endl;

	switcher  [module::swi::tsk::select ][1]   .bind(initializer[module::ini::sck::initialize::out]);
	iterator  [module::ite::tsk::iterate]      .bind(switcher   [module::swi::tsk::select][3]);
	switcher  [module::swi::tsk::commute][0]   .bind(switcher   [module::swi::tsk::select][2]);
	switcher  [module::swi::tsk::commute][1]   .bind(iterator   [module::ite::sck::iterate::out]);
	(*incs[0])[module::inc::sck::increment::in].bind(switcher   [module::swi::tsk::commute][2]);
	for (size_t s = 0; s < incs.size() -1; s++)
		(*incs[s+1])[module::inc::sck::increment::in].bind((*incs[s])[module::inc::sck::increment::out]);
	switcher  [module::swi::tsk::select][0]    .bind((*incs[incs.size()-1])[module::inc::sck::increment::out]);
	finalizer [module::fin::sck::finalize::in] .bind(switcher   [module::swi::tsk::commute][3]);

	tools::Sequence sequence_for_loop(initializer[module::ini::tsk::initialize], n_threads);
	sequence_for_loop.set_n_frames(n_inter_frames);
	sequence_for_loop.set_no_copy_mode(no_copy_mode);

	for (auto cur_module : sequence_for_loop.get_modules<tools::Interface_reset>())
		cur_module->reset();

	tid = 0;
	for (auto cur_initializer : sequence_for_loop.get_cloned_modules<module::Initializer<>>(initializer))
	{
		std::vector<std::vector<int>> init_data(n_inter_frames, std::vector<int>(data_length, 0));
		for (size_t f = 0; f < n_inter_frames; f++)
			std::fill(init_data[f].begin(), init_data[f].end(), tid * n_inter_frames +f);
		cur_initializer->set_init_data(init_data);
		tid++;
	}

	// configuration of the sequence tasks
	for (auto& mod : sequence_for_loop.get_modules<module::Module>(false)) for (auto& tsk : mod->tasks)
	{
		tsk->reset          (     );
		tsk->set_debug      (false); // disable the debug mode
		tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
		tsk->set_stats      (stats); // enable the statistics
		tsk->set_fast       (true ); // enable the fast mode (= disable the useless verifs in the tasks)
	}

	std::ofstream file2("sequence_for_loop.dot");
	sequence_for_loop.export_dot(file2);

	counter = 0;
	t_start = std::chrono::steady_clock::now();
	sequence_for_loop.exec([&counter, limit]() { return ++counter >= limit; });
	duration = std::chrono::steady_clock::now() - t_start;

	elapsed_time = duration.count() / 1000.f / 1000.f;
	std::cout << "Sequence elapsed time: " << elapsed_time << " ms" << std::endl;
	theoretical_time = ((chain_sleep_time * limit * n_inter_frames) / 1000.f / 1000.f / n_threads) * iterator.get_limit();
	std::cout << "Sequence theoretical time: " << theoretical_time << " ms" << std::endl;

	// verification of the sequence execution
	tests_passed = true;
	tid = 0;
	for (auto cur_finalizer : sequence_for_loop.get_cloned_modules<module::Finalizer<>>(finalizer))
	{
		for (size_t f = 0; f < n_inter_frames; f++)
		{
			const auto &final_data = cur_finalizer->get_final_data()[f];
			for (size_t d = 0; d < final_data.size(); d++)
			{
				auto expected = (int)incs.size() * iterator.get_limit() + (tid * n_inter_frames +f);
				if (final_data[d] != expected)
				{
					std::cout << "expected = " << expected << " - obtained = "
					          << final_data[d] << " (d = " << d << ", tid = " << tid << ")" << std::endl;
					tests_passed = false;
				}
			}
		}
		tid++;
	}
	std::cout << (tests_passed ? "Tests passed!" : "Tests failed :-(") << std::endl;

	// display the statistics of the tasks (if enabled)
	tools::Stats::show(sequence_for_loop.get_modules_per_types(), true);

	// unbind
	sequence_for_loop.set_n_frames(1);
	switcher  [module::swi::tsk::select ][1]   .unbind(initializer[module::ini::sck::initialize::out]);
	iterator  [module::ite::tsk::iterate]      .unbind(switcher   [module::swi::tsk::select][3]);
	switcher  [module::swi::tsk::commute][0]   .unbind(switcher   [module::swi::tsk::select][2]);
	switcher  [module::swi::tsk::commute][1]   .unbind(iterator   [module::ite::sck::iterate::out]);
	(*incs[0])[module::inc::sck::increment::in].unbind(switcher   [module::swi::tsk::commute][2]);
	for (size_t s = 0; s < incs.size() -1; s++)
		(*incs[s+1])[module::inc::sck::increment::in].unbind((*incs[s])[module::inc::sck::increment::out]);
	switcher  [module::swi::tsk::select][0]    .unbind((*incs[incs.size()-1])[module::inc::sck::increment::out]);
	finalizer [module::fin::sck::finalize::in] .unbind(switcher   [module::swi::tsk::commute][3]);

	std::cout << std::endl;
	std::cout << "###############################################" << std::endl;
	std::cout << "# Micro-benchmark 3: Do while loop            #" << std::endl;
	std::cout << "###############################################" << std::endl;

	iterator.set_limit(iterator.get_limit() -1);
	std::cout << "iterator.get_limit() = " << iterator.get_limit() << std::endl;
	// limit = 1;
	std::cout << "limit = " << limit << std::endl;

	switcher  [module::swi::tsk::select ][1]   .bind(initializer[module::ini::sck::initialize::out]);
	iterator  [module::ite::tsk::iterate]      .bind(switcher   [module::swi::tsk::select][3]);
	switcher  [module::swi::tsk::commute][1]   .bind(iterator   [module::ite::sck::iterate::out]);
	(*incs[0])[module::inc::sck::increment::in].bind(switcher   [module::swi::tsk::select][2]);
	for (size_t s = 0; s < incs.size() -1; s++)
		(*incs[s+1])[module::inc::sck::increment::in].bind((*incs[s])[module::inc::sck::increment::out]);
	switcher  [module::swi::tsk::commute][0]   .bind((*incs[5]) [module::inc::sck::increment::out]);
	switcher  [module::swi::tsk::select ][0]   .bind(switcher   [module::swi::tsk::commute][2]);
	finalizer [module::fin::sck::finalize::in] .bind(switcher   [module::swi::tsk::commute][3]);

	tools::Sequence sequence_do_while_loop(initializer[module::ini::tsk::initialize], n_threads);
	sequence_do_while_loop.set_n_frames(n_inter_frames);
	sequence_do_while_loop.set_no_copy_mode(no_copy_mode);

	for (auto cur_module : sequence_do_while_loop.get_modules<tools::Interface_reset>())
		cur_module->reset();

	tid = 0;
	for (auto cur_initializer : sequence_do_while_loop.get_cloned_modules<module::Initializer<>>(initializer))
	{
		std::vector<std::vector<int>> init_data(n_inter_frames, std::vector<int>(data_length, 0));
		for (size_t f = 0; f < n_inter_frames; f++)
			std::fill(init_data[f].begin(), init_data[f].end(), tid * n_inter_frames +f);
		cur_initializer->set_init_data(init_data);
		tid++;
	}

	// configuration of the sequence tasks
	for (auto& mod : sequence_do_while_loop.get_modules<module::Module>(false)) for (auto& tsk : mod->tasks)
	{
		tsk->reset          (     );
		tsk->set_debug      (false); // disable the debug mode
		tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
		tsk->set_stats      (stats); // enable the statistics
		tsk->set_fast       (true ); // enable the fast mode (= disable the useless verifs in the tasks)
	}

	std::ofstream file3("sequence_do_while_loop.dot");
	sequence_do_while_loop.export_dot(file3);

	counter = 0;
	t_start = std::chrono::steady_clock::now();
	sequence_do_while_loop.exec([&counter, limit]() { return ++counter >= limit; });
	duration = std::chrono::steady_clock::now() - t_start;

	elapsed_time = duration.count() / 1000.f / 1000.f;
	std::cout << "Sequence elapsed time: " << elapsed_time << " ms" << std::endl;
	theoretical_time = ((chain_sleep_time * limit * n_inter_frames) / 1000.f / 1000.f / n_threads) * (iterator.get_limit()+1);
	std::cout << "Sequence theoretical time: " << theoretical_time << " ms" << std::endl;

	// verification of the sequence execution
	tests_passed = true;
	tid = 0;
	for (auto cur_finalizer : sequence_do_while_loop.get_cloned_modules<module::Finalizer<>>(finalizer))
	{
		for (size_t f = 0; f < n_inter_frames; f++)
		{
			const auto &final_data = cur_finalizer->get_final_data()[f];
			for (size_t d = 0; d < final_data.size(); d++)
			{
				auto expected = (int)incs.size() * (iterator.get_limit()+1) + (tid * n_inter_frames +f);
				if (final_data[d] != expected)
				{
					std::cout << "expected = " << expected << " - obtained = "
					          << final_data[d] << " (d = " << d << ", tid = " << tid << ")" << std::endl;
					tests_passed = false;
				}
			}
		}
		tid++;
	}
	std::cout << (tests_passed ? "Tests passed!" : "Tests failed :-(") << std::endl;

	// display the statistics of the tasks (if enabled)
	tools::Stats::show(sequence_do_while_loop.get_modules_per_types(), true);

	// unbind
	sequence_do_while_loop.set_n_frames(1);
	switcher  [module::swi::tsk::select ][1]   .unbind(initializer[module::ini::sck::initialize::out]);
	iterator  [module::ite::tsk::iterate]      .unbind(switcher   [module::swi::tsk::select][3]);
	switcher  [module::swi::tsk::commute][1]   .unbind(iterator   [module::ite::sck::iterate::out]);
	(*incs[0])[module::inc::sck::increment::in].unbind(switcher   [module::swi::tsk::select][2]);
	for (size_t s = 0; s < incs.size() -1; s++)
		(*incs[s+1])[module::inc::sck::increment::in].unbind((*incs[s])[module::inc::sck::increment::out]);
	switcher  [module::swi::tsk::commute][0]   .unbind((*incs[5]) [module::inc::sck::increment::out]);
	switcher  [module::swi::tsk::select ][0]   .unbind(switcher   [module::swi::tsk::commute][2]);
	finalizer [module::fin::sck::finalize::in] .unbind(switcher   [module::swi::tsk::commute][3]);

	std::cout << std::endl;
	std::cout << "###############################################" << std::endl;
	std::cout << "# Micro-benchmark 4: Exclusive paths          #" << std::endl;
	std::cout << "###############################################" << std::endl;

	module::Controller_static controller;
	module::Switcher switchex(3, data_length, typeid(int));

	controller[module::ctr::tsk::control      ].bind(initializer[module::ini::sck::initialize::out]);
	switchex  [module::swi::tsk::commute   ][0].bind(initializer[module::ini::sck::initialize::out]);
	switchex  [module::swi::tsk::commute   ][1].bind(controller [module::ctr::sck::control   ::out]);
	// path 0
	(*incs[0])[module::inc::sck::increment::in].bind(switchex   [module::swi::tsk::commute     ][2]);
	(*incs[1])[module::inc::sck::increment::in].bind((*incs[0]) [module::inc::sck::increment ::out]);
	(*incs[2])[module::inc::sck::increment::in].bind((*incs[1]) [module::inc::sck::increment ::out]);
	switchex  [module::swi::tsk::select    ][0].bind((*incs[2]) [module::inc::sck::increment ::out]);
	// path 1
	(*incs[3])[module::inc::sck::increment::in].bind(switchex   [module::swi::tsk::commute     ][3]);
	(*incs[4])[module::inc::sck::increment::in].bind((*incs[3]) [module::inc::sck::increment ::out]);
	switchex  [module::swi::tsk::select    ][1].bind((*incs[4]) [module::inc::sck::increment ::out]);
	// path 2
	(*incs[5])[module::inc::sck::increment::in].bind(switchex   [module::swi::tsk::commute     ][4]);
	switchex  [module::swi::tsk::select    ][2].bind((*incs[5]) [module::inc::sck::increment ::out]);
	// end
	finalizer [module::fin::sck::finalize ::in].bind(switchex   [module::swi::tsk::select      ][3]);

	tools::Sequence sequence_exclusive_paths(initializer[module::ini::tsk::initialize], n_threads);
	sequence_exclusive_paths.set_n_frames(n_inter_frames);
	sequence_exclusive_paths.set_no_copy_mode(no_copy_mode);

	tid = 0;
	for (auto cur_initializer : sequence_exclusive_paths.get_cloned_modules<module::Initializer<>>(initializer))
	{
		std::vector<std::vector<int>> init_data(n_inter_frames, std::vector<int>(data_length, 0));
		for (size_t f = 0; f < n_inter_frames; f++)
			std::fill(init_data[f].begin(), init_data[f].end(), tid * n_inter_frames +f);
		cur_initializer->set_init_data(init_data);
		tid++;
	}

	std::ofstream file4("sequence_exclusive_paths.dot");
	sequence_exclusive_paths.export_dot(file4);

	size_t multiplier[3] = {2, 3, 6};
	for (size_t path = 0; path < 3; path++)
	{
		std::cout << "Sub-test " << (path+1) << " - path = " << path << " ---------------------" << std::endl;
		limit = 100000 * n_threads * multiplier[path];
		// limit = 1;

		for (auto cur_module : sequence_exclusive_paths.get_modules<tools::Interface_reset>())
			cur_module->reset();

		// configuration of the sequence tasks
		for (auto& mod : sequence_exclusive_paths.get_modules<module::Module>(false)) for (auto& tsk : mod->tasks)
		{
			tsk->reset          (     );
			tsk->set_debug      (false); // disable the debug mode
			tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
			tsk->set_stats      (stats); // enable the statistics
			tsk->set_fast       (true ); // enable the fast mode (= disable the useless verifs in the tasks)
		}

		for (auto cur_controller : sequence_exclusive_paths.get_cloned_modules<module::Controller>(controller))
			cur_controller->set_path(path);

		counter = 0;
		t_start = std::chrono::steady_clock::now();
		sequence_exclusive_paths.exec([&counter, limit]() { return ++counter >= limit; });
		duration = std::chrono::steady_clock::now() - t_start;

		elapsed_time = duration.count() / 1000.f / 1000.f;
		std::cout << "Sequence elapsed time: " << elapsed_time << " ms" << std::endl;
		theoretical_time = (((chain_sleep_time / multiplier[path]) * limit * n_inter_frames) / 1000.f / 1000.f / n_threads);
		std::cout << "Sequence theoretical time: " << theoretical_time << " ms" << std::endl;

		// verification of the sequence execution
		tests_passed = true;
		tid = 0;
		for (auto cur_finalizer : sequence_exclusive_paths.get_cloned_modules<module::Finalizer<>>(finalizer))
		{
			for (size_t f = 0; f < n_inter_frames; f++)
			{
				const auto &final_data = cur_finalizer->get_final_data()[f];
				for (size_t d = 0; d < final_data.size(); d++)
				{
					auto expected = (int)incs.size() / multiplier[path] + (tid * n_inter_frames +f);
					if (final_data[d] != expected)
					{
						std::cout << "expected = " << expected << " - obtained = "
						          << final_data[d] << " (d = " << d << ", tid = " << tid << ")" << std::endl;
						tests_passed = false;
					}
				}
			}
			tid++;
		}
		std::cout << (tests_passed ? "Tests passed!" : "Tests failed :-(") << std::endl;

		// display the statistics of the tasks (if enabled)
		tools::Stats::show(sequence_exclusive_paths.get_modules_per_types(), true);
	}

	sequence_exclusive_paths.set_n_frames(1);
	controller[module::ctr::tsk::control      ].unbind(initializer[module::ini::sck::initialize::out]);
	switchex  [module::swi::tsk::commute   ][0].unbind(initializer[module::ini::sck::initialize::out]);
	switchex  [module::swi::tsk::commute   ][1].unbind(controller [module::ctr::sck::control   ::out]);
	(*incs[0])[module::inc::sck::increment::in].unbind(switchex   [module::swi::tsk::commute     ][2]);
	(*incs[1])[module::inc::sck::increment::in].unbind((*incs[0]) [module::inc::sck::increment ::out]);
	(*incs[2])[module::inc::sck::increment::in].unbind((*incs[1]) [module::inc::sck::increment ::out]);
	switchex  [module::swi::tsk::select    ][0].unbind((*incs[2]) [module::inc::sck::increment ::out]);
	(*incs[3])[module::inc::sck::increment::in].unbind(switchex   [module::swi::tsk::commute     ][3]);
	(*incs[4])[module::inc::sck::increment::in].unbind((*incs[3]) [module::inc::sck::increment ::out]);
	switchex  [module::swi::tsk::select    ][1].unbind((*incs[4]) [module::inc::sck::increment ::out]);
	(*incs[5])[module::inc::sck::increment::in].unbind(switchex   [module::swi::tsk::commute     ][4]);
	switchex  [module::swi::tsk::select    ][2].unbind((*incs[5]) [module::inc::sck::increment ::out]);
	finalizer [module::fin::sck::finalize ::in].unbind(switchex   [module::swi::tsk::select      ][3]);

	std::cout << std::endl;
	std::cout << "###############################################" << std::endl;
	std::cout << "# Micro-benchmark 5: Nested loops             #" << std::endl;
	std::cout << "###############################################" << std::endl;

	iterator.set_limit((iterator.get_limit() +1) / 2);
	module::Switcher switcher2(2, data_length, typeid(int));
	switcher2.set_custom_name("Switcher2");
	module::Iterator iterator2(2);
	iterator2.set_custom_name("Iterator2");

	std::cout << "iterator.get_limit() = " << iterator.get_limit() << std::endl;
	std::cout << "iterator2.get_limit() = " << iterator2.get_limit() << std::endl;
	limit = (100000 * n_threads) / (iterator.get_limit() * iterator2.get_limit());
	// limit = 1;
	std::cout << "limit = " << limit << std::endl;

	switcher2 [module::swi::tsk::select ][1]   .bind(initializer[module::ini::sck::initialize::out]);
	iterator2 [module::ite::tsk::iterate]      .bind(switcher2  [module::swi::tsk::select][3]);
	switcher2 [module::swi::tsk::commute][0]   .bind(switcher2  [module::swi::tsk::select][2]);
	switcher2 [module::swi::tsk::commute][1]   .bind(iterator2  [module::ite::sck::iterate::out]);
	switcher  [module::swi::tsk::select ][1]   .bind(switcher2  [module::swi::tsk::commute][2]);
	iterator  [module::ite::tsk::iterate]      .bind(switcher   [module::swi::tsk::select ][3]);
	switcher  [module::swi::tsk::commute][0]   .bind(switcher   [module::swi::tsk::select ][2]);
	switcher  [module::swi::tsk::commute][1]   .bind(iterator   [module::ite::sck::iterate::out]);
	(*incs[0])[module::inc::sck::increment::in].bind(switcher   [module::swi::tsk::commute][2]);
	for (size_t s = 0; s < incs.size() -1; s++)
		(*incs[s+1])[module::inc::sck::increment::in].bind((*incs[s])[module::inc::sck::increment::out]);
	switcher  [module::swi::tsk::select][0]    .bind((*incs[incs.size()-1])[module::inc::sck::increment::out]);
	switcher2 [module::swi::tsk::select][0]    .bind(switcher   [module::swi::tsk::commute][3]);
	finalizer [module::fin::sck::finalize::in] .bind(switcher2  [module::swi::tsk::commute][3]);

	tools::Sequence sequence_nested_loops(initializer[module::ini::tsk::initialize], n_threads);
	sequence_nested_loops.set_n_frames(n_inter_frames);
	sequence_nested_loops.set_no_copy_mode(no_copy_mode);

	for (auto cur_module : sequence_for_loop.get_modules<tools::Interface_reset>())
		cur_module->reset();

	tid = 0;
	for (auto cur_initializer : sequence_nested_loops.get_cloned_modules<module::Initializer<>>(initializer))
	{
		std::vector<std::vector<int>> init_data(n_inter_frames, std::vector<int>(data_length, 0));
		for (size_t f = 0; f < n_inter_frames; f++)
			std::fill(init_data[f].begin(), init_data[f].end(), tid * n_inter_frames +f);
		cur_initializer->set_init_data(init_data);
		tid++;
	}

	// configuration of the sequence tasks
	for (auto& mod : sequence_nested_loops.get_modules<module::Module>(false)) for (auto& tsk : mod->tasks)
	{
		tsk->reset          (     );
		tsk->set_debug      (false); // disable the debug mode
		tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
		tsk->set_stats      (stats); // enable the statistics
		tsk->set_fast       (true ); // enable the fast mode (= disable the useless verifs in the tasks)
	}

	std::ofstream file5("sequence_nested_loops.dot");
	sequence_nested_loops.export_dot(file5);

	counter = 0;
	t_start = std::chrono::steady_clock::now();
	sequence_nested_loops.exec([&counter, limit]() { return ++counter >= limit; });
	duration = std::chrono::steady_clock::now() - t_start;

	elapsed_time = duration.count() / 1000.f / 1000.f;
	std::cout << "Sequence elapsed time: " << elapsed_time << " ms" << std::endl;
	theoretical_time = ((chain_sleep_time * limit * n_inter_frames) / 1000.f / 1000.f / n_threads) * (iterator.get_limit() * iterator2.get_limit());
	std::cout << "Sequence theoretical time: " << theoretical_time << " ms" << std::endl;

	// verification of the sequence execution
	tests_passed = true;
	tid = 0;
	for (auto cur_finalizer : sequence_nested_loops.get_cloned_modules<module::Finalizer<>>(finalizer))
	{
		for (size_t f = 0; f < n_inter_frames; f++)
		{
			const auto &final_data = cur_finalizer->get_final_data()[f];
			for (size_t d = 0; d < final_data.size(); d++)
			{
				auto expected = (int)incs.size() * iterator.get_limit() * iterator2.get_limit() + (tid * n_inter_frames +f);
				if (final_data[d] != expected)
				{
					std::cout << "expected = " << expected << " - obtained = "
					          << final_data[d] << " (d = " << d << ", tid = " << tid << ")" << std::endl;
					tests_passed = false;
				}
			}
		}
		tid++;
	}
	std::cout << (tests_passed ? "Tests passed!" : "Tests failed :-(") << std::endl;

	// display the statistics of the tasks (if enabled)
	tools::Stats::show(sequence_nested_loops.get_modules_per_types(), true);

	return 0;
}
