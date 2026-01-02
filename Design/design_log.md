# Design log

## 2/1/26

Goals:
- create a console application for programming, somewhat like claude's terminal
- run llm's locally and potentially fine tune them to be specialised for coding. Could also use other machine learning techniques instead of LLMs to enhance coding, because there's no way I can get claude opus performance out of the box with open source models.
- Also figure out how to get the models to determine commands to use when investigating a repo, or doing other system-wide actions - there could be a lot of use out of this beyond coding, such as automating notes, emails, and pretty much anything involving interacting with a computer.

Optional:
- potentially make a vim integration, vscode integration
- if the local AIs are successful, figure out how to expand their use. It would be nice to have LLM's used like a whatsapp bot that could schedule events for a group chat, set reminders, remember past conversations etc. There are many other uses as well.

## 2/1/26 - Architecture Session

**Decisions made**:
- Language: C++ (native llama.cpp integration, no FFI overhead, existing expertise)
- LLM Backend: llama.cpp directly (maximum control, performance)
- MVP Focus: System actions first (shell commands, file ops, process management)
- Agentic: LLM chains as many tools as needed autonomously
- Confirmation: Simple y/n for risky operations
- Workspace scoping: Optional configurable restriction

**Key architecture components**:
1. CLI Interface - REPL loop with streaming output (readline, later ftxui)
2. Core Orchestrator - Manages agentic loop and tool dispatch
3. LLM Backend - Native llama.cpp integration
4. Tool Registry - Pluggable system for shell, file, process tools
5. Context Manager - Tracks conversation history and system state

**See**: [architecture.md](architecture.md) for full design document

**Next steps**:
1. Set up CMake project with llama.cpp as git submodule
2. Get llama.cpp inference working with a small model
3. Build basic REPL loop with readline
4. Implement shell command tool with JSON parsing
5. Iterate on system prompt for reliable tool calls

**Model candidates**:
- Qwen2.5-Coder-7B-Instruct (primary target)
- DeepSeek-Coder-V2-Lite
- Mistral-7B-Instruct

**C++ Dependencies**:
- llama.cpp (submodule)
- nlohmann/json (JSON parsing)
- toml++ (config)
- fmt/spdlog (formatting/logging)
- readline (input)

**Additional design decisions**:
- Error handling: Result<T> type with configurable verbosity (--verbose flag)
- Audit logging: Optional, disabled by default, JSON Lines format
- Model management: Both manual paths and built-in `local-ai models pull` command
- Testing: Attest (custom library) for unit tests + integration tests with mock LLM