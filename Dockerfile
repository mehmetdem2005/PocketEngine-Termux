FROM node:20-slim

WORKDIR /app

# Install dependencies for better-sqlite3 build
RUN apt-get update && apt-get install -y \
    python3 make g++ sqlite3 \
    && rm -rf /var/lib/apt/lists/*

# Copy package files
COPY server/package*.json ./

RUN npm install --production

# Copy source
COPY server/src ./src

# Create data dir
RUN mkdir -p /data/uploads

ENV NODE_ENV=production
ENV PORT=3000
ENV DB_PATH=/data/pocket.db

EXPOSE 3000

HEALTHCHECK --interval=30s --timeout=10s --start-period=15s --retries=3 \
    CMD wget -qO- http://localhost:3000/ || exit 1

CMD ["node", "src/index.js"]
