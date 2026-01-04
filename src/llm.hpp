#pragma once
#include <functional>
#include <memory>
#include <string>

namespace local_ai
{

    class LLM
    {
        public:
            //the llm will take a token/chunk and return nothing until it's ready - this streaming approach is a lot more efficient
            using stream_callback = std::function<void(const std::string&)>;

            LLM();
            ~LLM();
            //non copyable, so prevent copy constructor and copy assignment
            LLM(const LLM&) = delete;
            LLM& operator=(const LLM&) = delete;
            //it can move
            LLM(LLM&&) noexcept;
            LLM& operator=(LLM&&) noexcept;

            //function to load a gguf LLM file. default context = 4096 tokens, default gpu = -1, meaning all layers are offloaded to gpu.
            bool load(const std::string& model_path, int context = 4096, int num_gpu_layers = -1);
            
            //function to generate text from a prompt
            std::string generate(const std::string& prompt, int max_tokens = 1024, stream_callback callback = nullptr);

            bool is_loaded() const;

        private:
            //pimpl idiom to hide ugly interface
            struct Impl;
            std::unique_ptr<Impl> impl_;

    };
}
