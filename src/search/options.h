#ifndef SEARCH_OPTIONS_H
#define SEARCH_OPTIONS_H

#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

class Options {
    std::string filename;
    std::string generator;
    std::string search_engine;
    std::string evaluator;
    std::string state_representation;
    std::string plan_file;
    bool full_novelty_check;
    bool novelty_early_stop;
    unsigned seed;

    static void print_help()
    {
        std::cout << "Allowed options:\n"
                  << "  -f, --filename       Lifted task file name (default: output.lifted)\n"
                  << "  -h, --help           Display this help message\n"
                  << "  --seed               Random seed (default: 1)\n"
                  << "  -e, --evaluator      Heuristic evaluator (required)\n"
                  << "  -g, --generator      Successor generator method (required)\n"
                  << "  -s, --search         Search engine (required)\n"
                  << "  --plan-file          Plan file (default: FilePathUndefined)\n"
                  << "  --full-novelty-check[=BOOL] Use full-state novelty checking "
                     "(default: false)\n"
                  << "  --novelty-early-stop[=BOOL] Stop evaluating novelty early "
                     "(default: false)\n";
    }

    static bool is_option_name(const std::string &arg)
    {
        return !arg.empty() && arg[0] == '-';
    }

    static bool is_named_option(const std::string &arg,
                                const std::string &long_opt,
                                const std::string &short_opt = "")
    {
        return arg == long_opt || (!short_opt.empty() && arg == short_opt);
    }

    static bool has_inline_value(const std::string &arg, const std::string &opt)
    {
        return arg.rfind(opt + "=", 0) == 0;
    }

    static std::string take_required_value(const std::vector<std::string> &args,
                                           std::size_t &i,
                                           const std::string &long_opt,
                                           const std::string &short_opt = "")
    {
        const std::string &arg = args[i];
        if (has_inline_value(arg, long_opt)) {
            return arg.substr(long_opt.size() + 1);
        }
        if (!short_opt.empty() && has_inline_value(arg, short_opt)) {
            return arg.substr(short_opt.size() + 1);
        }
        if (i + 1 >= args.size()) {
            throw std::runtime_error("Missing value for option " + long_opt + ".");
        }
        if (is_option_name(args[i + 1])) {
            throw std::runtime_error("Missing value for option " + long_opt + ".");
        }
        ++i;
        return args[i];
    }

    static bool parse_bool_value(const std::string &value, const std::string &opt_name)
    {
        if (value == "1" || value == "true" || value == "True" || value == "yes" ||
            value == "on") {
            return true;
        }
        if (value == "0" || value == "false" || value == "False" || value == "no" ||
            value == "off") {
            return false;
        }
        throw std::runtime_error("Invalid boolean value '" + value + "' for option " + opt_name +
                                 ".");
    }

    static bool take_bool_value(const std::vector<std::string> &args,
                                std::size_t &i,
                                const std::string &long_opt)
    {
        const std::string &arg = args[i];
        if (has_inline_value(arg, long_opt)) {
            return parse_bool_value(arg.substr(long_opt.size() + 1), long_opt);
        }
        if (i + 1 < args.size() && !is_option_name(args[i + 1])) {
            ++i;
            return parse_bool_value(args[i], long_opt);
        }
        return true;
    }

    static unsigned parse_unsigned_value(const std::string &value, const std::string &opt_name)
    {
        try {
            std::size_t parsed = 0;
            unsigned long result = std::stoul(value, &parsed);
            if (parsed != value.size()) {
                throw std::runtime_error("");
            }
            if (result > std::numeric_limits<unsigned>::max()) {
                throw std::runtime_error("");
            }
            return static_cast<unsigned>(result);
        } catch (const std::exception &) {
            throw std::runtime_error("Invalid unsigned integer value '" + value +
                                     "' for option " + opt_name + ".");
        }
    }

    [[noreturn]] static void exit_with_option_error(const std::string &message)
    {
        std::cerr << "Error with command-line options: " << message << std::endl << std::endl;
        print_help();
        exit(1);
    }

public:
    Options(int argc, char **argv)
        : filename("output.lifted"),
          plan_file("FilePathUndefined"),
          full_novelty_check(false),
          novelty_early_stop(false),
          seed(1)
    {
        std::vector<std::string> args(argv + 1, argv + argc);

        try {
            for (std::size_t i = 0; i < args.size(); ++i) {
                const std::string &arg = args[i];
                if (is_named_option(arg, "--help", "-h")) {
                    print_help();
                    exit(0);
                }
                else if (is_named_option(arg, "--filename", "-f") ||
                         has_inline_value(arg, "--filename") || has_inline_value(arg, "-f")) {
                    filename = take_required_value(args, i, "--filename", "-f");
                }
                else if (is_named_option(arg, "--evaluator", "-e") ||
                         has_inline_value(arg, "--evaluator") || has_inline_value(arg, "-e")) {
                    evaluator = take_required_value(args, i, "--evaluator", "-e");
                }
                else if (is_named_option(arg, "--generator", "-g") ||
                         has_inline_value(arg, "--generator") || has_inline_value(arg, "-g")) {
                    generator = take_required_value(args, i, "--generator", "-g");
                }
                else if (is_named_option(arg, "--search", "-s") ||
                         has_inline_value(arg, "--search") || has_inline_value(arg, "-s")) {
                    search_engine = take_required_value(args, i, "--search", "-s");
                }
                else if (is_named_option(arg, "--plan-file") || has_inline_value(arg, "--plan-file")) {
                    plan_file = take_required_value(args, i, "--plan-file");
                }
                else if (is_named_option(arg, "--seed") || has_inline_value(arg, "--seed")) {
                    seed = parse_unsigned_value(take_required_value(args, i, "--seed"), "--seed");
                }
                else if (is_named_option(arg, "--full-novelty-check") ||
                         has_inline_value(arg, "--full-novelty-check")) {
                    full_novelty_check = take_bool_value(args, i, "--full-novelty-check");
                }
                else if (is_named_option(arg, "--novelty-early-stop") ||
                         has_inline_value(arg, "--novelty-early-stop")) {
                    novelty_early_stop = take_bool_value(args, i, "--novelty-early-stop");
                }
                else if (is_option_name(arg)) {
                    throw std::runtime_error("Unknown option '" + arg + "'.");
                }
                else {
                    throw std::runtime_error("Unexpected positional argument '" + arg + "'.");
                }
            }
        } catch (const std::exception &ex) {
            exit_with_option_error(ex.what());
        }

        if (evaluator.empty() || generator.empty() || search_engine.empty()) {
            exit_with_option_error(
                "--evaluator (-e), --generator (-g), and --search (-s) are required.");
        }
    }

    const std::string &get_filename() const { return filename; }

    const std::string &get_successor_generator() const { return generator; }

    const std::string &get_search_engine() const { return search_engine; }

    const std::string &get_evaluator() const { return evaluator; }

    const std::string &get_plan_file() const { return plan_file; }

    bool use_full_novelty_check() const { return full_novelty_check; }

    bool get_novelty_early_stop() const { return novelty_early_stop; }

    unsigned get_seed() const { return seed; }
};

#endif  // SEARCH_OPTIONS_H
