# Local-AI: MVP Design Document

## Overview

A CLI that takes user requests, uses a local LLM to decide what shell commands to run, executes them, and returns results. The LLM runs autonomously until the task is complete.

---

## The Core Loop

```
User types request
       ↓
Send to LLM with system prompt
       ↓
LLM outputs JSON: {"tool": "shell", "args": {"command": "ls -la"}}
       ↓
Parse JSON, run command
       ↓
Send command output back to LLM
       ↓
LLM either:
  - Calls another tool → loop back
  - Gives final answer → show to user
```

---

## Architecture

```
┌─────────────────────────────────────────┐
│              main.cpp                    │
│         (REPL + orchestrator)            │
└─────────────────────────────────────────┘
                    │
        ┌───────────┼───────────┐
        ▼           ▼           ▼
┌─────────────┐ ┌─────────┐ ┌─────────┐
│  llm.cpp    │ │parser.cpp│ │shell.cpp│
│(llama.cpp)  │ │(JSON)    │ │(exec)   │
└─────────────┘ └─────────┘ └─────────┘
```

---

## Components

| Component | What it does | Complexity |
|-----------|--------------|------------|
| **main.cpp** | REPL loop: read input, call agent, print output | Simple |
| **llm.cpp** | Load GGUF model, send prompt, get response | Medium |
| **parser.cpp** | Extract `{"tool": "...", "args": {...}}` from LLM output | Simple |
| **shell.cpp** | Run shell command, capture stdout/stderr | Simple |

---

## Agent Loop Pseudocode

```cpp
std::string run_agent(const std::string& user_input) {
    std::string context = SYSTEM_PROMPT;
    context += "\nUser: " + user_input;

    int iterations = 0;
    const int MAX_ITERATIONS = 20;

    while (iterations++ < MAX_ITERATIONS) {
        // Generate LLM response
        std::string response = llm_generate(context);

        // Try to parse tool call
        auto tool_call = parse_tool_call(response);

        if (tool_call.has_value()) {
            // Execute the tool
            std::string result = execute_shell(tool_call->command);

            // Add to context and continue
            context += "\nAssistant: " + response;
            context += "\nTool result: " + result;
        } else {
            // No tool call = final answer
            return response;
        }
    }

    return "Max iterations reached";
}
```

---

## System Prompt

```
You are a local AI assistant running on the user's machine.
You can execute shell commands to help accomplish tasks.

When you need to run a command, output JSON like this:
{"tool": "shell", "args": {"command": "your command here"}}

After seeing the result, either:
- Run another command if more steps are needed
- Give your final answer as plain text (no JSON)

Rules:
1. Explain what you're about to do before doing it
2. If a command fails, analyze the error and try a different approach
3. Be concise in your responses

Current directory: {cwd}
```

---

## Tool Call Parsing

Find JSON objects in LLM output that match the tool call format:

```cpp
struct ToolCall {
    std::string tool;
    std::string command;
};

std::optional<ToolCall> parse_tool_call(const std::string& response) {
    // Find {"tool": "shell", "args": {"command": "..."}}
    // Use nlohmann::json or regex

    auto start = response.find("{\"tool\"");
    if (start == std::string::npos) return std::nullopt;

    // Find matching closing brace
    // Parse JSON
    // Extract tool name and command

    return ToolCall{...};
}
```

---

## Shell Execution

```cpp
std::string execute_shell(const std::string& command) {
    std::array<char, 4096> buffer;
    std::string result;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "Error: failed to run command";

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int status = pclose(pipe);
    if (status != 0) {
        result += "\n[Exit code: " + std::to_string(status) + "]";
    }

    return result;
}
```

---

## File Structure

```
local-ai/
├── CMakeLists.txt
├── external/
│   └── llama.cpp/          # Git submodule
└── src/
    ├── main.cpp            # REPL + orchestrator
    ├── llm.hpp
    ├── llm.cpp             # llama.cpp wrapper
    ├── parser.hpp
    ├── parser.cpp          # JSON extraction
    ├── shell.hpp
    └── shell.cpp           # Command execution
```

---

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(local-ai VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# llama.cpp as submodule
add_subdirectory(external/llama.cpp)

# Fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Main executable
add_executable(local-ai
    src/main.cpp
    src/llm.cpp
    src/parser.cpp
    src/shell.cpp
)

target_include_directories(local-ai PRIVATE src)
target_link_libraries(local-ai PRIVATE
    llama
    nlohmann_json::nlohmann_json
    readline
)
```

---

## Build Order

1. **Get llama.cpp working standalone** - load model, generate "hello world"
2. **Add REPL** - loop that reads input, passes to LLM, prints output
3. **Add JSON parsing** - detect tool calls in LLM output
4. **Add shell tool** - execute commands, capture output
5. **Close the loop** - feed tool output back to LLM until final answer

---

## MVP Scope

### Include
- Single tool: `shell` (run any command)
- Basic REPL (readline for input)
- llama.cpp model loading
- Simple JSON parsing
- Stream LLM output to terminal

### Exclude (add later)
- Config files
- Model downloading
- Workspace restrictions
- Audit logging
- Multiple tool types
- Confirmation prompts
- Error recovery/retry logic

---

## Model

**Target**: Qwen2.5-Coder-7B-Instruct (Q5_K_M quantization)

User must download manually for MVP:
```bash
# Download from HuggingFace
wget https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct-GGUF/resolve/main/qwen2.5-coder-7b-instruct-q5_k_m.gguf
```

Pass model path as CLI argument:
```bash
./local-ai --model /path/to/model.gguf
```

---

## Example Session

```
$ ./local-ai --model qwen2.5-coder-7b-instruct-q5_k_m.gguf

> What files are in the current directory?

I'll list the files for you.
{"tool": "shell", "args": {"command": "ls -la"}}

[Tool output]
total 24
drwxr-xr-x  5 user user 4096 Jan  2 10:00 .
drwxr-xr-x 10 user user 4096 Jan  2 09:00 ..
-rw-r--r--  1 user user  156 Jan  2 10:00 CMakeLists.txt
drwxr-xr-x  2 user user 4096 Jan  2 10:00 src
-rw-r--r--  1 user user   42 Jan  2 09:30 README.md

The current directory contains:
- CMakeLists.txt - Build configuration
- src/ - Source code directory
- README.md - Documentation

> Find all TODO comments in src/

{"tool": "shell", "args": {"command": "grep -rn 'TODO' src/"}}

[Tool output]
src/main.cpp:45:    // TODO: Add error handling
src/llm.cpp:23:     // TODO: Implement streaming

Found 2 TODO comments:
1. src/main.cpp:45 - Add error handling
2. src/llm.cpp:23 - Implement streaming
```

---

## Next Steps After MVP

Once the basic loop works, iterate by adding:

1. **Streaming output** - show tokens as they generate
2. **Confirmation prompts** - ask y/n before dangerous commands
3. **Config file** - model path, context length, etc.
4. **More tools** - read_file, write_file, search
5. **Error handling** - Result<T> type, retries
6. **Model management** - `local-ai models pull`
