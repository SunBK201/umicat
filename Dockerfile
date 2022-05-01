FROM ubuntu:20.04

RUN apt update

RUN apt install -y vim wget procps gcc make

COPY . /umicat

RUN cd /umicat && make && make install-docker
RUN rm -rf /umicat

RUN apt purge -y make gcc

CMD umicat -c /etc/umicat/umicat.conf -l /var/log/umicat.log