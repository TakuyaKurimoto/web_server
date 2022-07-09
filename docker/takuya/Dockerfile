FROM ubuntu:20.04

WORKDIR /usr/src/app
RUN apt update -y && \
    apt-get -y install tzdata && \
    apt-get -y install gcc && \
    apt-get -y install make && \
    apt-get -y install libc-dev && \
    apt-get -y install binutils && \
    apt-get -y install iputils-ping net-tools && \
    apt-get -y install libjansson-dev && \
    apt-get -y install git && \
    apt-get -y install curl zip unzip tar && \
    apt-get -y install autoconf && \
    apt-get -y install libtool && \
    apt-get -y install gdb

RUN git clone https://github.com/akheron/jansson.git && \
    cd jansson && \
    autoreconf -i && \
    ./configure && \
    make && \
    make install
    