#!/bin/bash

set -euo pipefail

# Check if required tools are available
command -v curl >/dev/null 2>&1 || { echo "curl is required but not installed." >&2; exit 1; }
command -v jq >/dev/null 2>&1 || { echo "jq is required but not installed." >&2; exit 1; }

# Check if casectl.c exists
if [[ ! -f "casectl.c" ]]; then
    echo "casectl.c not found in the current directory." >&2
    exit 1
fi

# Check if OPENAI_API_KEY is set
if [[ -z "${OPENAI_API_KEY:-}" ]]; then
    echo "OPENAI_API_KEY environment variable not set." >&2
    exit 1
fi

# Create temporary files
temp_request=$(mktemp)
temp_response=$(mktemp)

# Clean up temp files on exit
trap 'rm -f "$temp_request" "$temp_response"' EXIT

# Prepare the request
cat > "$temp_request" <<EOF
{
  "model": "gpt-4",
  "messages": [
    {
      "role": "system",
      "content": "You are a code reviewer. Review the following C code for code smells, bugs, and best practices. Provide a structured report in Markdown format, including sections: Summary, Issues Found (with line numbers if possible), and Suggestions for Improvement."
    },
    {
      "role": "user",
      "content": $(jq -Rs '.' < casectl.c)
    }
  ],
  "temperature": 0.2
}
EOF

# Send the request
curl -s https://api.openai.com/v1/chat/completions \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer $OPENAI_API_KEY" \
    -d @"$temp_request" > "$temp_response"

# Extract and save the reply
jq -r '.choices[0].message.content' "$temp_response" > casectl_review.md

echo "Review saved to casectl_review.md"
