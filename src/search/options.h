#ifndef SEARCH_OPTIONS_H
#define SEARCH_OPTIONS_H

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

class Options {
    std::string filename;
    std::string generator;
    std::string search_engine;
    std::string evaluator;
    std::string state_representation;
    std::string plan_file;
    bool only_effects_opt;
    bool novelty_early_stop;
    unsigned seed;

    static std::string find_arg(const std::vector<std::string> &args,
                                const std::string &long_opt,
                                const std::string &short_opt = "")
    {
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == long_opt || (!short_opt.empty() && args[i] == short_opt)) {
                if (i + 1 < args.size())
                    return args[i + 1];
            }
            // Handle --key=value syntax
            if (args[i].rfind(long_opt + "=", 0) == 0) {
                return args[i].substr(long_opt.size() + 1);
            }
        }
        return "";
    }

    static bool has_flag(const std::vector<std::string> &args,
                         const std::string &long_opt,
                         const std::string &short_opt = "")
    {
        return std::find(args.begin(), args.end(), long_opt) != args.end() ||
               (!short_opt.empty() && std::find(args.begin(), args.end(), short_opt) != args.end());
    }

public:
    Options(int argc, char **argv)
    {
        std::vector<std::string> args(argv + 1, argv + argc);

        if (has_flag(args, "--help", "-h")) {
            std::cout << "Allowed options:\n"
                      << "  -f, --filename       Lifted task file name (default: output.lifted)\n"
                      << "  -h, --help           Display this help message\n"
                      << "  --seed               Random seed (default: 1)\n"
                      << "  -e, --evaluator      Heuristic evaluator (required)\n"
                      << "  -g, --generator      Successor generator method (required)\n"
                      << "  -s, --search         Search engine (required)\n"
                      << "  --plan-file          Plan file (default: FilePathUndefined)\n"
                      << "  --only-effects-novelty-check  Check only effects for novelty (default: "
                         "false)\n"
                      << "  --novelty-early-stop Stop evaluating novelty early (default: false)\n";
            exit(0);
        }

        filename = find_arg(args, "--filename", "-f");
        if (filename.empty())
            filename = "output.lifted";

        evaluator = find_arg(args, "--evaluator", "-e");
        generator = find_arg(args, "--generator", "-g");
        search_engine = find_arg(args, "--search", "-s");

        if (evaluator.empty() || generator.empty() || search_engine.empty()) {
            std::cerr
                << "Error: --evaluator (-e), --generator (-g), and --search (-s) are required."
                << std::endl;
            exit(1);
        }

        plan_file = find_arg(args, "--plan-file");
        if (plan_file.empty())
            plan_file = "FilePathUndefined";

        std::string seed_str = find_arg(args, "--seed");
        seed = seed_str.empty() ? 1 : static_cast<unsigned>(std::stoul(seed_str));

        only_effects_opt = has_flag(args, "--only-effects-novelty-check");
        novelty_early_stop = has_flag(args, "--novelty-early-stop");
    }

    const std::string &get_filename() const { return filename; }

    const std::string &get_successor_generator() const { return generator; }

    const std::string &get_search_engine() const { return search_engine; }

    const std::string &get_evaluator() const { return evaluator; }

    const std::string &get_plan_file() const { return plan_file; }

    bool get_only_effects_opt() const { return only_effects_opt; }

    bool get_novelty_early_stop() const { return novelty_early_stop; }

    unsigned get_seed() const { return seed; }
};

#endif  // SEARCH_OPTIONS_H
