# Local-AI

An application that lets you run an LLM model locally, with no need for internet. It can write code, execute shell scripts, and even chain actions together, all the while explaining what it's thought process is.

I recommend using qwen 2.5 coder, with only 7 billion parameters, which is good for GPU's with only 8GB of VRAM. If you have more VRAM, it may be feasible to use larger and more powerful models.
However, it's obviously not going to be nearly as good as an industry leading application such as Claude Code, which uses much better models with larger context windows. Even so, with fine tuning and good prompting, even smaller models can be quite useful, and being able to run them locally can be a powerful tool.

This project uses llama.cpp as the inference engine, and nlohmann.json for JSON parsing. I also recommend enabling CUDA if you have an NVIDIA GPU. You will also need to download the actual LLM .gguf file that you want to use. In my case, qwen 2.5 coder took about 5GB of space.

## Example

<img width="1152" height="990" alt="image" src="https://github.com/user-attachments/assets/f03f4f0e-a1f3-4b79-8b80-3623305cd33d" />

---

## DONE

- [x] LLM loading and inference via llama.cpp
- [x] Streaming token output
- [x] Agentic loop with tool calling (max 20 iterations)
- [x] JSON tool call parsing from LLM output
- [x] Shell command execution with exit code handling
- [x] stderr/stdout capture and merging
- [x] Interactive REPL with readline and history
- [x] CLI argument parsing (--model, --ctx, --gpu)
- [x] CUDA/GPU acceleration support

## TODO

- [ ] Add `read_file` tool (with 50KB size limit)
- [ ] Add `write_file` tool
- [ ] Add `web_search` tool (web scraping)
- [ ] Conversation history persistence (~/.local-ai-history.json)
- [ ] Safety confirmations for dangerous commands (rm -rf, sudo, chmod etc.)
- [ ] Output truncation 

- [ ] Colored terminal output (user/assistant/tool differentiation) or maybe just make a full on terminal ui app
- [ ] Working directory tracking (persistent `cd`)
- [ ] Config file support (~/.local-ai.json)
- [ ] `--verbose` flag for debugging (token counts, timing, parsed JSON)
- [ ] Graceful error recovery for malformed LLM output
- [ ] `clear` command to reset conversation

- [ ] Unit tests for parser and shell execution
- [ ] Streaming tool output (real-time command output)
- [ ] Multiple backend support (Ollama, other GGUF models) or maybe make a custom backend?
- [ ] Token budget management and context warnings

---

## Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                            REPL                                │
│                        (main.cpp)                              │
│                                                                │
│   ┌─────────┐    ┌─────────────┐    ┌──────────────────────┐   │
│   │  User   │───>│   Input     │───>│   build_prompt()     │   │
│   │  Input  │    │  Handling   │    │   + System Prompt    │   │
│   └─────────┘    └─────────────┘    └──────────┬───────────┘   │
│                                                │               │
└────────────────────────────────────────────────┼───────────────┘
                                                 │
                                                 v
┌─────────────────────────────────────────────────────────────────┐
│                        Agent Loop                               │
│                      (run_agent())                              │
│                                                                 │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │                                                          │  │
│   │  ┌─────────────┐     ┌─────────────┐     ┌───────────┐   │  │
│   │  │   LLM       │────>│   Parser    │────>│  Tool     │   │  │
│   │  │  Generate   │     │  Extract    │     │  Found?   │   │  │
│   │  │  (llm.cpp)  │     │   JSON      │     │           │   │  │
│   │  └─────────────┘     │(parser.cpp) │     └─────┬─────┘   │  │
│   │        ^             └─────────────┘           │         │  │
│   │        │                                       │         │  │
│   │        │              ┌────────────────────────┴────┐    │  │
│   │        │              │                             │    │  │
│   │        │              v                             v    │  │
│   │        │        ┌───────────┐               ┌─────────┐  │  │
│   │        │        │   Yes     │               │   No    │  │  │
│   │        │        └─────┬─────┘               └────┬────┘  │  │
│   │        │              │                          │       │  │
│   │        │              v                          v       │  │
│   │        │     ┌─────────────────┐        ┌────────────┐   │  │
│   │        │     │  Execute Tool   │        │   Return   │   │  │
│   │        │     │  (shell.cpp)    │        │   Final    │   │  │
│   │        │     │                 │        │  Response  │   │  │
│   │        │     │  ┌───────────┐  │        └────────────┘   │  │
│   │        │     │  │   popen() │  │                         │  │
│   │        │     │  │  Execute  │  │                         │  │
│   │        │     │  │  Command  │  │                         │  │
│   │        │     │  └───────────┘  │                         │  │
│   │        │     └────────┬────────┘                         │  │
│   │        │              │                                  │  │
│   │        │              v                                  │  │
│   │        │     ┌─────────────────┐                         │  │
│   │        └─────│ Append Result   │                         │  │
│   │              │  to Context     │                         │  │
│   │              └─────────────────┘                         │  │
│   │                                                          │  │
│   │                  (max 20 iterations)                     │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                        LLM Engine                               │
│                        (llm.cpp)                                │
│                                                                 │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────────┐     │
│   │   Load      │    │  Tokenise   │    │    Generate     │     │
│   │   Model     │    │   Prompt    │    │    (Sample +    │     │
│   │  (GGUF)     │    │             │    │     Decode)     │     │
│   └─────────────┘    └─────────────┘    └─────────────────┘     │
│                                                                 │
│   Backend: llama.cpp  |  GPU: CUDA  |  Sampling: temp=0.7       │
└─────────────────────────────────────────────────────────────────┘

Tool Call Format:
┌─────────────────────────────────────────┐
│  {"tool": "shell", "args": {            │
│      "command": "ls -la"                │
│  }}                                     │
└─────────────────────────────────────────┘
```
