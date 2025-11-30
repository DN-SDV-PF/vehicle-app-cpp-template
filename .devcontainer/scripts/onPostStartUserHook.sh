#!/bin/bash
# User-defined post-start hook
# This script runs after the devcontainer starts

echo "######################################################"
echo "### Downloading vehicle model from GitLab ###"
echo "######################################################"

# Run the custom vspec download script with GitLab token BEFORE velocitas initialization
if [ -f ".devcontainer/scripts/download-vspec-with-token.sh" ]; then
    .devcontainer/scripts/download-vspec-with-token.sh
fi
