FROM ubuntu:22.04

# Stop interactive prompts during install
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    python3 \
    python3-pip \
    tree \
    && rm -rf /var/lib/apt/lists/*

# Hugging Face best practice: Create a non-root user (id 1000)
RUN useradd -m -u 1000 user
USER user
ENV PATH="/home/user/.local/bin:$PATH"

WORKDIR /home/user/app

# Copy files with proper ownership
COPY --chown=user . /home/user/app

# 🔴 DEBUGGING TRICK: Print all files and folders to the build log
RUN echo "=== DIRECTORY STRUCTURE ===" && tree /home/user/app || ls -laR /home/user/app

# Build C++ Engine
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20 && \
    make -j2

# Install Python backend requirements
RUN pip3 install --no-cache-dir fastapi uvicorn

EXPOSE 7860

# Start server safely
CMD ["python3", "-m", "uvicorn", "app:app", "--host", "0.0.0.0", "--port", "7860"]
