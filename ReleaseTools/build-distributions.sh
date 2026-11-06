#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 /path/to/Engine/Build/BatchFiles/RunUAT.sh" >&2
  exit 64
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPOSITORY_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
RUN_UAT="$1"
PLUGIN_FILE="$REPOSITORY_ROOT/HypeRate.uplugin"
VERSION="$(jq -r '.VersionName' "$PLUGIN_FILE")"
ENGINE_ROOT="$(cd "$(dirname "$RUN_UAT")/../.." && pwd)"
BUILD_VERSION_FILE="$ENGINE_ROOT/Build/Build.version"

if [[ ! -x "$RUN_UAT" ]]; then
  echo "RunUAT is not executable: $RUN_UAT" >&2
  exit 66
fi
if [[ ! -f "$BUILD_VERSION_FILE" ]]; then
  echo "Could not find the Engine version descriptor: $BUILD_VERSION_FILE" >&2
  exit 66
fi

ENGINE_VERSION="$(jq -r '(.MajorVersion | tostring) + "." + (.MinorVersion | tostring)' "$BUILD_VERSION_FILE")"
case "$(uname -s)" in
  Darwin) HOST_PLATFORM="Mac" ;;
  Linux) HOST_PLATFORM="Linux" ;;
  MINGW*|MSYS*|CYGWIN*) HOST_PLATFORM="Win64" ;;
  *) HOST_PLATFORM="UnknownHost" ;;
esac
VERSION_ROOT="$REPOSITORY_ROOT/Builds/$VERSION"
OUTPUT_ROOT="$VERSION_ROOT/UE_${ENGINE_VERSION}_$HOST_PLATFORM"
if [[ -e "$OUTPUT_ROOT" ]]; then
  echo "Release output already exists: $OUTPUT_ROOT" >&2
  echo "Move or remove that version directory before rebuilding." >&2
  exit 73
fi

"$SCRIPT_DIR/validate-plugin.sh"

# UE 5.8.4's UBA path handling can lose build products when macOS exposes the
# same temporary directory through both /var and /private/var. Keep this build
# deterministic and local without changing the user's global UBT settings.
export UnrealBuildTool_BuildConfiguration__bAllowUBAExecutor=false

TEMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/hyperate-unreal-release.XXXXXX")"
trap 'rm -rf "$TEMP_ROOT"' EXIT

PACKAGED_ROOT="$TEMP_ROOT/Packaged"
"$RUN_UAT" BuildPlugin \
  -Plugin="$PLUGIN_FILE" \
  -Package="$PACKAGED_ROOT" \
  -StrictIncludes \
  -Rocket

mkdir -p "$OUTPUT_ROOT"
mv "$PACKAGED_ROOT" "$OUTPUT_ROOT/HypeRate"
(
  cd "$OUTPUT_ROOT"
  zip -qry "HypeRate-Unreal-$VERSION-UE$ENGINE_VERSION-$HOST_PLATFORM-Fab.zip" HypeRate
)

SOURCE_ZIP="$VERSION_ROOT/HypeRate-Unreal-$VERSION-Source.zip"
if [[ ! -e "$SOURCE_ZIP" ]]; then
  SOURCE_ROOT="$TEMP_ROOT/HypeRate"
  mkdir -p "$SOURCE_ROOT"
  cp "$PLUGIN_FILE" "$SOURCE_ROOT/"
  cp "$REPOSITORY_ROOT/README.md" "$SOURCE_ROOT/"
  cp "$REPOSITORY_ROOT/CHANGELOG.md" "$SOURCE_ROOT/"
  cp "$REPOSITORY_ROOT/LICENSE.md" "$SOURCE_ROOT/"
  cp "$REPOSITORY_ROOT/ThirdPartyNotices.md" "$SOURCE_ROOT/"
  cp -R "$REPOSITORY_ROOT/Config" "$SOURCE_ROOT/"
  cp -R "$REPOSITORY_ROOT/Content" "$SOURCE_ROOT/"
  cp -R "$REPOSITORY_ROOT/Docs" "$SOURCE_ROOT/"
  cp -R "$REPOSITORY_ROOT/Resources" "$SOURCE_ROOT/"
  cp -R "$REPOSITORY_ROOT/Source" "$SOURCE_ROOT/"
  (
    cd "$TEMP_ROOT"
    zip -qry "$SOURCE_ZIP" HypeRate
  )
fi

echo "Created release artifacts in $OUTPUT_ROOT"

