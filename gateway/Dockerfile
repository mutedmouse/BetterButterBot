# ButterBot voice gateway - CPU image (linux/amd64 and linux/arm64).
#   To move it elsewhere: docker build -t ghcr.io/<your-account>/bb-voice-gateway:cpu -f Dockerfile . && docker push ...
#   From here: docker compose --profile cpu up -d --build
FROM python:3.12-slim

ARG VERSION=dev
LABEL org.opencontainers.image.title="bb-voice-gateway" \
      org.opencontainers.image.description="BetterButterBot voice gateway (CPU): Whisper speech-to-text, command matching, LLM answers, calendar" \
      org.opencontainers.image.version="$VERSION" \
      org.opencontainers.image.licenses="MIT"

ENV PYTHONUNBUFFERED=1 PIP_NO_CACHE_DIR=1 \
    HF_HOME=/data/hf BB_DATA_DIR=/data
WORKDIR /app

COPY requirements.txt .
RUN pip install -r requirements.txt

COPY *.py Scenarios.h config.example.yaml VERSION ./

EXPOSE 8765 8766
VOLUME ["/data"]
# Healthy once the WebSocket port accepts connections (the first start downloads the Whisper model).
HEALTHCHECK --interval=30s --timeout=5s --start-period=10m --retries=3 \
    CMD python -c "import socket; socket.create_connection(('127.0.0.1', 8765), 3)" || exit 1
CMD ["python", "gateway.py", "--config", "/app/config.yaml"]
