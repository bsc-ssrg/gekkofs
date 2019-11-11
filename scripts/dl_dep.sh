#!/bin/bash

#set -x

COMMON_CURL_FLAGS="--silent --fail --show-error --location -O"
COMMON_GIT_FLAGS="--quiet --single-branch"
PATCH_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PATCH_DIR="${PATCH_DIR}/patches"
CLUSTER=""
DEPENDENCY=""
NA_LAYER=""

MOGON2_DEPS=(
    "zstd" "lz4" "snappy" "bmi" "mercury" "argobots" "margo" "rocksdb" 
    "capstone" "syscall_intercept" "date"
)

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

list_dependencies() {

    echo "Available dependencies: "

    echo -n "  Mogon 2: "
    for d in "${MOGON2_DEPS[@]}"
    do
        echo -n "$d "
    done
    echo ""

}

clonedeps() {
    set -ex
    trap exit_child EXIT

    local FOLDER=$1
    local REPO=$2
    local COMMIT=$3
    local GIT_FLAGS=$4
    local PATCH=$5

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

    # apply patch if provided
    if [ ! -z ${PATCH} ]; then
        git apply --verbose ${PATCH_DIR}/${PATCH}
    fi

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
usage: dl_dep.sh [-h] [-l] [-n <NAPLUGIN>] [-c <CLUSTER>] [-d <DEPENDENCY>] 
                    source_path
	"
}

help_msg() {

        usage_short
    echo "
This script gets all GekkoFS dependency sources (excluding the fs itself)

positional arguments:
        source_path              path where the dependency downloads are put


optional arguments:
        -h, --help              shows this help message and exits
        -l, --list-dependencies
                                list dependencies available for download
        -n <NAPLUGIN>, --na <NAPLUGIN>
                                network layer that is used for communication. Valid: {bmi,ofi,all}
                                defaults to 'all'
        -c <CLUSTER>, --cluster <CLUSTER>
                                additional configurations for specific compute clusters
                                supported clusters: {mogon2}
        -d <DEPENDENCY>, --dependency <DEPENDENCY>
                                download a specific dependency. If unspecified 
                                all dependencies are built and installed.
        "
}

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
    -d|--dependency)
    if [[ -z "$2" ]]; then
        echo "Missing argument for -d/--dependency option"
        exit
    fi
    DEPENDENCY="$2"
    shift # past argument
    shift # past value
    ;;
    -l|--list-dependencies)
    list_dependencies
    exit
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
if [[ ( "${NA_LAYER}" == "bmi" ) || ( "${NA_LAYER}" == "ofi" ) || ( "${NA_LAYER}" == "all" ) ]]; then
	echo NAPLUGIN = "${NA_LAYER}"
else
    echo "No valid plugin selected"
    usage_short
    exit
fi
if [[ "${CLUSTER}" != "" ]]; then
	if [[ ( "${CLUSTER}" == "mogon2" ) ]]; then
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
if [[ ( "${CLUSTER}" == "mogon2" ) ]]; then

    # get zstd for fast compression in rocksdb
    if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "zstd" ) ]]; then
        wgetdeps "zstd" "https://github.com/facebook/zstd/archive/v1.3.2.tar.gz" &
    fi

    # get zlib for rocksdb
    if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "zstd" ) ]]; then
        wgetdeps "lz4" "https://github.com/lz4/lz4/archive/v1.8.0.tar.gz" &
    fi

	  # get snappy for rocksdb
    if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "snappy" ) ]]; then
            wgetdeps "snappy" "https://github.com/google/snappy/archive/1.1.7.tar.gz" &
    fi

    # get capstone for syscall-intercept
    if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "capstone" ) ]]; then
      wgetdeps "capstone" "https://github.com/aquynh/capstone/archive/4.0.1.tar.gz" &
    fi
fi

# get BMI
if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "bmi" ) ]]; then
    if [ "${NA_LAYER}" == "bmi" ] || [ "${NA_LAYER}" == "all" ]; then
        clonedeps "bmi" "https://xgitlab.cels.anl.gov/sds/bmi.git" "81ad0575fc57a69269a16208417cbcbefa51f9ea" &
    fi
fi

# get libfabric
if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "ofi" ) ]]; then
    if [ "${NA_LAYER}" == "ofi" ] || [ "${NA_LAYER}" == "all" ]; then
        # No need to get libfabric for mogon2 as it is already installed
        if [[ ("${CLUSTER}" != "mogon2") ]]; then
            wgetdeps "libfabric" "https://github.com/ofiwg/libfabric/releases/download/v1.8.1/libfabric-1.8.1.tar.bz2" &
        fi
    fi
fi

# get Mercury
if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "mercury" ) ]]; then
    clonedeps "mercury" "https://github.com/mercury-hpc/mercury" "fd410dfb9852b2b98d21113531f3058f45bfcd64"  "--recurse-submodules" &
fi

# get Argobots
if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "argobots" ) ]]; then
    wgetdeps "argobots" "https://github.com/pmodels/argobots/archive/v1.0rc1.tar.gz" &
fi

# get Margo
if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "margo" ) ]]; then
    clonedeps "margo" "https://xgitlab.cels.anl.gov/sds/margo.git" "016dbdce22da3fe4f97b46c20a53bced9370a217" &
fi

# get rocksdb
if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "rocksdb" ) ]]; then
    wgetdeps "rocksdb" "https://github.com/facebook/rocksdb/archive/v6.2.2.tar.gz" &
fi

# get syscall_intercept
if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "syscall_intercept" ) ]]; then
    clonedeps "syscall_intercept" "https://github.com/pmem/syscall_intercept.git" "cc3412a2ad39f2e26cc307d5b155232811d7408e" "" "syscall_intercept.patch" &
fi

# get date
if [[ ( "${DEPENDENCY}" == "" ) || ( "${DEPENDENCY}" == "date" ) ]]; then
    clonedeps "date" "https://github.com/HowardHinnant/date.git" "e7e1482087f58913b80a20b04d5c58d9d6d90155" &
fi

# Wait for all download to be completed
wait
