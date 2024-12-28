#!/usr/bin/env bash
# Copyright (c) 2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -exo pipefail;

BASENAME="$(basename "${0}")"

if [ -z "${BUILD_TARGET}" ]; then
    echo "${BASENAME}: BUILD_TARGET not defined, cannot continue!";
    exit 1;
elif [ -z "${HOST}" ]; then
    echo "${BASENAME}: HOST not defined, cannot continue!";
    exit 1;
elif [ ! -d "depends" ]; then
    echo "${BASENAME}: 'depends' directory missing, cannot continue!";
    echo "Are you in the source's root directory?";
    exit 1;
elif  [ ! "$(command -v zstd)" ]; then
    echo "${BASENAME}: 'zstd' not found, cannot continue!";
    exit 1;
elif  [ -z "${1}" ]; then
    echo "${BASENAME}: Please specify verb ('create' or 'extract')";
    exit 1;
fi

DEPENDS_ARCHIVE="depends_${BUILD_TARGET}.tar.zst"
DEPENDS_PATHS=(
    "depends/${HOST}"
    "depends/built"
)

if [ "${1}" == "create" ]; then
    if [ -f "${DEPENDS_ARCHIVE}" ]; then
        echo "${BASENAME}: Deleting existing archive..."
        rm "${DEPENDS_ARCHIVE}";
    fi
    echo "${BASENAME}: Creating new archive..."
    mkdir -p "${DEPENDS_PATHS[@]}"
    tar --create \
        --preserve-permissions \
        --to-stdout \
        "${DEPENDS_PATHS[@]}" | zstd -5 -T0 > "${DEPENDS_ARCHIVE}";
elif [ "${1}" == "extract" ]; then
    if [ ! -f "${DEPENDS_ARCHIVE}" ]; then
        echo "${BASENAME}: Cannot extract archive, '${DEPENDS_ARCHIVE}' not found";
    fi
    tar --extract \
        --same-owner \
        --same-permissions \
        --no-overwrite-dir \
        --use-compress-program=zstd \
        --file "${DEPENDS_ARCHIVE}";
else
    echo "${BASENAME}: Unrecognized verb";
    exit 1;
fi

exit 0;
