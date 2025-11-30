#!/bin/bash
# User-defined on-create hook
# This script runs during container creation, before velocitas init

echo "######################################################"
echo "### Pre-downloading vehicle model from GitLab ###"
echo "######################################################"

# Read the GitLab source URL from AppManifest.json
GITLAB_SRC=$(python3 -c "
import json
with open('/workspaces/vehicle-app-cpp-template/app/AppManifest.json', 'r') as f:
    manifest = json.load(f)
    for interface in manifest.get('interfaces', []):
        if interface.get('type') == 'vehicle-signal-interface':
            # Look for _gitlab_src field
            gitlab_src = interface['config'].get('_gitlab_src')
            if gitlab_src:
                print(gitlab_src)
            break
" 2>/dev/null)

if [ -z "$GITLAB_SRC" ]; then
    echo "[WARNING] No _gitlab_src found in AppManifest.json, skipping download"
    exit 0
fi

echo "[INFO] GitLab source: $GITLAB_SRC"

# Download from GitLab to local workspace location
if [ -n "$GITLAB_API_TOKEN" ]; then
    echo "[INFO] Downloading from GitLab with authentication..."
    mkdir -p /workspaces/vehicle-app-cpp-template/downloaded-datamodel
    
    # Convert GitLab raw URL to API URL
    API_URL=$(python3 -c "
import urllib.parse
import re

url = '$GITLAB_SRC'
match = re.search(r'gitlab\.geniie\.net/(.+?)/-/raw/([^/]+)/(.+)', url)

if match:
    project_path = match.group(1)
    ref = match.group(2)
    file_path = match.group(3)
    
    project_encoded = urllib.parse.quote(project_path, safe='')
    file_encoded = urllib.parse.quote(file_path, safe='')
    
    api_url = f'https://gitlab.geniie.net/api/v4/projects/{project_encoded}/repository/files/{file_encoded}/raw?ref={ref}'
    print(api_url)
")
    
    curl -f -H "PRIVATE-TOKEN: $GITLAB_API_TOKEN" "$API_URL" -o /workspaces/vehicle-app-cpp-template/downloaded-datamodel/dataModel_basic.json
    
    if [ $? -eq 0 ]; then
        echo "[INFO] Successfully downloaded vehicle model to workspace"
    else
        echo "[ERROR] Failed to download from GitLab"
        exit 1
    fi
else
    echo "[WARNING] GITLAB_API_TOKEN not set, skipping download"
fi
