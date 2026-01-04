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
        
    }
          
  
    bool LLM::is_loaded() const
    {
        return impl_->model != nullptr && impl_->ctx != nullptr;
    }
} 
