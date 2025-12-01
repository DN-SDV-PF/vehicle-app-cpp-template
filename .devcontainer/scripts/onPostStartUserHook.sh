#!/bin/bash
# User-defined post-start hook
# This script runs after the devcontainer starts
# This file is NOT managed by velocitas - it will persist across rebuilds

WORKSPACE_DIR="/workspaces/vehicle-app-cpp-template"
cd "$WORKSPACE_DIR"

echo "######################################################"
echo "### Installing Conan SDK fix hook                 ###"
echo "######################################################"
mkdir -p ~/.conan2/extensions/hooks
if [ -f ".devcontainer/scripts/conan_hooks/fix_sdk_hook.py" ]; then
    cp .devcontainer/scripts/conan_hooks/fix_sdk_hook.py ~/.conan2/extensions/hooks/
    echo "✓ Conan hook installed to ~/.conan2/extensions/hooks/"
fi

echo "######################################################"
echo "### Fixing SDK conanfile.py                       ###"
echo "######################################################"
# Run the SDK fix script (includes Conan cache cleanup and re-export)
if [ -f ".devcontainer/scripts/fix-sdk-conanfile.sh" ]; then
    .devcontainer/scripts/fix-sdk-conanfile.sh
fi

echo "######################################################"
echo "### Installing Dependencies                       ###"
echo "######################################################"
cd "$WORKSPACE_DIR"
if [ -f "./install_dependencies.sh" ]; then
    ./install_dependencies.sh
    echo "✓ Dependencies installed"
fi

echo "######################################################"
echo "### Installing Quad                               ###"
echo "######################################################"
if [ -d "$WORKSPACE_DIR/quad-local/quad" ]; then
    cd "$WORKSPACE_DIR/quad-local/quad"
    if [ -f "./install.sh" ]; then
        ./install.sh
        echo "✓ Quad installed"
    fi
fi

echo "######################################################"
echo "### Starting Quad                                 ###"
echo "######################################################"
if [ -d "$WORKSPACE_DIR/quad-local/quad" ]; then
    cd "$WORKSPACE_DIR/quad-local/quad"
    if [ -f "./run.sh" ]; then
        # Run quad in background
        nohup ./run.sh > /tmp/quad.log 2>&1 &
        echo "✓ Quad started in background (log: /tmp/quad.log)"
        
        # Wait for quad to be ready
        echo "Waiting for Quad services to be ready..."
        for i in {1..30}; do
            if curl -s http://localhost:50050 > /dev/null 2>&1; then
                echo "✓ Quad is ready"
                break
            fi
            echo "  Waiting... ($i/30)"
            sleep 2
        done
    fi
fi

echo "######################################################"
echo "### Downloading vehicle model from GitLab         ###"
echo "######################################################"
cd "$WORKSPACE_DIR"
# Run the custom vspec download script with GitLab token
if [ -f ".devcontainer/scripts/download-vspec-with-token.sh" ]; then
    .devcontainer/scripts/download-vspec-with-token.sh
fi

echo ""
echo "######################################################"
echo "### Post-start setup complete!                    ###"
echo "######################################################"
echo ""
echo "Next steps:"
echo "  1. Build the app: ./build.sh"
echo "  2. Run tests: ./app/tests/integration_test.sh"
echo ""
