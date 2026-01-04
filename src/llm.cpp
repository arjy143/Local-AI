#include "llm.hpp"
#include "llama.h"

//common.h contains helper functions and utilities
#include "common.h"

#include <iostream>
#include <vector>

namespace local_ai
{
    struct LLM::Impl 
    {
        //model represents loaded weights
        llama_model* model = nullptr;
        //ctx represents kv cache, token history, inference state
        llama_context* ctx = nullptr;
        const llama_vocab* vocab = nullptr;
        int context = 4096;

        ~Impl()
        { 
            if (ctx)
            {  
                llama_free(ctx);
            }   
            if(model)
            {  
                llama_model_free(model);
            }  
        }  
    };  
      
    //allocate the private implementation, but its still empty at this point
    LLM::LLM() : impl_(std::make_unique<Impl>()){}
    LLM::~LLM() = default;
  
    //move - default is just to move the unique ptr
    LLM::LLM(LLM&&) noexcept = default;
    LLM& LLM::operator=(LLM&&) noexcept = default;
  
    bool LLM::load(const std::string& model_path, int context, int num_gpu_layers)
    {    
        //initialise global state, set up cuda. call once per process 
        llama_backend_init(); 
    
        llama_model_params model_params = llama_model_default_params(); 
        model_params.n_gpu_layers = num_gpu_layers; 
  
        impl_->model = llama_model_load_from_file(model_path.c_str(), model_params);
        if(!impl_->model) 
        {  
            std::cerr << "Failed to load model from the file: " << model_path << "\n";
            return false; 
        }  
    
        impl_->vocab = llama_model_get_vocab(impl_->model);  
          
        llama_context_params context_params = llama_context_default_params(); 
        context_params.n_ctx = context; 
        context_params.n_batch = 512; 
          
        impl_->ctx = llama_init_from_model(impl_->model, context_params); 
        if (!impl_->ctx) 
        {  
            std::cerr << "Failed to create context\n"; 
            llama_model_free(impl_->model); 
            impl_->model = nullptr; 
            return false;   
        }    
           
        impl_->context = context; 
        return true;   
                 
    } 
     
    //function to generate text from a prompt 
    std::string LLM::generate(const std::string& prompt, int max_tokens, stream_callback callback)
    {
       //tokenise, evaluate, sample, decode, repeat 
       if (!is_loaded())
       {
           return "Model failed to load";
       }

       //tokenise
       std::vector<llama_token> tokens(prompt.size() + 32);
       int n_tokens = llama_tokenize(
               impl_->vocab,
               prompt.c_str(),
               prompt.size(),
               tokens.data(),
               tokens.size(),
               true, true //add beginning of sequence token, and parse special tokens
               );

       //if the buffer was too small, resize and try again
       if (n_tokens < 0)
       {
           tokens.resize(-n_tokens);
           n_tokens = llama_tokenize(
                   impl_->vocab,
                   prompt.c_str(),
                   prompt.size(),
                   tokens.data(),
                   tokens.size(),
                   true, true //add beginning of sequence token, and parse special tokens
                   );
       }

       //if it happens again, just fail
       if (n_tokens < 0)
       {
           return "Failed to tokenise";
       }
       tokens.resize(n_tokens);

       //if the prompt does not fit in the context window
       if (n_tokens > impl_->context - 4)
       {
           return "Prompt did not fit in context window";
       }
       //maybe remove the below if we want to have memory across generations
       llama_kv_cache_clear(impl_->ctx);

       llama_batch batch = llama_batch_init(512, 0, 1);

       //loop to chunk the prompt to fit gpu memory and match llama.cpp expectations
       for (int i = 0; i < n_tokens; i += 512)
       {
           batch.n_tokens = 0;
           int batch_end = std::min(i+512, n_tokens);
           for (int j = i; j < batch_end; j++)
           {
               batch.token[batch.n_tokens] = tokens[j];
               batch.pos[batch.n_tokens] = j;
               batch.n_seq_id[batch.n_tokens] = 1;
               batch.seq_id[batch.n_tokens][0] = 0;

               //get logits for last token only to save compute
               batch.logits[batch.n_tokens] = (j == n_tokens -1);
               batch.n_tokens++;
           }

           //llama_decode will run the model, fill kv cache
           if (llama_decode(impl_->ctx, batch) != 0)
           {
               llama_batch_free(batch);
               return "Failed to process prompt.";
           }
       }

       //now to generate new tokens - autoregressive loop
       std::string result;
       int n_cur = n_tokens;

       llama_sampler* sampler = llama_sampler_chain_init(
               llama_sampler_chain_default_params()
               );

       //control settings
       llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.7f));
       //top p set to 90% to filter out unlikely tokens
       llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.9f, 1));
       //fixed seed
       llama_sampler_chain_add(sampler, llama_sampler_init_dist(1));

       //stop when eos reached, or when max tokens reached
       while (n_cur < n_tokens + max_tokens)
       {
           llama_token new_token = llama_sampler_sample(sampler, impl_->ctx, -1);

           //end of generation
           if (llama_vocab_is_eog(impl_->vocab, new_token))
           {
               break;
           }

           char buffer[256];
           //convert token to string
           int n = llama_token_to_piece(impl_->vocab, new_token, buffer, sizeof(buffer), 0, true);
           if (n > 0)
           {
               std::string piece(buffer, n);
               result += piece;

               if (callback)
               {
                   //use callback for streaming, so it feels nice to use
                   callback(piece);
               }
           }
           batch.n_tokens = 0;
           batch.token[0] = new_token;
           batch.pos[0] = n_cur;
           batch.n_seq_id[0] = 1;
           batch.seq_id[0][0] = 0;
           batch.logits[0] = true;
           batch.n_tokens = 1;
            
           if (llama_decode(impl_->ctx, batch) != 0)
           {
               break;
           }
           n_cur++;
       }

       llama_sampler_free(sampler);
       llama_batch_free(batch);

       return result;
}
          
  
    bool LLM::is_loaded() const
    {
        return impl_->model != nullptr && impl_->ctx != nullptr;
    }
} 
