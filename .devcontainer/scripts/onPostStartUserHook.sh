#!/bin/bash
# User-defined post-start hook
# This script runs after the devcontainer starts

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
if [ -f ".devcontainer/scripts/fix-sdk-conanfile.sh" ]; then
    .devcontainer/scripts/fix-sdk-conanfile.sh
fi

echo "######################################################"
echo "### Downloading vehicle model from GitLab ###"
echo "######################################################"

# Run the custom vspec download script with GitLab token BEFORE velocitas initialization
if [ -f ".devcontainer/scripts/download-vspec-with-token.sh" ]; then
    .devcontainer/scripts/download-vspec-with-token.sh
fi
