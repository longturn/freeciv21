FROM ghcr.io/linuxcontainers/debian-slim:latest
RUN apt-get update -y \
    && apt-get upgrade -y \
    && apt-get install -y wget \
    && wget https://github.com/longturn/freeciv21/releases/download/v3.0-patch.1/freeciv21_3.0-patch.1_amd64.deb \
    && wget https://github.com/longturn/freeciv21/releases/download/v3.0-patch.1/freeciv21_3.0-patch.1_amd64.deb.sha256 \
    && sha256sum -c freeciv21_3.0-patch.1_amd64.deb.sha256 \
    && apt-get install -y ./freeciv21_3.0-patch.1_amd64.deb \
    && useradd freeciv21
EXPOSE 5556
ENTRYPOINT su -c freeciv21-server freeciv21
