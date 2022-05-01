FROM ubuntu:20.04

RUN apt update

RUN apt install -y wget
RUN apt install -y vim
RUN apt install -y procps

RUN apt install -y gcc
RUN apt install -y make

COPY . /umicat

RUN cd /umicat && make && make install-docker
RUN rm -rf /umicat

RUN apt uninstall wget vim procps make gcc

CMD umicat -c /etc/umicat/umicat.conf -l /var/log/umicat.log