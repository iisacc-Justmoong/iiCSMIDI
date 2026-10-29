#!/usr/bin/env bash
set -euo pipefail

source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${source_dir}/build"
install_prefix="${INSTALL_PREFIX:-${HOME}/.local/SDK/iiCSMIDI}"
cmake_prefix_path="${CMAKE_PREFIX_PATH:-}"
cmake_prefix_path="${cmake_prefix_path//:/;}"
if [[ $# -ne 0 ]]; then
    echo "Use INSTALL_PREFIX and CMAKE_PREFIX_PATH environment variables." >&2
    exit 2
fi
if [[ "$install_prefix" != /* ]]; then
    echo "INSTALL_PREFIX must be an absolute path." >&2
    exit 2
fi

cmake -S "$source_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON \
    -DCMAKE_INSTALL_PREFIX="$install_prefix" \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
    -DCMAKE_PREFIX_PATH="$cmake_prefix_path"
cmake --build "$build_dir" --config Release --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
ctest --test-dir "$build_dir" -C Release --output-on-failure
cmake --install "$build_dir" --config Release

consumer_build_dir="${build_dir}/consumer/build"
cmake -S "${source_dir}/tests/consumer" -B "$consumer_build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DiiCSMIDI_DIR="${install_prefix}/lib/cmake/iiCSMIDI" \
    -DCMAKE_PREFIX_PATH="${install_prefix}${cmake_prefix_path:+;${cmake_prefix_path}}"
cmake --build "$consumer_build_dir" --config Release --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
ctest --test-dir "$consumer_build_dir" -C Release --output-on-failure

if [[ "${IICSMIDI_TEST_FILE_PROVIDER:-OFF}" == ON ]]; then
    file_adapter_build="${build_dir}/file-consumer/build"
    cmake -S "${source_dir}/example/FileProvider" -B "$file_adapter_build" \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
        -DiiCSMIDI_DIR="${install_prefix}/lib/cmake/iiCSMIDI" \
        -DCMAKE_PREFIX_PATH="${install_prefix}${cmake_prefix_path:+;${cmake_prefix_path}}"
    cmake --build "$file_adapter_build" --config Release --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
    ctest --test-dir "$file_adapter_build" -C Release --output-on-failure
fi

if [[ "${IICSMIDI_INSTALL_ASYNC:-OFF}" == ON ]]; then
    async_adapter_build="${build_dir}/core-async/build"
    cmake -S "${source_dir}/example/CoreAsync" -B "$async_adapter_build" \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
        -DiiCSMIDI_DIR="${install_prefix}/lib/cmake/iiCSMIDI" \
        -DCMAKE_PREFIX_PATH="${install_prefix}${cmake_prefix_path:+;${cmake_prefix_path}}"
    cmake --build "$async_adapter_build" --config Release --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
    ctest --test-dir "$async_adapter_build" -C Release --output-on-failure
    cmake --install "$async_adapter_build" --config Release --prefix "$install_prefix"
    async_build_dir="${build_dir}/async-consumer/build"
    cmake -S "${source_dir}/tests/async-consumer" -B "$async_build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DiiCSMIDIAsync_DIR="${install_prefix}/lib/cmake/iiCSMIDIAsync" \
        -DCMAKE_PREFIX_PATH="${install_prefix}${cmake_prefix_path:+;${cmake_prefix_path}}"
    cmake --build "$async_build_dir" --config Release --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
    ctest --test-dir "$async_build_dir" -C Release --output-on-failure
fi
