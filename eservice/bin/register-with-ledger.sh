#!/bin/bash

# Copyright 2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


PY3_VERSION=$(python --version | sed 's/Python 3\.\([0-9]\).*/\1/')
if [[ $PY3_VERSION -lt 5 ]]; then
    echo activate python3 first
    exit 101
fi

SCRIPTDIR="$(dirname $(readlink --canonicalize ${BASH_SOURCE}))"
SRCDIR="$(realpath ${SCRIPTDIR}/../..)"

function yell {
    echo "$0: $*" >&2;
}

function die {
    yell "$*"
    exit 111
}

function try {
    "$@" || die "operation failed: $*"
}

eservice_enclave_info_file=$(mktemp /tmp/pdo-test.XXXXXXXXX)

function cleanup {
    yell "Clean up temporary files"
    rm -f ${eservice_enclave_info_file}
}

trap cleanup EXIT

#Set SPID to parameter if passed
SPID=$PDO_SPID
if (( "$#" == 1 )) ; then
    SPID=$1
fi

# Store MR_ENCLAVE & MR_BASENAME to eservice_enclave_info_file
# Note: an alternative way without any enclave invocations would be the following.
#
#    if [ -z "${SPID}" -o ${#SPID} != 32 ]; then
#	echo "No valid (length 32) SPID pass as argument or PDO_SPID environment variable"
#	exit 1
#    fi
#    perl -0777 -ne 'if (/metadata->enclave_css.body.enclave_hash.m:([a-fx0-9 \n]+)/) { $eh = $1; $eh=~s/0x| |\n//g; $eh=~tr/a-z/A-Z/; $bn="'${SPID}'"; $bn .= "0" x (64 - length $bn); print "MRENCLAVE:${eh}\nBASENAME:${bn}\n"; }' ./build/lib/libpdo-enclave.signed.so.meta > $eservice_enclave_info_file
#    # Note: group id is always zero, hence the zero-padding ...
#
# This would also allow removing in eservice/pservice the code related to CreateErsatzEnclaveReport and GetEnclave Characteristics
# However, getting basename via enclave invocation & quote is somewhat cleaner than below ..
function Store {
    : "${SPID:?Need PDO_SPID environment variable set or passed in for valid MR_BASENAME}"
    yell Compute the enclave information
    try eservice-enclave-info --spid ${SPID} --save ${eservice_enclave_info_file} --loglevel warn
}

# Registers MR_ENCLAVE & BASENAMES with Ledger
function Register {
    if [ ! -f $eservice_enclave_info_file ]; then
        yell Registration failed! eservice_enclave_info_file not found!
    else
        VAR_MRENCLAVE=$(grep -o 'MRENCLAVE:.*' ${eservice_enclave_info_file} | cut -f2- -d:)
        VAR_BASENAME=$(grep -o 'BASENAME:.*' ${eservice_enclave_info_file} | cut -f2- -d:)

        : "${PDO_LEDGER_URL:?Registration failed! PDO_LEDGER_URL environment variable not set}"
        : "${PDO_LEDGER_KEY_SKF:?Registration failed! PDO_LEDGER_KEY_SKF environment variable not set}"
        : "PDO_IAS_KEY_PEM" "${PDO_IAS_KEY_PEM:?Registration failed! PDO_IAS_KEY_PEM environment variable not set}"

        try ${SRCDIR}/sawtooth/bin/pdo-cli set-setting --keyfile $PDO_LEDGER_KEY_SKF --url $PDO_LEDGER_URL \
            pdo.test.registry.measurements ${VAR_MRENCLAVE}
        try ${SRCDIR}/sawtooth/bin/pdo-cli set-setting --keyfile $PDO_LEDGER_KEY_SKF --url $PDO_LEDGER_URL \
            pdo.test.registry.basenames ${VAR_BASENAME}
        try ${SRCDIR}/sawtooth/bin/pdo-cli set-setting --keyfile $PDO_LEDGER_KEY_SKF --url $PDO_LEDGER_URL \
            pdo.test.registry.public_key "$(cat $PDO_IAS_KEY_PEM)"
    fi
}

if [ "$SGX_MODE" = "HW" ]; then
    Store
    Register
else
    yell Registration failed! SGX_MODE not set to HW
fi
