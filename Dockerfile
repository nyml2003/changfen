FROM ubuntu:22.04
LABEL authors="肠粉"

RUN mv /etc/apt/sources.list /etc/apt/sources.list.bak
COPY sources.list /etc/apt/
COPY start.sh /home/
RUN sed -i 's/\r$//' /home/start.sh

RUN apt-get update && apt-get install -y \
    gcc \
    cmake \
    g++ \
    git \
    gdb \
    openssh-server \
    vim \
    flex \
    bison \
    clang \
    llvm 


# 创建用户admin,admin
RUN useradd -m admin && echo "admin:admin" | chpasswd && adduser admin sudo
RUN echo "admin ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# ssh
RUN mkdir /var/run/sshd
RUN sed -ri 's/^#?PermitRootLogin\s+.*/PermitRootLogin yes/' /etc/ssh/sshd_config
RUN sed -ri 's/UsePAM yes/UsePAM no/g' /etc/ssh/sshd_config

RUN apt-get install -y \
    gdb-multiarch \
    binutils-riscv64-linux-gnu \
    gcc-riscv64-linux-gnu \
    qemu-user \
    qemu-system-misc\
    gcc-riscv64-unknown-elf\
    binutils-riscv64-unknown-elf\
    libc6-riscv64-cross

RUN  cp /usr/riscv64-linux-gnu/lib/* /lib/
RUN  apt-get -y install libgtest-dev 
RUN  cd /usr/src/gtest && \
mkdir build && \
cd build && \
cmake .. && \
make && \
cp lib/libgtest*.a /usr/local/lib





