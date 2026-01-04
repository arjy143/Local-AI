#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

echo -e "${BOLD}${CYAN}================================${RESET}"
echo -e "${BOLD}${CYAN}   Local-AI Installation Script ${RESET}"
echo -e "${BOLD}${CYAN}================================${RESET}"
echo

# Check if running on Ubuntu/Debian
if ! command -v apt &> /dev/null; then
    echo -e "${RED}Error: This script requires apt (Ubuntu/Debian).${RESET}"
    echo "For other distributions, please install dependencies manually."
    exit 1
fi

# Install dependencies
echo -e "${YELLOW}[1/4] Installing dependencies...${RESET}"
sudo apt update
sudo apt install -y build-essential cmake libreadline-dev git wget

# Check for CUDA
echo
echo -e "${YELLOW}[2/4] Checking for CUDA...${RESET}"
if command -v nvcc &> /dev/null; then
    echo -e "${GREEN}CUDA found: $(nvcc --version | grep release | awk '{print $6}')${RESET}"
    CUDA_AVAILABLE=true
else
    echo -e "${YELLOW}CUDA not found. Building with CPU-only support.${RESET}"
    echo -e "To enable GPU acceleration, install CUDA toolkit:"
    echo -e "  sudo apt install nvidia-cuda-toolkit"
    CUDA_AVAILABLE=false
fi

# Build project
echo
echo -e "${YELLOW}[3/4] Building project...${RESET}"
mkdir -p build
cd build
cmake ..
make -j$(nproc)
cd ..

echo -e "${GREEN}Build complete!${RESET}"

# Download model
echo
echo -e "${YELLOW}[4/4] Model setup${RESET}"
MODEL_PATH="qwen2.5-coder-7b-instruct-q5_k_m.gguf"
MODEL_URL="https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct-GGUF/resolve/main/qwen2.5-coder-7b-instruct-q5_k_m.gguf"

if [ -f "$MODEL_PATH" ] || [ -f "build/$MODEL_PATH" ]; then
    echo -e "${GREEN}Model already exists, skipping download.${RESET}"
else
    echo -e "Would you like to download Qwen2.5-Coder-7B? (~5.4GB)"
    echo -e "This is the recommended model for 8GB VRAM GPUs."
    read -p "Download model? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${CYAN}Downloading model...${RESET}"
        wget -q --show-progress "$MODEL_URL"
        echo -e "${GREEN}Model downloaded successfully.${RESET}"
    else
        echo -e "${YELLOW}Skipping model download.${RESET}"
        echo -e "You can download a model manually from Hugging Face."
    fi
fi

# Done
echo
echo -e "${BOLD}${GREEN}================================${RESET}"
echo -e "${BOLD}${GREEN}   Installation Complete!       ${RESET}"
echo -e "${BOLD}${GREEN}================================${RESET}"
echo
echo -e "To run Local-AI:"
echo -e "  ${CYAN}cd build${RESET}"
echo -e "  ${CYAN}./local-ai --model ../qwen2.5-coder-7b-instruct-q5_k_m.gguf${RESET}"
echo
if [ "$CUDA_AVAILABLE" = false ]; then
    echo -e "${YELLOW}Note: Running without GPU. For better performance, install CUDA.${RESET}"
fi
