#!/bin/bash
# scripts/test_art_cli.sh
set -e

CLI_PATH="./build/rtgui/ART-cli"
RAW_FILE="data/fivek_sample/sample1.nef"
PROFILE="data/test_profile.arp"
OUT_DIR="data/fivek_sample"

echo "Running ART-cli on $RAW_FILE..."

if [ ! -f "$CLI_PATH" ]; then
    echo "ART-cli not found. Cannot proceed."
    exit 1
fi

rm -f "${OUT_DIR}/sample1.jpg"

"$CLI_PATH" -a -Y -p "$PROFILE" -c "$RAW_FILE"

if [ -f "${OUT_DIR}/sample1.jpg" ]; then
    echo "SUCCESS: Output JPEG generated."
else
    echo "FAILED: Output JPEG not found."
    exit 1
fi
