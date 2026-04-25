#!/usr/bin/env bash
set -eu

python3 -m west debug --runner jlink "$@"
