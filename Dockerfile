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
    llvm \
    clang \
    valgrind \
    openssh-server \
    zlib1g-dev \
    net-tools \
    sudo \
    vim \
    flex \
    bison

# 创建用户admin,admin
RUN useradd -m admin && echo "admin:admin" | chpasswd && adduser admin sudo
RUN echo "admin ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# ssh
RUN mkdir /var/run/sshd
RUN sed -ri 's/^#?PermitRootLogin\s+.*/PermitRootLogin yes/' /etc/ssh/sshd_config
RUN sed -ri 's/UsePAM yes/UsePAM no/g' /etc/ssh/sshd_config





