#!/usr/bin/env bash
if command -v pacman >/dev/null 2>&1; then
    proto_xml=$(pacman -Ql wlr-protocols 2>/dev/null |
                awk '$2 ~ /wlr-output-management/ {print $2; exit}')
fi

: "${proto_xml:=/usr/share/wlr-protocols/unstable/wlr-output-management-unstable-v1.xml}"

if [[ ! -e "$proto_xml" ]]; then
    echo "Could not find wlr protocol stub. Please extend this script to find it for your package manager"
    exit 1
fi

wayland-scanner client-header "$proto_xml" \
                zwlr-output-management-unstable-v1-client-protocol.h
wayland-scanner private-code "$proto_xml" \
                zwlr-output-management-unstable-v1-protocol.c
