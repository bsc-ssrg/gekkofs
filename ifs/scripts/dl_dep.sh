#!/bin/bash

#set -x

COMMON_CURL_FLAGS="--silent --fail --show-error --location -O"
COMMON_GIT_FLAGS="--quiet --single-branch"

# Stop all backround jobs on interruption.
# "kill -- -$$" sends a SIGTERM to the whole process group,
# thus killing also descendants.
trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM


exit_child() {
    if [ ! $? -eq 0 ]; then
        # notify the parent
        kill -s SIGTERM -- -$$
    fi
}

error_exit() {
    echo "$1" >&2   ## Send message to stderr. Exclude >&2 if you don't want it that way.
    exit "${2:-1}"  ## Return a code specified by $2 or 1 by default.
}

clonedeps() {
    set -e
    trap exit_child EXIT

    local FOLDER=$1
    local REPO=$2
    local COMMIT=$3
    local GIT_FLAGS=$4

    local ACTION

    if [ -d "${SOURCE}/${FOLDER}/.git" ]; then
        cd ${SOURCE}/${FOLDER} && git fetch -q
        ACTION="Pulled"
    else
        git clone ${COMMON_GIT_FLAGS} ${GIT_FLAGS} -- "${REPO}" "${SOURCE}/${FOLDER}"
        ACTION="Cloned"
    fi
    # fix the version
    cd "${SOURCE}/${FOLDER}" && git checkout -qf ${COMMIT}
    echo "${ACTION} ${FOLDER} [$COMMIT]"
}

wgetdeps() {
    set -e
    trap exit_child EXIT

    FOLDER=$1
    URL=$2
    if [ -d "${SOURCE}/${FOLDER}" ]; then
        rm -rf "${SOURCE}/${FOLDER}"
    fi
    mkdir -p "${SOURCE}/${FOLDER}"
    cd ${SOURCE}
    FILENAME=$(basename $URL)
    if [ -f "${SOURCE}/$FILENAME" ]; then
        rm -f "${SOURCE}/$FILENAME"
    fi
    curl ${COMMON_CURL_FLAGS} "$URL" || error_exit "Failed to download ${URL}" $?
    tar -xf "$FILENAME" --directory "${SOURCE}/${FOLDER}" --strip-components=1
    rm -f "$FILENAME"
    echo "Downloaded ${FOLDER}"
}

usage_short() {
	echo "
usage: dl_dep.sh [-h] [-n <NAPLUGIN>] [-c <CLUSTER>]
                    source_path
	"
}

help_msg() {

        usage_short
    echo "
This script gets all ADA-FS dependency sources (excluding the fs itself)

positional arguments:
        source_path              path where the dependency downloads are put


optional arguments:
        -h, --help              shows this help message and exits
        -n <NAPLUGIN>, --na <NAPLUGIN>
                                network layer that is used for communication. Valid: {bmi,cci,ofi,all}
                                defaults to 'all'
        -c <CLUSTER>, --cluster <CLUSTER>
                                additional configurations for specific compute clusters
                                supported clusters: {mogon1,mogon2,fh2}
        "
}
CLUSTER=""
NA_LAYER=""

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case ${key} in
    -n|--na)
    NA_LAYER="$2"
    shift # past argument
    shift # past value
    ;;
    -c|--cluster)
    CLUSTER="$2"
    shift # past argument
    shift # past value
    ;;
    -h|--help)
    help_msg
    exit
    #shift # past argument
    ;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

# positional arguments
if [[ -z ${1+x} ]]; then
    echo "Positional arguments missing."
    usage_short
    exit 1
fi
SOURCE="$( readlink -mn "${1}" )"

# optional arguments
if [ "${NA_LAYER}" == "" ]; then
        echo "Defaulting NAPLUGIN to 'all'"
        NA_LAYER="all"
fi

# sanity checks
if [[ ( "${NA_LAYER}" == "cci" ) || ( "${NA_LAYER}" == "bmi" ) || ( "${NA_LAYER}" == "ofi" ) || ( "${NA_LAYER}" == "all" ) ]]; then
	echo NAPLUGIN = "${NA_LAYER}"
else
    echo "No valid plugin selected"
    usage_short
    exit
fi
if [[ "${CLUSTER}" != "" ]]; then
	if [[ ( "${CLUSTER}" == "mogon1" ) || ( "${CLUSTER}" == "mogon2" ) || ( "${CLUSTER}" == "fh2" ) ]]; then
		echo CLUSTER  = "${CLUSTER}"
    else
        echo "${CLUSTER} cluster configuration is invalid. Exiting ..."
        usage_short
        exit
    fi
else
    echo "No cluster configuration set."
fi

echo "Source path is set to  \"${SOURCE}\""

mkdir -p ${SOURCE}

# get cluster dependencies
if [[ ( "${CLUSTER}" == "mogon1" ) || ( "${CLUSTER}" == "mogon2" ) || ( "${CLUSTER}" == "fh2" ) ]]; then
    # get libtool for cci
    wgetdeps "libtool" "https://ftp.gnu.org/gnu/libtool/libtool-2.4.6.tar.gz" &
    # get gflags for rocksdb
    wgetdeps "gflags" "https://github.com/gflags/gflags/archive/v2.2.1.tar.gz" &
    # get zstd for fast compression in rocksdb
    wgetdeps "zstd" "https://github.com/facebook/zstd/archive/v1.3.2.tar.gz" &
    # get zlib for rocksdb
    wgetdeps "lz4" "https://github.com/lz4/lz4/archive/v1.8.0.tar.gz" &
	# get snappy for rocksdb
    wgetdeps "snappy" "https://github.com/google/snappy/archive/1.1.7.tar.gz" &
fi
#if [ "${CLUSTER}" == "fh2" ]; then
	# no distinct 3rd party software needed as of now.
#fi

# get BMI
if [ "${NA_LAYER}" == "bmi" ] || [ "${NA_LAYER}" == "all" ]; then
    clonedeps "bmi" "http://git.mcs.anl.gov/bmi.git" "2abbe991edc45b713e64c5fed78a20fdaddae59b" &
fi
# get CCI
if [ "${NA_LAYER}" == "cci" ] || [ "${NA_LAYER}" == "all" ]; then
    clonedeps "cci" "https://github.com/CCI/cci" "58fd58ea2aa60c116c2b77c5653ae36d854d78f2" &
fi
# get libfabric
if [ "${NA_LAYER}" == "ofi" ] || [ "${NA_LAYER}" == "all" ]; then
    # No need to get libfabric for mogon2 as it is already installed
    if [[ ("${CLUSTER}" != "mogon2") ]]; then
        wgetdeps "libfabric" "https://github.com/ofiwg/libfabric/releases/download/v1.6.2/libfabric-1.6.2.tar.gz" &
    fi
fi
# get Mercury
clonedeps "mercury" "https://github.com/mercury-hpc/mercury" "v1.0.0"  "--recurse-submodules" &
# get Argobots
wgetdeps "argobots" "https://github.com/pmodels/argobots/archive/v1.0rc1.tar.gz" &
# get Margo
clonedeps "margo" "https://xgitlab.cels.anl.gov/sds/margo.git" "2676c8cf61874c4378ed699e3ca056636a4e152b" &
# get rocksdb
wgetdeps "rocksdb" "https://github.com/facebook/rocksdb/archive/v5.15.10.tar.gz" &
# get syscall_intercept
clonedeps "syscall_intercept" "https://github.com/pmem/syscall_intercept.git" "4b3a3b50f9d8d0127a4b39022d78b7369a534673" &

# Wait for all download to be completed
wait
