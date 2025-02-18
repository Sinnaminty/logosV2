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
  libasound2-dev \
  libpulse-dev \
  libgtk2.0-dev \
  libssl-dev \
  libopus-dev \
  libogg-dev \
  libmpg123-dev \
  libfmt-dev \
  libcurl4-openssl-dev \
  unzip \
  ffmpeg \
  curl \
  wget \
  yt-dlp \
  autoconf \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/* \
  && wget --progress=dot:giga -O dpp.deb https://dl.dpp.dev/ \
  && dpkg -i dpp.deb \
  && git clone https://github.com/dectalk/dectalk.git \
  && cd dectalk/src \
  && autoreconf -si \
  && ./configure \
  && make -j \
  && make install 




# Copy your bot source code into the container
WORKDIR /app
COPY . .

# Build the bot.
RUN cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TZ_LIB=ON \
  && cmake --build build


CMD ["./build/Logos"]

