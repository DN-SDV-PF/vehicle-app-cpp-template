#!/bin/bash
# User-defined post-start hook
# This script runs after the devcontainer starts
# This file is NOT managed by velocitas - it will persist across rebuilds

echo "######################################################"
echo "### Installing Conan SDK fix hook                 ###"
echo "######################################################"
mkdir -p ~/.conan2/extensions/hooks
if [ -f ".devcontainer/scripts/conan_hooks/fix_sdk_hook.py" ]; then
    cp .devcontainer/scripts/conan_hooks/fix_sdk_hook.py ~/.conan2/extensions/hooks/
    echo "✓ Conan hook installed to ~/.conan2/extensions/hooks/"
fi

echo "######################################################"
echo "### Fixing SDK conanfile.py (FORCED)              ###"
echo "######################################################"
# Always remove SDK from Conan cache first to ensure clean state
echo "Removing vehicle-app-sdk from Conan cache..."
conan remove "vehicle-app-sdk*" -c 2>/dev/null || true

# Run the SDK fix script
if [ -f ".devcontainer/scripts/fix-sdk-conanfile.sh" ]; then
    .devcontainer/scripts/fix-sdk-conanfile.sh
fi

# Ensure SDK is re-exported after fix
SDK_PATH=""
for project_dir in ~/.velocitas/projects/*/; do
    if [ -d "${project_dir}vehicle-app-sdk-cpp" ]; then
        SDK_PATH="${project_dir}vehicle-app-sdk-cpp"
        break
    fi
done
if [ -n "$SDK_PATH" ] && [ -d "$SDK_PATH" ]; then
    echo "Re-exporting SDK to ensure it's in cache..."
    cd "$SDK_PATH" && conan export . --name vehicle-app-sdk --version 1.0.3 2>/dev/null || true
    echo "✓ SDK exported to Conan cache"
fi

echo "######################################################"
echo "### Downloading vehicle model from GitLab ###"
echo "######################################################"

# Run the custom vspec download script with GitLab token BEFORE velocitas initialization
if [ -f ".devcontainer/scripts/download-vspec-with-token.sh" ]; then
    .devcontainer/scripts/download-vspec-with-token.sh
fi
