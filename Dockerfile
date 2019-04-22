FROM ubuntu:18.10
MAINTAINER Falcon22
USER root

RUN apt-get -y update && apt-get install -y wget
RUN apt-get -y upgrade

RUN apt-get install -y cmake g++ make libtool m4 autotools-dev autoconf automake
RUN apt-get -y install libuv1.dev

RUN apt-get -y update
RUN apt-get -y upgrade

ENV PROJECT_NAME server

WORKDIR /opt/$PROJECT_NAME
ADD . /opt/$PROJECT_NAME/

RUN mkdir build &&\
    cd build &&\
    cmake -DCMAKE_BUILD_TYPE=Release .. &&\
    make

EXPOSE 8080
CMD ["server"]