#include <type_traits>
#include <functional>
#include <exception>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstdlib>
#include <chrono>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <random>

#include <aff3ct.hpp>
using namespace aff3ct;

std::unique_ptr<spu::module::Stateless_Julia>
build_decoder_rep_julia(const int K,
                        const int N_cw,
                        const size_t n_frames,
                        const bool buffered,
                        const std::string& jl_ver,
                        const std::string& jl_filepath,
                        const bool force_jl_safe)
{
    bool jl_safe     = true;
    bool single_wave = false;
    std::string fn_prefix;

    if (jl_ver == "v0")
    {
        fn_prefix   = "decode_rep_v0";
        jl_safe     = true;
        single_wave = false;
    }
    else if (jl_ver == "v1")
    {
        fn_prefix   = "decode_rep_v1";
        jl_safe     = true;
        single_wave = false;
    }
    else if (jl_ver == "v2")
    {
        fn_prefix   = "decode_rep_v2";
        jl_safe     = true;
        single_wave = false;
    }
    else if (jl_ver == "v3")
    {
        fn_prefix   = "decode_rep_v2";
        jl_safe     = false;
        single_wave = false;
    }
    else if (jl_ver == "v4")
    {
        fn_prefix   = "decode_rep_v4";
        jl_safe     = force_jl_safe;
        single_wave = true;
    }
    else
    {
        std::stringstream message;
        message << "Unknown Julia repetition decoder version ('jl_ver' = " << jl_ver
                << ", expected v0, v1, v2, v3, or v4).";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    const std::string fn_name = fn_prefix + (buffered ? "_buf" : "_nobuf");

    std::unique_ptr<spu::module::Stateless_Julia> dec(new spu::module::Stateless_Julia(jl_safe));
    dec->set_name("Decoder_repetition_JL_" + jl_ver);
    dec->set_short_name("Decoder_rep_" + jl_ver);
    if (single_wave)
        dec->set_single_wave(true);
    dec->set_n_frames(n_frames);

    auto& t = dec->create_task("decode_siho");
    t.set_replicability(true);
    dec->create_socket_in<float>(t, "Y_N", N_cw);
    dec->create_socket_out<int>(t, "V_K", K);
    dec->create_codelet_file(t, jl_filepath, fn_name);
    dec->eval();

    return dec;
}

struct params : public factory::Factory
{
    size_t      n_threads      = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1;
    size_t      n_frames       = 1;     // inter-frame level
    float       ebn0_min       = 0.00f; // minimum SNR value (dB)
    float       ebn0_max       = 6.00f; // maximum SNR value (dB)
    float       ebn0_step      = 1.00f; // SNR step (dB)
    float       R              = 0.0f;  // code rate (R=K/N)
    bool        jl_dec         = false; // use the Julia Stateless_Julia repetition decoder
    std::string jl_ver         = "v4";  // Julia optimization tier: v0, v1, v2, v3, v4
    bool        force_jl_safe  = false; // explicitly force jl_safe = true (e.g., on v4)
    std::string jl_path        = "../src/rep_decoder.jl";
    bool        buffered       = true;  // encoder/decoder bit layout (false if --enc-no-buff)
    bool        stats          = false; // enable per-task execution statistics (disables fast mode)
    bool        verbose        = false; // display per-SNR execution statistics (# Exec_stats)
    bool        force_sequence = false; // run a replicated Sequence instead of the 3-stage Pipeline
    int         interstage_buf = 0;     // Pipeline inter-stage buffer size (0 = auto >= 2 * n_threads)
    bool        active_waiting = false; // enable active waiting on Pipeline inter-stage buffers
    std::string dot_filepath   = "";    // optional Graphviz export path

    std::unique_ptr<factory::Source          > source;
    std::unique_ptr<factory::Codec_repetition> codec;
    std::unique_ptr<factory::Modem           > modem;
    std::unique_ptr<factory::Channel         > channel;
    std::unique_ptr<factory::Monitor_BFER    > monitor;
    std::unique_ptr<factory::Sink            > sink;
    std::unique_ptr<factory::Terminal        > terminal;

    params()
      : factory::Factory("Simulation", "Simulation", "sim")
    {
    }

    virtual ~params() = default;

    virtual params* clone() const override
    {
        return nullptr;
    }

    virtual void get_description(cli::Argument_map_info& args) const override
    {
        args.erase({ source ->get_prefix() + "-info-bits", "K" });
        args.erase({ modem  ->get_prefix() + "-fra-size",  "N" });
        args.erase({ channel->get_prefix() + "-fra-size",  "N" });
        args.erase({ monitor->get_prefix() + "-info-bits", "K" });
        args.erase({ sink   ->get_prefix() + "-info-bits", "K" });

        auto p = this->get_prefix();
        args.add({ p + "-threads",        "t" }, cli::Integer(cli::Positive()),                  "set the number of threads in the middle pipeline stage (or sequence).");
        args.add({ p + "-inter-fra",      "F" }, cli::Integer(cli::Positive(), cli::Non_zero()), "set the number of inter-frame level to process.");
        args.add({ p + "-ebn0-min",       "m" }, cli::Real(),                                    "set the minimum Eb/N0 noise value in dB.");
        args.add({ p + "-ebn0-max",       "M" }, cli::Real(),                                    "set the maximum Eb/N0 noise value in dB.");
        args.add({ p + "-ebn0-step",      "s" }, cli::Real(cli::Positive(), cli::Non_zero()),    "set the Eb/N0 noise step in dB.");
        args.add({ p + "-stats"               }, cli::None(),                                    "enable per-task execution statistics (disables fast mode).");
        args.add({ p + "-verbose",        "v" }, cli::None(),                                    "display per-SNR execution statistics (# Exec_stats).");
        args.add({ p + "-sequence"            }, cli::None(),                                    "execute a single multi-threaded Sequence instead of the 3-stage Pipeline.");
        args.add({ p + "-interstage-buf"      }, cli::Integer(cli::Positive(), cli::Non_zero()), "set the inter-stage buffer size in Pipeline mode (default: max(4, 2*t)).");
        args.add({ p + "-active-waiting"      }, cli::None(),                                    "enable active spin-waiting on inter-stage buffers in Pipeline mode.");
        args.add({ p + "-dot"                 }, cli::File(cli::openmode::write),                "export the Pipeline/Sequence graph to a Graphviz .dot file.");
        args.add({ "dec-jl"                   }, cli::None(),                                    "use the Julia Stateless_Julia repetition decoder.");
        args.add({ "dec-jl-ver"               }, cli::Text(cli::Including_set("v0", "v1", "v2", "v3", "v4")),
                                                                                                 "select the Julia repetition decoder tier (v0..v4).");
        args.add({ "dec-jl-safe"              }, cli::None(),                                    "force Julia exception checking on every call (e.g. with v4).");
        args.add({ "dec-jl-path"              }, cli::File(cli::openmode::read),                 "set the path to the Julia repetition decoder script.");
        cli::add_options(args.at({ "dec-implem" }), 0, "JL_V0", "JL_V1", "JL_V2", "JL_V3", "JL_V4");
    }

    virtual void store(const cli::Argument_map_value& vals) override
    {
        auto p = this->get_prefix();
        if (vals.exist({ p + "-threads",        "t" })) this->n_threads      = vals.to_int  ({ p + "-threads",        "t" });
        if (vals.exist({ p + "-inter-fra",      "F" })) this->n_frames       = vals.to_int  ({ p + "-inter-fra",      "F" });
        if (vals.exist({ p + "-ebn0-min",       "m" })) this->ebn0_min       = vals.to_float({ p + "-ebn0-min",       "m" });
        if (vals.exist({ p + "-ebn0-max",       "M" })) this->ebn0_max       = vals.to_float({ p + "-ebn0-max",       "M" });
        else if (this->ebn0_max < this->ebn0_min)       this->ebn0_max       = this->ebn0_min;
        if (vals.exist({ p + "-ebn0-step",      "s" })) this->ebn0_step      = vals.to_float({ p + "-ebn0-step",      "s" });
        if (vals.exist({ p + "-stats"               })) this->stats          = true;
        if (vals.exist({ p + "-verbose",        "v" })) this->verbose        = true;
        if (vals.exist({ p + "-sequence"            })) this->force_sequence = true;
        if (vals.exist({ p + "-interstage-buf"      })) this->interstage_buf = vals.to_int  ({ p + "-interstage-buf"      });
        if (vals.exist({ p + "-active-waiting"      })) this->active_waiting = true;
        if (vals.exist({ p + "-dot"                 })) this->dot_filepath   = vals.to_file ({ p + "-dot"                 });
        if (vals.exist({ "dec-jl"                   })) this->jl_dec         = true;
        if (vals.exist({ "dec-jl-ver"               })) { this->jl_dec = true; this->jl_ver = vals.at({ "dec-jl-ver" }); }
        if (vals.exist({ "dec-jl-safe"              })) this->force_jl_safe  = true;
        if (vals.exist({ "dec-jl-path"              })) this->jl_path        = vals.to_file ({ "dec-jl-path"              });
        else
        {
            const std::vector<std::string> candidates = {
                "../src/rep_decoder.jl",
                "src/rep_decoder.jl",
                "examples/pipeline_julia/src/rep_decoder.jl"
            };
            for (const auto& c : candidates)
            {
                std::ifstream f(c);
                if (f.good())
                {
                    this->jl_path = c;
                    break;
                }
            }
        }

        const std::string& imp = this->codec->dec->implem;
        if (imp == "JL_V0" || imp == "JL_V1" || imp == "JL_V2" || imp == "JL_V3" || imp == "JL_V4")
        {
            this->jl_dec = true;
            this->jl_ver = std::string("v") + imp.back();
            this->codec->dec->implem = "STD";
        }
        else if (this->jl_dec)
        {
            this->codec->dec->implem = "STD";
        }

        const auto& enc_rep = dynamic_cast<const factory::Encoder_repetition&>(*this->codec->enc);
        this->buffered = enc_rep.buffered;

        // Propagate K and N from Codec_repetition to the other module factories
        this->source ->K = this->codec->K;
        this->monitor->K = this->codec->K;
        this->sink   ->K = this->codec->K;

        this->modem->N = this->codec->N;
        this->modem->store(vals);

        this->channel->N            = this->modem->N_mod;
        this->channel->complex      = this->modem->complex;
        this->modem  ->channel_type = this->channel->type;

        this->R = (float)this->codec->K / (float)this->codec->N_cw;
    }

    virtual void get_headers(std::map<std::string, tools::header_list>& headers, const bool /*full*/ = true) const override
    {
        auto p = this->get_prefix();
        const size_t eff_threads = this->n_threads ? this->n_threads : 1;
        const size_t eff_buf = this->interstage_buf > 0 ? (size_t)this->interstage_buf : std::max<size_t>(4, 2 * eff_threads);
        headers[p].push_back(std::make_pair("Execution runtime", this->force_sequence ? "Sequence (replicated)" : "Pipeline (3 stages)"));
        headers[p].push_back(std::make_pair("Num. of threads (t)", std::to_string(eff_threads)));
        headers[p].push_back(std::make_pair("Inter frame level (F)", std::to_string(this->n_frames)));
        if (!this->force_sequence)
        {
            headers[p].push_back(std::make_pair("Inter-stage buffer", std::to_string(eff_buf)));
            headers[p].push_back(std::make_pair("Active waiting", this->active_waiting ? "on" : "off"));
        }
        headers[p].push_back(std::make_pair("Task statistics", this->stats ? "on" : "off (fast mode)"));
        headers[p].push_back(std::make_pair("Eb/N0 min (m)", std::to_string(this->ebn0_min) + " dB"));
        headers[p].push_back(std::make_pair("Eb/N0 max (M)", std::to_string(this->ebn0_max) + " dB"));
        headers[p].push_back(std::make_pair("Eb/N0 step (s)", std::to_string(this->ebn0_step) + " dB"));
        headers[p].push_back(std::make_pair("Decoder backend", this->jl_dec ? ("Julia Stateless_Julia (" + this->jl_ver + ")")
                                                                            : ("C++ AFF3CT (" + this->codec->dec->implem + ")")));
        if (this->jl_dec)
        {
            const bool is_safe = (this->jl_ver == "v0" || this->jl_ver == "v1" || this->jl_ver == "v2") ? true
                               : (this->jl_ver == "v4" ? this->force_jl_safe : false);
            headers[p].push_back(std::make_pair("Julia safety", is_safe ? "safe" : "unsafe (fast)"));
            headers[p].push_back(std::make_pair("Single wave (F in JL)", this->jl_ver == "v4" ? "on" : "off"));
            headers[p].push_back(std::make_pair("Julia script path", this->jl_path));
        }
    }
};
void init_params(int argc, char** argv, params &p);

struct modules
{
    std::unique_ptr<spu::module::Source<>>             source;
    std::unique_ptr<     tools ::Constellation<>>      constellation;
    std::unique_ptr<     module::Modem<>>              modem;
    std::unique_ptr<     module::Channel<>>            channel;
    std::unique_ptr<     module::Monitor_BFER<>>       monitor;
    std::unique_ptr<spu::module::Sink<>>               sink;
    std::unique_ptr<     tools ::Codec_repetition<>>   codec;
    std::unique_ptr<spu::module::Stateless_Julia>      decoder_jl;
                         module::Encoder<>*            encoder = nullptr;
                    spu::module::Module*               decoder = nullptr;
};
void init_modules(const params &p, modules &m);

using Monitor_BFER_reduction = tools::Monitor_reduction<module::Monitor_BFER<>>;

struct utils
{
                std::unique_ptr<     tools  ::Sigma<>          > noise;       // a sigma noise type
    std::vector<std::unique_ptr<spu::tools  ::Reporter         >> reporters;  // list of reporters displayed in the terminal
                std::unique_ptr<spu::tools  ::Terminal         > terminal;    // manage the output text in the terminal
                std::unique_ptr<     Monitor_BFER_reduction    > monitor_red; // monitor reduction across threads (Sequence mode)
                std::unique_ptr<spu::runtime::Pipeline         > pipeline;    // 3-stage pipeline runtime
                std::unique_ptr<spu::runtime::Sequence         > sequence;    // replicated sequence runtime (--sim-sequence)
};
void init_utils(const params &p, const modules &m, utils &u);

int main(int argc, char** argv)
{
    // StreamPU will catch and manage sigint
    spu::tools::Signal_handler::init();

    // get the AFF3CT version
    const std::string v = "v" + std::to_string(tools::version_major()) + "." +
                                std::to_string(tools::version_minor()) + "." +
                                std::to_string(tools::version_release());

    std::cout << "#----------------------------------------------------------"      << std::endl;
    std::cout << "# This is a basic program using the AFF3CT library (" << v << ")" << std::endl;
    std::cout << "# Feel free to improve it as you want to fit your needs."         << std::endl;
    std::cout << "#----------------------------------------------------------"      << std::endl;
    std::cout << "#"                                                                << std::endl;

    params p; init_params(argc, argv, p); // create and initialize the parameters from the command line with factories

    if (p.jl_dec)
        spu::tools::Julia::init();

    {
        modules m; init_modules(p, m); // create and initialize the modules

        // sockets binding (connect the sockets of the tasks = fill the input sockets with the output sockets)
        (*m.encoder)[      "encode::U_K"     ] = (*m.source )[   "generate::out_data" ];
        (*m.modem  )[    "modulate::X_N1"    ] = (*m.encoder)[     "encode::X_N"      ];
        (*m.channel)[   "add_noise::X_N"     ] = (*m.modem  )[   "modulate::X_N2"     ];
        (*m.modem  )[  "demodulate::Y_N1"    ] = (*m.channel)[  "add_noise::Y_N"      ];
        (*m.decoder)[ "decode_siho::Y_N"     ] = (*m.modem  )[ "demodulate::Y_N2"     ];
        (*m.monitor)["check_errors::U"       ] = (*m.source )[   "generate::out_data" ];
        (*m.monitor)["check_errors::V"       ] = (*m.decoder)["decode_siho::V_K"      ];
        (*m.sink   )[  "send_count::in_count"] = (*m.source )[   "generate::out_count"];
        (*m.sink   )[  "send_count::in_data" ] = (*m.decoder)["decode_siho::V_K"      ];
        std::vector<float> sigma(p.n_frames);
        (*m.channel)[   "add_noise::CP"      ] = sigma;
        (*m.modem  )[  "demodulate::CP"      ] = sigma;

        utils u; init_utils(p, m, u); // create and initialize the utils

        // set the noise and register to noise updates
        m.codec->set_noise(*u.noise);
        u.noise->record_callback_update([&m](){ m.codec->notify_noise_update(); });
        if (p.force_sequence)
        {
            for (auto &mod : u.sequence->get_modules<tools::Interface_get_set_noise>())
                mod->set_noise(*u.noise);
            for (auto &mod : u.sequence->get_modules<tools::Interface_notify_noise_update>())
                u.noise->record_callback_update([mod](){ mod->notify_noise_update(); });
        }
        else
        {
            for (auto &mod : u.pipeline->get_modules<tools::Interface_get_set_noise>())
                mod->set_noise(*u.noise);
            for (auto &mod : u.pipeline->get_modules<tools::Interface_notify_noise_update>())
                u.noise->record_callback_update([mod](){ mod->notify_noise_update(); });
        }

        // display the legend in the terminal
        u.terminal->legend();

        // loop over the various SNRs
        for (auto ebn0 = p.ebn0_min; ebn0 <= p.ebn0_max + 1e-5f; ebn0 += p.ebn0_step)
        {
            // set deterministic seeds in the modules that use PRNG at the start of each SNR point
            std::mt19937 prng;
            if (p.force_sequence)
            {
                for (auto &mod : u.sequence->get_modules<spu::module::Source<>>())
                    mod->set_seed(prng());
                for (auto &mod : u.sequence->get_modules<module::Channel<>>())
                    mod->set_seed(prng());
            }
            else
            {
                for (auto &mod : u.pipeline->get_modules<spu::module::Source<>>())
                    mod->set_seed(prng());
                for (auto &mod : u.pipeline->get_modules<module::Channel<>>())
                    mod->set_seed(prng());
            }

            // compute the current sigma for the channel noise
            const auto esn0 = tools::ebn0_to_esn0(ebn0, p.R, p.modem->bps);
            std::fill(sigma.begin(), sigma.end(), tools::esn0_to_sigma(esn0, p.modem->cpm_upf));

            u.noise->set_values(sigma[0], ebn0, esn0);

            // display the performance (BER and FER) in real time (in a separate thread) if enabled
            if (p.terminal->frequency != std::chrono::milliseconds(0))
                u.terminal->start_temp_report(p.terminal->frequency);

            for (auto& r : u.reporters)
                r->init();

            const auto t_exec_start = std::chrono::steady_clock::now();
            if (p.force_sequence)
            {
                u.sequence->exec([&u]() -> bool
                {
                    return u.monitor_red->is_done() || u.sequence->is_done();
                });
                u.monitor_red->reduce();
            }
            else
            {
                if (p.monitor->max_frame > 0)
                {
                    // Cap Stage 0 (single-threaded Source) at `max_frame` generated frames so downstream stages
                    // drain the exact number of frames without computing an extra uncounted in-flight wave.
                    size_t generated_fra = 0;
                    const size_t max_fra = (size_t)p.monitor->max_frame;
                    const size_t step_fra = (size_t)p.n_frames;
                    m.monitor->disable_is_done(true);
                    u.pipeline->exec(std::vector<std::function<bool()>>{
                        [&]() -> bool {
                            generated_fra += step_fra;
                            return generated_fra >= max_fra || m.monitor->fe_limit_achieved() || m.source->is_done();
                        },
                        [&]() -> bool { return m.monitor->fe_limit_achieved(); },
                        [&]() -> bool { return m.monitor->fe_limit_achieved(); }
                    });
                }
                else
                {
                    u.pipeline->exec();
                }
            }
            const auto t_exec_stop = std::chrono::steady_clock::now();
            const double elapsed_us = std::chrono::duration<double, std::micro>(t_exec_stop - t_exec_start).count();

            // display the performance (BER and FER) in the terminal
            u.terminal->final_report();

            if (p.verbose)
            {
                const unsigned long long n_fra = p.force_sequence ? u.monitor_red->get_n_analyzed_fra()
                                                                  : m.monitor->get_n_analyzed_fra();
                const double info_thr_mbps  = elapsed_us > 0.0 ? ((double)n_fra * (double)p.codec->K) / elapsed_us : 0.0;
                const double coded_thr_mbps = elapsed_us > 0.0 ? ((double)n_fra * (double)p.codec->N_cw) / elapsed_us : 0.0;
                std::cout << "# Exec_stats: ebn0=" << std::fixed << std::setprecision(2) << ebn0
                          << " fra=" << n_fra
                          << " elapsed_us=" << std::setprecision(3) << elapsed_us
                          << " info_thr_mbps=" << std::setprecision(6) << info_thr_mbps
                          << " coded_thr_mbps=" << std::setprecision(6) << coded_thr_mbps
                          << std::endl;
            }

            if (spu::tools::Signal_handler::is_sigint() || m.source->is_done())
                break;

            // reset the modules for the next SNR
            if (p.force_sequence)
            {
                u.monitor_red->reset();
                for (auto &mod : u.sequence->get_modules<spu::tools::Interface_reset>())
                    mod->reset();
            }
            else
            {
                for (auto &mod : u.pipeline->get_modules<spu::tools::Interface_reset>())
                    mod->reset();
            }
        }

        // display the statistics of the tasks (if enabled)
        if (p.stats)
        {
            if (p.force_sequence)
            {
                std::cout << "#" << std::endl;
                spu::tools::Stats::show(u.sequence->get_modules_per_types(), true);
            }
            else
            {
                auto stages = u.pipeline->get_stages();
                for (size_t s = 0; s < stages.size(); s++)
                {
                    const int n_threads = stages[s]->get_n_threads();
                    std::cout << "#" << std::endl << "# Pipeline stage " << s << " (" << n_threads << " thread(s)): " << std::endl;
                    spu::tools::Stats::show(stages[s]->get_tasks_per_types(), true);
                }
            }
        }
        std::cout << "#" << std::endl << "# End of the simulation" << std::endl;
    }

    if (p.jl_dec)
        spu::tools::Julia::exit();

    return 0;
}

void init_params(int argc, char** argv, params &p)
{
    if (std::getenv("AFF3CT_CLI_PATH") == nullptr)
        setenv("AFF3CT_CLI_PATH", "../../../../lib/aff3ct:../../../lib/aff3ct:../../lib/aff3ct:../lib/aff3ct:lib/aff3ct", 0);

    p.source   = std::unique_ptr<factory::Source          >(new factory::Source          ());
    p.codec    = std::unique_ptr<factory::Codec_repetition>(new factory::Codec_repetition());
    p.modem    = std::unique_ptr<factory::Modem           >(new factory::Modem           ());
    p.channel  = std::unique_ptr<factory::Channel         >(new factory::Channel         ());
    p.monitor  = std::unique_ptr<factory::Monitor_BFER    >(new factory::Monitor_BFER    ());
    p.sink     = std::unique_ptr<factory::Sink            >(new factory::Sink            ());
    p.terminal = std::unique_ptr<factory::Terminal        >(new factory::Terminal        ());

    std::vector<factory::Factory*> params_list = { p.source  .get(), p.codec  .get(), p.modem.get(),
                                                   p.channel .get(), p.monitor.get(), p.sink .get(),
                                                   p.terminal.get(), &p                             };

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
    tools::Header::print_parameters(params_list);
    std::cout << "#" << std::endl;
    cp.print_warnings();
}

void init_modules(const params &p, modules &m)
{
    if (factory::Modem::has_constellation(p.modem->type))
        m.constellation.reset(p.modem->build_constellation<float>());

    m.source  = std::unique_ptr<spu::module::Source      <>>(p.source ->build());
    m.codec   = std::unique_ptr<     tools ::Codec_repetition<>>(p.codec->build());
    m.modem   = std::unique_ptr<     module::Modem       <>>(p.modem  ->build(m.constellation.get()));
    m.channel = std::unique_ptr<     module::Channel     <>>(p.channel->build());
    m.monitor = std::unique_ptr<     module::Monitor_BFER<>>(p.monitor->build());
    m.sink    = std::unique_ptr<spu::module::Sink        <>>(p.sink   ->build());
    m.encoder = &m.codec->get_encoder();

    if (p.jl_dec)
    {
        m.decoder_jl = build_decoder_rep_julia(p.codec->K,
                                               p.codec->N_cw,
                                               p.n_frames,
                                               p.buffered,
                                               p.jl_ver,
                                               p.jl_path,
                                               p.force_jl_safe);
        m.decoder = m.decoder_jl.get();
    }
    else
    {
        m.decoder = &m.codec->get_decoder_siho();
    }

    m.source ->set_n_frames(p.n_frames);
    m.codec  ->set_n_frames(p.n_frames);
    m.modem  ->set_n_frames(p.n_frames);
    m.channel->set_n_frames(p.n_frames);
    m.monitor->set_n_frames(p.n_frames);
    m.sink   ->set_n_frames(p.n_frames);

    // when reading a binary file, prevent the monitor from stopping the pipeline before EOF
    if (p.source->type == "USER_BIN")
        m.monitor->disable_is_done(true);
}

void init_utils(const params &p, const modules &m, utils &u)
{
    const size_t eff_threads = p.n_threads ? p.n_threads : 1;
    const size_t eff_buf = p.interstage_buf > 0 ? (size_t)p.interstage_buf : std::max<size_t>(4, 2 * eff_threads);

    // create a sigma noise type
    u.noise = std::unique_ptr<tools::Sigma<>>(new tools::Sigma<>());
    // report the noise values (Es/N0 and Eb/N0)
    u.reporters.push_back(std::unique_ptr<spu::tools::Reporter>(new tools::Reporter_noise<>(*u.noise)));

    if (p.force_sequence)
    {
        u.sequence.reset(new spu::runtime::Sequence((*m.source)("generate"), eff_threads));
        u.monitor_red.reset(new Monitor_BFER_reduction(u.sequence->get_modules<module::Monitor_BFER<>>()));
        u.monitor_red->set_reduce_frequency(std::chrono::milliseconds(1));

        if (!p.dot_filepath.empty())
        {
            std::ofstream f(p.dot_filepath);
            u.sequence->export_dot(f);
        }

        u.reporters.push_back(std::unique_ptr<spu::tools::Reporter>(new tools::Reporter_BFER<>(*u.monitor_red)));
        u.reporters.push_back(std::unique_ptr<spu::tools::Reporter>(new tools::Reporter_throughput<>(*u.monitor_red)));

        for (auto& type : u.sequence->get_tasks_per_types()) for (auto& tsk : type)
        {
            tsk->set_debug      (false  );
            tsk->set_debug_limit(16     );
            tsk->set_stats      (p.stats);

            if (!tsk->is_debug() && !tsk->is_stats())
                tsk->set_fast(true);
        }
    }
    else
    {
        u.pipeline.reset(spu::tools::Pipeline_builder()
            .add_task_for_checking((*m.source)("generate"))
            // ------------------------------------------------------------------------------------------------------------
            .add_stage(spu::tools::Pipeline_builder::Stage_builder() // ------------------------------------------- STAGE 0
                .add_first_task((*m.source)("generate"))
                .add_last_task((*m.source)("generate"))
                .set_n_threads(1))
            // ------------------------------------------------------------------------------------------------------------
            .configure_interstage_synchro(spu::tools::Pipeline_builder::Synchro_builder() // ---------- INTER-STAGE 0 <-> 1
                .set_buffer_size(eff_buf)
                .set_active_waiting(p.active_waiting))
            // ------------------------------------------------------------------------------------------------------------
            .add_stage(spu::tools::Pipeline_builder::Stage_builder() // ------------------------------------------- STAGE 1
                .add_first_task((*m.encoder)("encode"))
                .add_last_task((*m.decoder)("decode_siho"))
                .set_n_threads(eff_threads))
            // ------------------------------------------------------------------------------------------------------------
            .configure_interstage_synchro(spu::tools::Pipeline_builder::Synchro_builder() // ---------- INTER-STAGE 1 <-> 2
                .set_buffer_size(eff_buf)
                .set_active_waiting(p.active_waiting))
            // ------------------------------------------------------------------------------------------------------------
            .add_stage(spu::tools::Pipeline_builder::Stage_builder() // ------------------------------------------- STAGE 2
                .set_first_tasks({&(*m.monitor)("check_errors"),
                                  &(*m.sink)("send_count")})
                .set_n_threads(1))
            // ------------------------------------------------------------------------------------------------------------
            .build_ptr());

        if (!p.dot_filepath.empty())
        {
            std::ofstream f(p.dot_filepath);
            u.pipeline->export_dot(f);
        }

        u.reporters.push_back(std::unique_ptr<spu::tools::Reporter>(new tools::Reporter_BFER<>(*m.monitor)));
        u.reporters.push_back(std::unique_ptr<spu::tools::Reporter>(new tools::Reporter_throughput<>(*m.monitor)));

        for (auto& type : u.pipeline->get_tasks_per_types()) for (auto& tsk : type)
        {
            tsk->set_debug      (false  );
            tsk->set_debug_limit(16     );
            tsk->set_stats      (p.stats);

            if (!tsk->is_debug() && !tsk->is_stats())
                tsk->set_fast(true);
        }
    }

    // create a terminal that will display the collected data from the reporters
    u.terminal = std::unique_ptr<spu::tools::Terminal>(p.terminal->build(u.reporters));
}