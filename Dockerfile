FROM gcc AS builder
COPY . /umicat
WORKDIR /umicat
RUN make

FROM ubuntu:22.04
COPY --from=builder /umicat .
RUN install -Dm755 "umicat" "/usr/local/bin/umicat" \
    && mkdir -p "/etc/umicat" \
    && cat "conf/umicat.conf" > "/etc/umicat/umicat.conf" \
    && mkdir -p "/var/log/umicat"

EXPOSE 10201
ENTRYPOINT ["/usr/local/bin/umicat", "-c", "/etc/umicat/umicat.conf", "-l", "/var/log/umicat.log"]
