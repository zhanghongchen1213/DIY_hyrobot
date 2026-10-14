#!/usr/bin/env bash
set -euo pipefail

docs_directory="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if ! command -v sphinx-autobuild >/dev/null 2>&1; then
    echo "请先激活文档虚拟环境，并执行 python -m pip install sphinx-autobuild。" >&2
    exit 1
fi

exec sphinx-autobuild "$docs_directory/source" "$docs_directory/build/html" "$@"
