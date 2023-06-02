FROM gcc AS buildStage
COPY . /umicat
RUN cd /umicat && make

FROM ubuntu:22.04
COPY --from=buildStage /umicat .
RUN apt update && apt install make
RUN make install-docker

CMD umicat -c /etc/umicat/umicat.conf -l /var/log/umicat.log
