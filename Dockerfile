FROM ubuntu:20.04

RUN apt update

RUN apt install -y wget
RUN apt install -y vim
RUN apt install -y procps

RUN apt install -y gcc
RUN apt install -y make


RUN mkdir /umicat
ADD ./umicat /umicat

