# Build environment for open-bn (image open-bn-env): modern mipsel binutils, python
# and the pinned clang-format. The Sony toolchain is mounted at runtime (see below).
# Run it as linux/amd64 (emulated on Apple Silicon).
FROM debian:bookworm

# patch: make headers (tools/setup_cxx_headers.sh) runs in the container.
# python3-pyelftools: the link pipeline (make link / make link-cmp, tools/relink/) runs in the container.
RUN dpkg --add-architecture i386 && apt-get update && apt-get install -y --no-install-recommends \
    binutils-mipsel-linux-gnu make python3 diffutils file patch \
    qemu-user-static rpm2cpio cpio libc6:i386 \
    python3-pyelftools \
    && rm -rf /var/lib/apt/lists/*

# /usr/cross/mipsEEel-linux/include: tools/check.py and tools/match.sh create here, at every start, the
# symlink g++-3 -> docker/toolchain/include/libstdc++ (the path of the Sony installation, for the `__FILE__`
# of the asserts in <std/bastring.cc>). World-writable (1777) because the targets run with --user
# (Makefile, DOCKER_USER): the user creates the link in every container (--rm), so it follows the mount path
# of the repo.
RUN mkdir -p /usr/cross/mipsEEel-linux/include && chmod 1777 /usr/cross/mipsEEel-linux/include

# Source formatter, pinned: tools/format.py accepts only this version (make format, make check); keep in sync with it.
RUN apt-get update && apt-get install -y --no-install-recommends python3-pip \
    && pip3 install --no-cache-dir --break-system-packages clang-format==23.1.5 \
    && rm -rf /var/lib/apt/lists/*

# The Sony toolchain (rebuilt gcc 2.95.2-3a and binutils 2.9EE) is not baked into
# the image: it is mounted at runtime from docker/toolchain/.
WORKDIR /work
