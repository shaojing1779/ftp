ARG CGO=0
FROM ccr.ccs.tencentyun.com/golden-cloud/golang-git-base:1.14-buster AS builder

WORKDIR /build/

COPY . /build/reimburse-srv/
FROM ccr.ccs.tencentyun.com/golden-cloud/debian-base:buster

ARG env
ARG xconf

ENV MICRO_SRV_ENV=$env
ENV MICRO_CONFIG_SRV_ADDR=$xconf

WORKDIR /data

VOLUME ["/data/log"]

EXPOSE 10096
ENTRYPOINT ["./ftp"]

