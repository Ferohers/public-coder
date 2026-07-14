#!/bin/sh
set -eu

WORKSPACE_DIR="${WORKSPACE:-/app/workspace}"
RETRO68_DIR="${RETRO68_ROOT:-/opt/retro68/toolchain}"
BUILD_DIR_NAME="${BUILD_DIR_NAME:-build_macos9}"

normalize_cmakelists() {
    dir="$1"
    if [ ! -d "$dir" ]; then
        return 1
    fi
    if [ -f "$dir/CMakeLists.txt" ]; then
        return 0
    fi
    found_case="$(find "$dir" -maxdepth 1 -iname CMakeLists.txt -type f | head -n 1 || true)"
    if [ -n "$found_case" ]; then
        cp "$found_case" "$dir/CMakeLists.txt"
        return 0
    fi
    return 1
}

check_tool_libs() {
    tool_name="$1"
    tool_path="$2"
    if [ -z "$tool_path" ] || [ ! -f "$tool_path" ]; then
        return 0
    fi
    missing_libs="$(ldd "$tool_path" 2>/dev/null | awk '/not found/ { print $1 }' | tr '\n' ' ' || true)"
    if [ -n "$missing_libs" ]; then
        echo "Retro68 $tool_name shared libraries are missing in this container: $missing_libs"
        echo "Rebuild the bridge image with the missing Debian runtime packages installed."
        exit 2
    fi
}

check_retro68_libs() {
    missing_report="$(
        find "$RETRO68_DIR/bin" "$RETRO68_DIR/libexec" -type f -perm -111 2>/dev/null |
        while IFS= read -r tool_path; do
            missing_libs="$(ldd "$tool_path" 2>/dev/null | awk '/not found/ { print $1 }' | tr '\n' ' ' || true)"
            if [ -n "$missing_libs" ]; then
                printf '%s: %s\n' "$tool_path" "$missing_libs"
            fi
        done
    )"
    if [ -n "$missing_report" ]; then
        echo "Retro68 toolchain shared libraries are missing in this container:"
        echo "$missing_report"
        echo "Install the missing Debian runtime packages in the bridge image and rebuild it."
        exit 2
    fi
}

find_project_dir() {
    if [ -n "${BUILD_PROJECT_DIR:-}" ]; then
        if normalize_cmakelists "$WORKSPACE_DIR/$BUILD_PROJECT_DIR"; then
            printf '%s\n' "$WORKSPACE_DIR/$BUILD_PROJECT_DIR"
            return 0
        fi
        if normalize_cmakelists "$BUILD_PROJECT_DIR"; then
            printf '%s\n' "$BUILD_PROJECT_DIR"
            return 0
        fi
    fi

    if normalize_cmakelists "$WORKSPACE_DIR"; then
        printf '%s\n' "$WORKSPACE_DIR"
        return 0
    fi

    if normalize_cmakelists "$WORKSPACE_DIR/macos9_client"; then
        printf '%s\n' "$WORKSPACE_DIR/macos9_client"
        return 0
    fi

    found="$(find "$WORKSPACE_DIR" -maxdepth 3 -iname CMakeLists.txt -type f | head -n 1 || true)"
    if [ -n "$found" ]; then
        project="$(dirname "$found")"
        normalize_cmakelists "$project" >/dev/null 2>&1 || true
        printf '%s\n' "$project"
        return 0
    fi

    return 1
}

if [ ! -x "$RETRO68_DIR/bin/powerpc-apple-macos-gcc" ]; then
    echo "Retro68 toolchain not found at $RETRO68_DIR."
    echo "Expected compiler: $RETRO68_DIR/bin/powerpc-apple-macos-gcc"
    echo "On the Docker host, set RETRO68_HOST_ROOT to the folder that contains bin/powerpc-apple-macos-gcc."
    echo "Example: RETRO68_HOST_ROOT=/opt/retro68/toolchain"
    exit 2
fi

cc1_path="$("$RETRO68_DIR/bin/powerpc-apple-macos-gcc" -print-prog-name=cc1 2>/dev/null || true)"
check_tool_libs "compiler" "$cc1_path"
check_tool_libs "Rez" "$RETRO68_DIR/bin/Rez"
check_retro68_libs

if ! project_dir="$(find_project_dir)"; then
    echo "No CMakeLists.txt found under $WORKSPACE_DIR."
    echo "Put a Retro68/CMake Mac OS 9 project in the workspace, or set BUILD_PROJECT_DIR."
    exit 2
fi

toolchain_file="$project_dir/retroppc.toolchain.cmake"
if [ ! -f "$toolchain_file" ]; then
    toolchain_file="${RETRO68_DIR}/powerpc-apple-macos/cmake/retroppc.toolchain.cmake"
fi
if [ ! -f "$toolchain_file" ]; then
    echo "retroppc.toolchain.cmake not found in project or Retro68 toolchain."
    exit 2
fi

echo "Project: $project_dir"
echo "Retro68: $RETRO68_DIR"
echo "Toolchain: $toolchain_file"

cmake -S "$project_dir" -B "$project_dir/$BUILD_DIR_NAME" \
    -DCMAKE_TOOLCHAIN_FILE="$toolchain_file" \
    -DRETRO68_ROOT="$RETRO68_DIR"
cmake --build "$project_dir/$BUILD_DIR_NAME"
