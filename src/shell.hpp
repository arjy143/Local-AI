#pragma once
#include <string>

namespace local_ai
{
    struct ShellResult
    {
        std::string output;
        int exit_code;
    };

    ShellResult execute_shell(const std::string& command);
}
