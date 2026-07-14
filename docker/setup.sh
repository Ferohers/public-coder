#!/bin/bash
set -e

echo "=== Mac OS 9 AI Code Studio - Bridge Setup ==="

if [ ! -f .env ]; then
    echo "Creating .env from template..."
    cp .env.example .env
    echo "Edit .env with your Groq/provider API key, endpoint, model, and bridge credentials"
    exit 1
fi

echo "Starting bridge container..."
docker compose pull
docker compose up -d

echo ""
echo "Bridge running on http://localhost:8080"
echo "Health check:"
curl -s -u $(grep BRIDGE_USER .env | cut -d= -f2):$(grep BRIDGE_PASS .env | cut -d= -f2) http://localhost:8080/health
echo ""
