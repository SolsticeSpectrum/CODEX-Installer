#!/bin/bash
UNARC="$(dirname "$0")/../../app/include/unarc"
PATCHES="$(dirname "$0")/../../patches"
cd "$UNARC"
for p in "$PATCHES"/*.patch; do
    git apply --check "$p" 2>/dev/null && git apply "$p" && echo "applied $(basename $p)"
done
