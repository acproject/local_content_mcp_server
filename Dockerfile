# Multi-stage build for Local Content MCP Server
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsqlite3-dev \
    pkg-config \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Create build directory and build the project
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_CLIENT=ON && \
    make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -r -s /bin/false -m -d /opt/mcp mcp

# Set working directory
WORKDIR /opt/mcp

# Copy built binaries from builder stage
COPY --from=builder /app/build/server/mcp_server ./bin/
COPY --from=builder /app/build/client/mcp_client ./bin/

# Copy configuration files
COPY --chown=mcp:mcp config/ ./config/

# Create necessary directories
RUN mkdir -p data logs && \
    chown -R mcp:mcp /opt/mcp

# Switch to non-root user
USER mcp

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Default command
CMD ["./bin/mcp_server", "--config", "config/server.json"]