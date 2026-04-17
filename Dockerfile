FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    ca-certificates \
    g++ \
    make \
    cmake \
    libomp-dev \
    libgtest-dev \
    git \
  && rm -rf /var/lib/apt/lists/*

# Google Test en Ubuntu instala el código fuente. 
# Estos comandos lo compilan e instalan en el sistema para que g++ lo encuentre.
RUN cd /usr/src/googletest \
    && mkdir build && cd build \
    && cmake .. && make && make install

WORKDIR /workspace

CMD ["bash"]