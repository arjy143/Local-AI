#include "shell.hpp"

#include <array>
#include <cstdio>
#include <sys/wait.h>

namespace local_ai
{
    ShellResult execute_shell(const std::string& command)
    {
        //fixed size buffer for output reading. need to be aware of this in case the model decides to go crazy
        std::array<char, 4096> buffer;
        std::string output;

        //redirect stderr to stdout
        std::string full_command = command + " 2>&1";
        
        //popen could be vulnerable to shell injection so maybe change this later
        //also singlethreaded so maybe could delegate this to a worker thread
        FILE* pipe = popen(full_command.c_str(), "r");
        if (!pipe)
        {
            return ShellResult{"Failed to execute command", -1};
        }

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        {
            output += buffer.data();
        }

        //get exit status
        int status = pclose(pipe);
        int exit_code;

        if (WIFEXITED(status))
        {
            exit_code = WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            exit_code = 128 + WTERMSIG(status);
            output += "[Killed by signal " + std::to_string(WTERMSIG(status)) + "]\n";
        }
        else
        {
            exit_code = -1;
        }

        return ShellResult{output, exit_code};
    }
}
