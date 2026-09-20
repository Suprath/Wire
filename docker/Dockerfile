# Dockerfile for Project Wire Cross-Compilation & Microkit Build Environment
# Target Architecture: ARM64 (aarch64) seL4 / Microkit / LionsOS on QEMU virt
# Host Platform Support: Apple Silicon (ARM64) & Linux x86_64/ARM64

FROM ubuntu:22.04

LABEL maintainer="Project Wire Developer <suprath@projectwire.dev>"
LABEL description="Reproducible build environment for seL4 Microkit ARM64 Project Wire"

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# Install foundational build tools, LLVM 18, GNU ARM cross toolchain, Python, and QEMU
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    curl \
    git \
    gnupg \
    ninja-build \
    cmake \
    device-tree-compiler \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    libssl-dev \
    python3 \
    python3-pip \
    python3-dev \
    python3-setuptools \
    qemu-system-aarch64 \
    qemu-utils \
    libglib2.0-dev \
    libpixman-1-dev \
    && rm -rf /var/lib/apt/lists/*

# Install LLVM/Clang 18 for modern C++20 memory-safe compilation and cross-targeting
RUN curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /etc/apt/trusted.gpg.d/llvm.gpg && \
    echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-18 main" > /etc/apt/sources.list.d/llvm-18.list && \
    apt-get update && apt-get install -y --no-install-recommends \
    clang-18 \
    lld-18 \
    llvm-18 \
    clang-tools-18 \
    && rm -rf /var/lib/apt/lists/*

# Set Clang 18 as default system compiler
ENV CC=clang-18
ENV CXX=clang++-18

# Install Python dependencies required by seL4 build system and Microkit
RUN pip3 install --no-cache-dir \
    sel4-deps \
    jinja2 \
    pycparser \
    ply \
    colorama \
    pyelftools

# Create workspace directory
WORKDIR /workspace

# Set entry point
CMD ["/bin/bash"]
