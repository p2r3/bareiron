# ---- Build stage ----
FROM openjdk:21-jdk-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential gcc git curl nodejs npm unzip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
RUN git clone https://github.com/p2r3/bareiron.git .

# Copy in Minecraft server.jar (must match version!)
# COPY server.jar notchian/server.jar
COPY notchian/ notchian/

# Run registry extraction
RUN chmod +x extract_registries.sh && ./extract_registries.sh

# Build
RUN gcc src/*.c -O3 -Iinclude -o bareiron

# ---- Runtime stage ----
FROM debian:bookworm-slim
WORKDIR /app
COPY --from=builder /src/bareiron ./bareiron
VOLUME ["/app/world"]
EXPOSE 25565
CMD ["./bareiron"]
