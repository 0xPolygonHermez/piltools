FROM ubuntu:22.04 AS build

RUN DEBIAN_FRONTEND=noninteractive apt-get -y update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential \
    libomp-dev nlohmann-json3-dev libgmp-dev libreadline-dev git

WORKDIR /src
RUN git clone https://github.com/ithewei/libhv.git && cd libhv && \
    ./configure && make && make install

COPY .git /src/.git
COPY .gitmodules /src/
COPY Makefile /src/
COPY src /src/src
WORKDIR /src
RUN make clean && mkdir -p build && git submodule init \
    && git submodule update && make

FROM ubuntu:22.04
RUN DEBIAN_FRONTEND=noninteractive apt-get -y update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y libomp-dev libreadline-dev
COPY --from=build /usr/local/lib/libhv.so /usr/local/lib/libhv.so
RUN ldconfig
COPY --from=build /src/build/pilserver /usr/local/bin/pilserver
COPY --from=build /src/build/pilverify /usr/local/bin/pilverify
WORKDIR /app
COPY html /app/html
