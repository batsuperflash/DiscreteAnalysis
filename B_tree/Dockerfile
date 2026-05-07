FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    make \
    binutils \
    libdmalloc-dev \
    time \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work