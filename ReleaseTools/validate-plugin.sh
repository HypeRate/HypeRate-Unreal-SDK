#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPOSITORY_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DESCRIPTOR="$REPOSITORY_ROOT/HypeRate.uplugin"

jq -e '
  .FileVersion == 3 and
  (.Version | type == "number") and
  (.VersionName | test("^[0-9]+\\.[0-9]+\\.[0-9]+([-.][0-9A-Za-z.-]+)?$")) and
  .FriendlyName == "HypeRate for Unreal Engine" and
  .CanContainContent == true and
  ([.Modules[] | select(.Name == "HypeRate" and .Type == "Runtime")] | length == 1) and
  ([.Modules[] | select(.Name == "HypeRateEditor" and .Type == "Editor")] | length == 1)
' "$DESCRIPTOR" >/dev/null

PLUGIN_VERSION="$(jq -r '.VersionName' "$DESCRIPTOR")"
if ! grep -Fq "## [$PLUGIN_VERSION]" "$REPOSITORY_ROOT/CHANGELOG.md"; then
  echo "CHANGELOG.md has no entry for $PLUGIN_VERSION." >&2
  exit 1
fi

required_paths=(
  Config
  Content
  Resources
  Source
  Source/HypeRate/HypeRate.Build.cs
  Source/HypeRate/Public/HypeRateClientComponent.h
  Source/HypeRateEditor/HypeRateEditor.Build.cs
  README.md
  LICENSE.md
)

for relative_path in "${required_paths[@]}"; do
  if [[ ! -e "$REPOSITORY_ROOT/$relative_path" ]]; then
    echo "Missing required plugin path: $relative_path" >&2
    exit 1
  fi
done

if find "$REPOSITORY_ROOT" \
  \( -path "$REPOSITORY_ROOT/.git" -o -path "$REPOSITORY_ROOT/Builds" \) -prune -o \
  -type d \( -name Binaries -o -name Intermediate -o -name Saved -o -name DerivedDataCache \) \
  -print -quit | grep -q .; then
  echo "Generated Unreal directories must not be committed." >&2
  exit 1
fi

if find "$REPOSITORY_ROOT" \
  \( -path "$REPOSITORY_ROOT/.git" -o -path "$REPOSITORY_ROOT/Builds" \) -prune -o \
  \( -name '.DS_Store' -o -name '*.xcuserstate' \) -print -quit | grep -q .; then
  echo "Local operating-system or IDE files are present." >&2
  exit 1
fi

if rg -n --hidden \
  --glob '!ReleaseTools/validate-plugin.sh' \
  --glob '!.git/**' \
  --glob '!Builds/**' \
  '(token=[A-Za-z0-9_-]{20,}|ApiToken[[:space:]]*=[[:space:]]*TEXT\("[^"]{12,}"\))' \
  "$REPOSITORY_ROOT"; then
  echo "Possible credential found in the plugin source." >&2
  exit 1
fi

while IFS= read -r path; do
  relative_path="${path#"$REPOSITORY_ROOT/"}"
  if (( ${#relative_path} >= 140 )); then
    echo "Plugin path is 140 characters or longer: $relative_path" >&2
    exit 1
  fi
done < <(
  find "$REPOSITORY_ROOT" -mindepth 1 \
    \( -path "$REPOSITORY_ROOT/.git" -o -path "$REPOSITORY_ROOT/Builds" \) -prune -o \
    -print | sort
)

python3 - "$REPOSITORY_ROOT/Resources/Icon128.png" <<'PY'
import struct
import sys

path = sys.argv[1]
with open(path, "rb") as image:
    signature = image.read(24)
if len(signature) != 24 or signature[:8] != b"\x89PNG\r\n\x1a\n":
    raise SystemExit("Resources/Icon128.png is not a valid PNG")
width, height = struct.unpack(">II", signature[16:24])
if (width, height) != (128, 128):
    raise SystemExit(f"Resources/Icon128.png must be 128x128, got {width}x{height}")
PY

echo "Validated HypeRate Unreal plugin $PLUGIN_VERSION"

