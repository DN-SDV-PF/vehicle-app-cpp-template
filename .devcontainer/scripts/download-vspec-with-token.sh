#!/bin/bash
# Script to download vspec from GitLab with API token

set -e

# Determine script directory and workspace
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Load GitLab token from .env if it exists
if [ -f "$WORKSPACE_DIR/.env" ]; then
    source "$WORKSPACE_DIR/.env"
fi

# Read the AppManifest to get the source URL
APP_MANIFEST="$WORKSPACE_DIR/app/AppManifest.json"

if [ ! -f "$APP_MANIFEST" ]; then
    echo "[ERROR] AppManifest.json not found!"
    exit 1
fi

# Extract the src URL from AppManifest.json using Python
SRC_URL=$(python3 -c "
import json
with open('$APP_MANIFEST', 'r') as f:
    manifest = json.load(f)
    for interface in manifest.get('interfaces', []):
        if interface.get('type') == 'vehicle-signal-interface':
            print(interface['config']['src'])
            break
")

if [ -z "$SRC_URL" ]; then
    echo "[ERROR] No vehicle-signal-interface src found in AppManifest.json"
    exit 1
fi

echo "[INFO] Downloading vehicle model from: $SRC_URL"

# Check if SRC_URL is a local file path
if [[ "$SRC_URL" == /* ]] || [[ "$SRC_URL" == file://* ]]; then
    LOCAL_PATH="${SRC_URL#file://}"
    if [ -f "$LOCAL_PATH" ]; then
        echo "[INFO] Source is a local file, no download needed"
        echo "[INFO] Vehicle model available at: $LOCAL_PATH"
        exit 0
    else
        echo "[ERROR] Local file not found: $LOCAL_PATH"
        exit 1
    fi
fi

# Determine the cache directory (same as velocitas uses)
CACHE_DIR="$HOME/.velocitas/projects/$(echo -n "$PWD" | md5sum | cut -d' ' -f1)"
mkdir -p "$CACHE_DIR"
OUTPUT_FILE="$CACHE_DIR/vspec.json"

# Download with GitLab authentication if needed
if [[ "$SRC_URL" == *"gitlab.geniie.net"* ]] && [ -n "$GITLAB_API_TOKEN" ]; then
    echo "[INFO] Using GitLab API token for authentication..."
    
    # Convert the raw URL to API URL format using Python
    API_URL=$(python3 -c "
import urllib.parse
import re

url = '$SRC_URL'
match = re.search(r'gitlab\.geniie\.net/(.+?)/-/raw/([^/]+)/(.+)', url)

if match:
    project_path = match.group(1)
    ref = match.group(2)
    file_path = match.group(3)
    
    # URL encode
    project_encoded = urllib.parse.quote(project_path, safe='')
    file_encoded = urllib.parse.quote(file_path, safe='')
    
    api_url = f'https://gitlab.geniie.net/api/v4/projects/{project_encoded}/repository/files/{file_encoded}/raw?ref={ref}'
    print(api_url)
else:
    print(url)
")
    
    echo "[INFO] Downloading from GitLab API..."
    curl -f -H "PRIVATE-TOKEN: $GITLAB_API_TOKEN" "$API_URL" -o "$OUTPUT_FILE"
    
    if [ $? -eq 0 ]; then
        echo "[INFO] Successfully downloaded to $OUTPUT_FILE"
    else
        echo "[ERROR] Failed to download from GitLab"
        exit 1
    fi
else
    echo "[INFO] Downloading without authentication..."
    curl -f "$SRC_URL" -o "$OUTPUT_FILE"
fi

# Also copy to local workspace location for AppManifest.json file:// reference
mkdir -p /workspaces/vehicle-app-cpp-template/downloaded-datamodel
cp "$OUTPUT_FILE" /workspaces/vehicle-app-cpp-template/downloaded-datamodel/dataModel_basic.json
echo "[INFO] Also copied to /workspaces/vehicle-app-cpp-template/downloaded-datamodel/dataModel_basic.json"

echo "[INFO] Vehicle model downloaded successfully!"
