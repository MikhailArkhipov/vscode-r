#!/bin/sh

usage()
{
    cat << EOF
Usage: $0 [options]
 OPTIONS:
    -h        Print this message.
    -d        Build debug binaries.
EOF
}

BUILD_TYPE=Release

OPTIND=1

while getopts "h?d" opt; do
    case "$opt" in
    h|\?)
        usage
        exit 0
        ;;
    d)  BUILD_TYPE=Debug
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift
mkdir -p build && \
cd build && \
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. && \
make
