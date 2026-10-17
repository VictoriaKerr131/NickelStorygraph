# Kobo Qt6 cross toolchain + Rust/just/resvg, for NickelStorygraph.
#
# This is the Qt 6 counterpart of NickelTC: it cross-compiles against Kobo
# firmware 5.18+ (Qt 6.5.2). The first three stages (qt6-base, qt6-build,
# qt6-toolchain) are the canonical Qt6 toolchain — see qt6/Dockerfile for the
# annotated original. The final stage adds just, the Rust
# arm-unknown-linux-gnueabihf toolchain and resvg on top.
#
# Kobo built firmware 5.18 with GCC 11.5.0 and glibc 2.35, a Yocto kirkstone
# set, and its Qt 6.5.2 links system ICU 70, glib 2.72, fontconfig, FreeType,
# HarfBuzz, xkbcommon, D-Bus and OpenGL ES 2. Ubuntu 22.04 ships the same
# glibc, GCC 11.4 and the same library generation for armhf, so the sysroot is
# Ubuntu's armhf multiarch packages. GCC 11.4 and 11.5 share the C++ ABI
# (GLIBCXX_3.4.29). Qt is built from Kobo's published 6.5.2 qtbase source.

FROM docker.io/library/ubuntu:22.04@sha256:2edbbc5dc405e9612ba3584ce95480277e3eb374407b5505fe26f17df77c7dbc AS qt6-base
ARG DEBIAN_FRONTEND=noninteractive

# The armhf packages come from ports.ubuntu.com on every host architecture.
# The stock sources.list must be restricted to the native architecture first,
# or apt looks for armhf packages on archive.ubuntu.com and fails.
RUN set -eu; \
    arch="$(dpkg --print-architecture)"; \
    sed -i "s|^deb |deb [arch=$arch] |" /etc/apt/sources.list; \
    dpkg --add-architecture armhf; \
    for pocket in jammy jammy-updates jammy-security; do \
        echo "deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports $pocket main universe"; \
    done > /etc/apt/sources.list.d/armhf.list; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        bash bzip2 ca-certificates cmake coreutils curl file g++ gcc git jq \
        libdbus-1-dev make ninja-build patch pkg-config python3 unzip xz-utils zip \
        gcc-11-arm-linux-gnueabihf g++-11-arm-linux-gnueabihf binutils-arm-linux-gnueabihf \
        libc6-dev:armhf libstdc++-11-dev:armhf \
        libatspi2.0-dev:armhf libdbus-1-dev:armhf libdrm-dev:armhf libegl-dev:armhf libevdev-dev:armhf \
        libfontconfig-dev:armhf libfreetype-dev:armhf libgbm-dev:armhf libgles-dev:armhf \
        libglib2.0-dev:armhf libharfbuzz-dev:armhf libicu-dev:armhf libinput-dev:armhf \
        libmtdev-dev:armhf libpcre2-dev:armhf libpng-dev:armhf libsqlite3-dev:armhf \
        libssl-dev:armhf libudev-dev:armhf libxkbcommon-dev:armhf libzstd-dev:armhf \
        zlib1g-dev:armhf; \
    rm -rf /var/lib/apt/lists/*

FROM qt6-base AS qt6-build

COPY qt6/source /tc/source
RUN python3 /tc/source/prepare.py /tc/src/qtbase

COPY qt6/wrappers.sh qt6/kobo.cmake qt6/build-qt.sh /tc/src/
COPY qt6/mkspecs /tc/src/mkspecs
RUN /tc/src/wrappers.sh /tc/arm-kobo-linux-gnueabihf

# The Qt build takes tens of minutes. It runs at its final path because the
# generated toolchain files and qmake wrappers record absolute paths.
RUN /tc/src/build-qt.sh /tc/arm-kobo-linux-gnueabihf /tc/src

FROM qt6-base AS qt6-toolchain
COPY --from=qt6-build /tc/arm-kobo-linux-gnueabihf /tc/arm-kobo-linux-gnueabihf
COPY qt6/check.sh /tc/arm-kobo-linux-gnueabihf/check.sh
# CMake 3.21 and later read CMAKE_TOOLCHAIN_FILE from the environment, so a
# plain cmake call cross-compiles here.
ENV PATH=/tc/arm-kobo-linux-gnueabihf/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
    CROSS_TC=arm-kobo-linux-gnueabihf \
    CMAKE_TOOLCHAIN_FILE=/tc/arm-kobo-linux-gnueabihf/toolchain.cmake \
    PKG_CONFIG=arm-kobo-linux-gnueabihf-pkg-config \
    LC_ALL=C.UTF-8
RUN /tc/arm-kobo-linux-gnueabihf/check.sh && rm -rf /tmp/*

FROM qt6-toolchain AS final

ARG UID=0
ARG GID=0
RUN if [ $UID -ne 0 ]; then \
      groupadd --gid $GID user && \
      adduser --uid $UID --gid $GID user && \
      mkdir /rustup /cargo && \
      chown user:user /rustup /cargo; \
    fi

RUN curl --proto '=https' --tlsv1.2 -sSfL https://github.com/casey/just/releases/download/1.55.1/just-1.55.1-x86_64-unknown-linux-musl.tar.gz | tar zxf - -C /usr/bin just

USER $UID

ENV RUSTUP_HOME=/rustup CARGO_HOME=/cargo
RUN mkdir -p /rustup /cargo && \
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --target arm-unknown-linux-gnueabihf
ENV PATH=$CARGO_HOME/bin:$PATH

RUN cargo install resvg

# The Forgejo action (using: docker) passes its `args` to this entrypoint.
ENTRYPOINT ["/bin/bash", "-c"]
