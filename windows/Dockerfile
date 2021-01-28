FROM ubuntu:20.04 AS builder

ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake python3 gettext qt5-default \
    g++ build-essential \
    libreadline-dev \
    libkf5archive-dev libsqlite3-dev libsdl2-mixer-dev gettext git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root
RUN git clone https://github.com/longturn/freeciv21
WORKDIR /root/freeciv21

RUN cmake . -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DFREECIV_ENABLE_NLS=NO -DFREECIV_ENABLE_TOOLS=NO -DFREECIV_ENABLE_CLIENT=NO
RUN cmake --build build -j8
RUN cmake --build build --target install

FROM ubuntu:20.04
ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt-get update && apt-get install -y --no-install-recommends qt5-default libkf5archive5 libreadline5 && rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/ /usr/local/

RUN useradd -u 8877 wilddog
USER wilddog

ENV FREECIV_DATA_PATH=/usr/local/share/freeciv21/
ENTRYPOINT ["/usr/local/bin/freeciv-server"]
CMD ["-p" "6000"]



