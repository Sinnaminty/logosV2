# Use an official lightweight C++ image as a base
FROM ubuntu:latest

# Set up environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install required dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    git \
    build-essential \
    libssl-dev \
    libwebsocketpp-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-chrono-dev \
    libboost-regex-dev \
    libssl-dev \
    libopus-dev \
    libogg-dev \
    libmpg123-dev \
    libfmt-dev \
    ffmpeg \
    curl \
    wget \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && wget -O dpp.deb https://dl.dpp.dev/ \
    && dpkg -i dpp.deb 
    

# Copy your bot source code into the container
WORKDIR /app
COPY . .


# Build the bot.
RUN cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    && cmake --build build


CMD ["./build/Logos"]
