FROM python:3.11-slim

WORKDIR /app

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        libboost-atomic1.83.0 \
        libboost-chrono1.83.0 \
        libboost-date-time1.83.0 \
        libboost-filesystem1.83.0 \
        libboost-iostreams1.83.0 \
        libboost-program-options1.83.0 \
        libboost-regex1.83.0 \
        libboost-system1.83.0 \
        libboost-thread1.83.0 \
        libboost-wave1.83.0 \
        libgmp10 \
        libmpc3 \
        libmpfr6 \
        make \
        zlib1g \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir flask requests

COPY bridge.py .
COPY build_macos9.sh .
COPY scaffold_macos9_project.py .
RUN chmod +x /app/build_macos9.sh

EXPOSE 8080

CMD ["python", "bridge.py"]
