#!/bin/bash
# Script to rebuild the dev container after fixing memory issues

echo "=== Docker Container Rebuild Preparation ==="
echo ""

# Check Docker memory
DOCKER_MEMORY=$(docker info 2>/dev/null | grep "Total Memory" | awk '{print $3}')
echo "Current Docker Memory: ${DOCKER_MEMORY}"

if [ ! -z "$DOCKER_MEMORY" ]; then
    MEMORY_NUM=$(echo $DOCKER_MEMORY | sed 's/GiB//')
    if (( $(echo "$MEMORY_NUM < 6" | bc -l) )); then
        echo "⚠️  WARNING: Docker has less than 6GB memory!"
        echo "   Please increase Docker memory to at least 6-8GB before rebuilding."
        echo ""
        read -p "Continue anyway? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    else
        echo "✓ Docker memory is sufficient (${DOCKER_MEMORY})"
    fi
fi

echo ""
echo "Stopping any running containers..."
docker stop $(docker ps -aq) 2>/dev/null || true

echo "Removing old container images for this project..."
docker images | grep "vehicle-app-cpp-template" | awk '{print $3}' | xargs -r docker rmi -f

echo "Pruning Docker system..."
docker system prune -f

# Set BuildKit environment variables
export DOCKER_BUILDKIT=1
export BUILDKIT_STEP_LOG_MAX_SIZE=10000000

echo ""
echo "✓ Container rebuild preparation complete!"
echo ""
echo "Next steps:"
echo "1. In VS Code, press F1 (or Ctrl+Shift+P)"
echo "2. Type and select: 'Dev Containers: Rebuild Container'"
echo "3. Wait for the build to complete (may take 10-15 minutes)"
echo ""
echo "If build still fails:"
echo "- Increase Docker memory to 8GB in Docker Desktop settings"
echo "- Close other applications to free up system memory"
echo "- Try building with: 'Dev Containers: Rebuild Container Without Cache'"
