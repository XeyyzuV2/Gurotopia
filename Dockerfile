FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential libssl-dev libmariadb-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . /build
WORKDIR /build

RUN make -j$(nproc)

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        libssl3t64 libmariadb3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /gurotopia
COPY --from=build /build/main.out .
COPY --from=build /build/resources ./resources

EXPOSE 17091/udp
EXPOSE 443/tcp

CMD ["./main.out"]