#ifndef SED_UTILS_H_
#define SED_UTILS_H_

#include "index.h"
#include <filesystem>

namespace sed
{

/// Options from the commandline arguments.
struct Options
{
    std::string input_filename;
    int optimization_level;
    bool aggressive_opt;
    std::optional<std::string> output_file;
    std::optional<std::string> token_file;
    std::optional<std::string> ast_file;
    std::optional<std::string> ir_file;
};

Options parse_args(int argc, char **argv);

} // namespace sed

#endif