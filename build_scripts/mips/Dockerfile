# FreeBASIC MIPS Linux toolchain container
# ----------------------------------------
#
# File: build_scripts/mips/Dockerfile
#
# Purpose:
#
#     Provide a reproducible Ubuntu environment for building and testing the
#     MIPS32 and MIPS64 Linux ports when the host distribution does not ship
#     the corresponding cross toolchains.
#
# Responsibilities:
#
#     - install big-endian and little-endian GNU MIPS cross toolchains
#     - provide target libffi archives for FreeBASIC's THREADCALL runtime
#     - provide target ncurses archives for the console runtime
#     - install QEMU user-mode emulators for target executable validation
#     - provide the native build tools used by the FreeBASIC makefiles
#
# This file intentionally does NOT contain:
#
#     - FreeBASIC source code
#     - target build policy
#     - test orchestration

FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

# Keep Ubuntu's default mirrors when they resolve; use an independent full
# archive mirror when the runner's Ubuntu mirror DNS is unavailable.
RUN set -eu; \
    mirror_dns_failed=0; \
    for mirror_host in azure.archive.ubuntu.com archive.ubuntu.com security.ubuntu.com; do \
        if ! getent ahosts "$mirror_host" >/dev/null 2>&1; then \
            mirror_dns_failed=1; \
            break; \
        fi; \
    done; \
    if [ "$mirror_dns_failed" -ne 0 ]; then \
        if getent ahosts mirrors.edge.kernel.org >/dev/null 2>&1; then \
            if [ -f /etc/apt/apt-mirrors.txt ]; then \
                printf '%s\n' 'https://mirrors.edge.kernel.org/ubuntu/' > /etc/apt/apt-mirrors.txt; \
            fi; \
            for source_file in /etc/apt/sources.list /etc/apt/sources.list.d/*.sources /etc/apt/sources.list.d/*.list; do \
                [ -f "$source_file" ] || continue; \
                sed -i -E 's#https?://(azure\.archive|archive|security)\.ubuntu\.com/ubuntu/?#https://mirrors.edge.kernel.org/ubuntu/#g' "$source_file"; \
            done; \
            echo "Ubuntu archive DNS failed; using the independent fallback mirror" >&2; \
        else \
            echo "WARNING: Ubuntu archive DNS and the fallback mirror are unavailable" >&2; \
        fi; \
    fi; \
    for attempt in 1 2 3; do \
        if apt-get -o Acquire::Retries=2 -o APT::Update::Error-Mode=any update \
            && apt-get -o Acquire::Retries=2 install -y --no-install-recommends \
        binutils-mips-linux-gnu \
        binutils-mips64-linux-gnuabi64 \
        binutils-mips64el-linux-gnuabi64 \
        binutils-mipsel-linux-gnu \
        build-essential \
        ca-certificates \
        curl \
        file \
        g++-mips-linux-gnu \
        g++-mips64-linux-gnuabi64 \
        g++-mips64el-linux-gnuabi64 \
        g++-mipsel-linux-gnu \
        gcc-mips-linux-gnu \
        gcc-mips64-linux-gnuabi64 \
        gcc-mips64el-linux-gnuabi64 \
        gcc-mipsel-linux-gnu \
        libc6-dev-mips-cross \
        libc6-dev-mips64-cross \
        libc6-dev-mips64el-cross \
        libc6-dev-mipsel-cross \
        libffi-dev \
        libncurses-dev \
        make \
        pkg-config \
        python3 \
        qemu-user \
        rsync \
        unzip \
        zip; then \
            rm -rf /var/lib/apt/lists/*; \
            exit 0; \
        fi; \
        if [ "$attempt" -lt 3 ]; then \
            sleep "$((attempt * 5))"; \
        fi; \
    done; \
    echo "ERROR: APT setup failed after 3 attempts" >&2; \
    exit 1

ARG LIBFFI_VERSION=3.4.6
ARG LIBFFI_SHA256=b0dea9df23c863a7a50e825440f3ebffabd65df1497108e5d437747843895a4e
ARG NCURSES_VERSION=6.4
ARG NCURSES_SHA256=6931283d9ac87c5073f30b6290c4c75f21632bb4fc3603ac8100812bed248159

# Ubuntu publishes the MIPS libc sysroots but not matching libffi development
# packages. Build the small dependency once per ABI so THREADCALL remains a
# supported runtime feature instead of becoming a target-specific omission.
RUN curl -fsSL --retry 10 --retry-max-time 300 \
        "https://github.com/libffi/libffi/releases/download/v${LIBFFI_VERSION}/libffi-${LIBFFI_VERSION}.tar.gz" \
        -o /tmp/libffi.tar.gz \
    && printf '%s  %s\n' "${LIBFFI_SHA256}" /tmp/libffi.tar.gz \
        | sha256sum -c - \
    && tar -xzf /tmp/libffi.tar.gz -C /tmp \
    && for target in \
        mips-linux-gnu \
        mipsel-linux-gnu \
        mips64-linux-gnuabi64 \
        mips64el-linux-gnuabi64; do \
        build_dir="/tmp/libffi-build-${target}"; \
        mkdir -p "${build_dir}"; \
        cd "${build_dir}"; \
        "/tmp/libffi-${LIBFFI_VERSION}/configure" \
            --host="${target}" \
            --prefix="/usr/${target}" \
            --includedir="/usr/${target}/include" \
            --libdir="/usr/${target}/lib" \
            --disable-docs \
            --disable-shared \
            --enable-static; \
        make -j2; \
        make install; \
        rm -rf "${build_dir}"; \
    done \
    && rm -rf "/tmp/libffi-${LIBFFI_VERSION}" /tmp/libffi.tar.gz

# The cross compilers otherwise find the host curses headers while lacking a
# target libtinfo. Install target headers and static libraries together so the
# console ABI and final link are both self-consistent.
COPY fetch-source.sh /usr/local/bin/fbc-fetch-source

RUN sh /usr/local/bin/fbc-fetch-source /tmp/ncurses.tar.gz "${NCURSES_SHA256}" \
        "https://ftp.gnu.org/gnu/ncurses/ncurses-${NCURSES_VERSION}.tar.gz" \
        "https://mirrors.kernel.org/gnu/ncurses/ncurses-${NCURSES_VERSION}.tar.gz" \
    && tar -xzf /tmp/ncurses.tar.gz -C /tmp \
    && for target in \
        mips-linux-gnu \
        mipsel-linux-gnu \
        mips64-linux-gnuabi64 \
        mips64el-linux-gnuabi64; do \
        build_dir="/tmp/ncurses-build-${target}"; \
        mkdir -p "${build_dir}"; \
        cd "${build_dir}"; \
        CFLAGS="-O2 -fPIC" \
        "/tmp/ncurses-${NCURSES_VERSION}/configure" \
            --host="${target}" \
            --prefix="/usr/${target}" \
            --includedir="/usr/${target}/include" \
            --libdir="/usr/${target}/lib" \
            --disable-db-install \
            --without-ada \
            --without-cxx \
            --without-cxx-binding \
            --without-debug \
            --without-form \
            --without-manpages \
            --without-menu \
            --without-panel \
            --without-progs \
            --without-shared \
            --without-tests \
            --with-normal \
            --with-pic \
            --with-termlib; \
        make -j2 libs; \
        make install.includes install.libs; \
        rm -rf "${build_dir}"; \
    done \
    && rm -rf "/tmp/ncurses-${NCURSES_VERSION}" /tmp/ncurses.tar.gz

WORKDIR /workspace

# end of build_scripts/mips/Dockerfile
