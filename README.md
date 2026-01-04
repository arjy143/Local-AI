# Local-AI

An application that lets you run an LLM model locally, with no need for internet. It can write code, execute shell scripts, and even chain actions together, all the while explaining what it's thought process is.

I recommend using qwen 2.5 coder, with only 7 billion parameters, which is good for GPU's with only 8GB of VRAM. If you have more VRAM, it may be feasible to use larger and more powerful models.
However, it's obviously not going to be nearly as good as an industry leading application such as Claude Code, which uses much better models with larger context windows. Even so, with fine tuning and good prompting, even smaller models can be quite useful, and being able to run them locally can be a powerful tool.

This project uses llama.cpp as the inference engine, and nlohmann.json for JSON parsing. I also recommend enabling CUDA if you have an NVIDIA GPU. You will also need to download the actual LLM .gguf file that you want to use. In my case, qwen 2.5 coder took about 5GB of space.
