#!/bin/bash
# Script to download files from GitLab with API token authentication

URL="$1"
OUTPUT_FILE="$2"

if [ -z "$URL" ] || [ -z "$OUTPUT_FILE" ]; then
    echo "Usage: $0 <url> <output_file>"
    exit 1
fi

# Check if this is a GitLab URL and if we have a token
if [[ "$URL" =~ gitlab\.geniie\.net ]] && [ -n "$GITLAB_API_TOKEN" ]; then
    echo "Downloading from GitLab with authentication..."
    curl -f -H "PRIVATE-TOKEN: $GITLAB_API_TOKEN" "$URL" -o "$OUTPUT_FILE"
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "Failed to download file from GitLab"
        exit $exit_code
    fi
else
    echo "Downloading without authentication..."
    curl -f "$URL" -o "$OUTPUT_FILE"
fi
