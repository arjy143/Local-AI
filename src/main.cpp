#include "llm.hpp"
#include "shell.hpp"
#include "parser.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>
#include <string>

namespace local_ai
{
    // ANSI color codes
    namespace color
    {
        const char* RESET   = "\033[0m";
        const char* RED     = "\033[31m";
        const char* GREEN   = "\033[32m";
        const char* YELLOW  = "\033[33m";
        const char* BLUE    = "\033[34m";
        const char* MAGENTA = "\033[35m";
        const char* CYAN    = "\033[36m";
        const char* BOLD    = "\033[1m";
        const char* DIM     = "\033[2m";
    }
    const char* SYSTEM_PROMPT = R"(You are a local AI assistant running on the user's machine.
    You can execute shell commands to help accomplish tasks.

    When you need to run a command, output JSON like this:
    {"tool": "shell", "args": {"command": "your command here"}}

    After seeing the result, either:
    - Run another command if more steps are needed
    - Give your final answer as plain text (no JSON)

    Rules:
    1. Explain what you're about to do before doing it
    2. If a command fails, analyse the error and try a different approach
    3. Be concise in your responses

    Current directory: )"; 

    //no more than 20 tool calls at once
    const int MAX_ITERATIONS = 20;

    std::string build_prompt(const std::string& user_input)
    {
        std::string cwd = std::filesystem::current_path().string();
        return std::string(SYSTEM_PROMPT) + cwd + "\nUser: " + user_input + "\nAssistant: ";
    }

    std::string run_agent(local_ai::LLM& llm, const std::string& user_input)
    {
        std::string context = build_prompt(user_input);
        std::string final_response;

        for (int i = 0; i < MAX_ITERATIONS; i++)
        {
            std::cout << "\n" << color::GREEN;

            std::string response = llm.generate(context, 1024, [](const std::string& token)
                    {
                        std::cout << token << std::flush;
                    });
            std::cout << color::RESET;
            
            auto tool_call = local_ai::parse_tool_call(response);

            if(tool_call.has_value())
            {
                auto cmd_it = tool_call->args.find("command");
                if (cmd_it != tool_call->args.end())
                {
                    std::cout << color::YELLOW << "[Executing: " << cmd_it->second << "]" << color::RESET << "\n";
                    auto result = local_ai::execute_shell(cmd_it->second);

                    std::cout << color::CYAN << result.output << color::RESET;
                    if (result.exit_code != 0)
                    {
                        std::cout << color::RED << "[Exit code: " << result.exit_code << "]" << color::RESET << "\n";
                    }


                    context += response + "[Tool output]" + result.output;
                    if (result.exit_code != 0) 
                    {
                        context += "[Exit code: " + std::to_string(result.exit_code) + "]";
                    }
                    context += "Assistant: ";
                }
            }
            else 
            {
                final_response = response;
                break;                                   
            }
        }
        return final_response;
    }

    void print_usage(const char* prog_name)
    {
        std::cerr << "Usage: " << prog_name << " --model <path-to-gguf>\n";
        std::cerr << "Options:\n";
        std::cerr << "  --model <path>   Path to GGUF model file (required)\n";
        std::cerr << "  --ctx <size>     Context size (default: 4096)\n";
        std::cerr << "  --gpu <layers>   GPU layers (-1 = all, 0 = CPU only)\n";
        std::cerr << "  --help           Show this help\n";
    }
}

int main(int argc, char** argv)
{
    std::string model_path;
    int n_ctx = 4096;
    int num_gpu_layers = -1;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--model" && i + 1 < argc) 
        {
            model_path = argv[++i];                            
        }
        else if (arg == "--ctx" && i + 1 < argc) 
        {
            n_ctx = std::stoi(argv[++i]);                            
        }
        else if (arg == "--gpu" && i + 1 < argc)
        {
            num_gpu_layers = std::stoi(argv[++i]);
        }
        else if (arg == "--help" || arg == "-h") 
        {
            local_ai::print_usage(argv[0]);
            return 0;                                        
        }            
    }

    if (model_path.empty()) 
    {
        local_ai::print_usage(argv[0]);
        return 1;                    
    }
    //load the model
    std::cout << local_ai::color::DIM << "Loading model: " << model_path << "..." << local_ai::color::RESET << "\n";
    local_ai::LLM llm;

    if (!llm.load(model_path, n_ctx, num_gpu_layers))
    {
        std::cerr << local_ai::color::RED << "Failed to load model" << local_ai::color::RESET << "\n";
        return 1;
    }
    std::cout << local_ai::color::GREEN << "Model loaded successfully." << local_ai::color::RESET << "\n";
    std::cout << local_ai::color::BOLD << local_ai::color::CYAN << "Local AI Assistant" << local_ai::color::RESET;
    std::cout << local_ai::color::DIM << " (type 'exit' to quit)" << local_ai::color::RESET << "\n";
    std::cout << local_ai::color::CYAN << "==========================================" << local_ai::color::RESET << "\n";
    //Main loop
                                                                 
    // Colored prompt
    std::string prompt = std::string(local_ai::color::BOLD) + local_ai::color::BLUE + "> " + local_ai::color::RESET;

    while (true)
    {
        char* line = readline(prompt.c_str());
        if (!line)
        {
            std::cout << "\n";
            break;
        }
        std::string input(line);
        free(line);

        size_t start = input.find_first_not_of(" \t\n\r");
        size_t end = input.find_last_not_of(" \t\n\r");

        if (start == std::string::npos)
        {
            continue;
        }
        input = input.substr(start, end - start + 1);

        if (input.empty())
        {
            continue;
        }
        
        add_history(input.c_str());

        if (input == "exit" || input == "quit")
        {
            break;
        }
        run_agent(llm, input);
        std::cout << "\n";
    }

    std::cout << local_ai::color::DIM << "Goodbye!" << local_ai::color::RESET << "\n";
    return 0;
}
