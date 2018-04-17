#!/bin/bash

usage_short() {
	echo "
usage: compile_dep.sh [-h] [-n <NAPLUGIN>] [-c <CLUSTER>] [-j <COMPILE_CORES>]
                      source_path install_path
	"
}

help_msg() {

	usage_short
    echo "
This script compiles all ADA-FS dependencies (excluding the fs itself)

positional arguments:
    source_path 	path to the cloned dependencies path from clone_dep.sh
    install_path    path to the install path of the compiled dependencies


optional arguments:
    -h, --help      shows this help message and exits
    -n <NAPLUGIN>, --na <NAPLUGIN>
                network layer that is used for communication. Valid: {bmi,cci,ofi,all}
                defaults to 'all'
    -c <CLUSTER>, --cluster <CLUSTER>
                additional configurations for specific compute clusters
                supported clusters: {mogon1,mogon2,fh2}
    -j <COMPILE_CORES>, --compilecores <COMPILE_CORES>
                number of cores that are used to compile the depdencies
                defaults to number of available cores
    -t, --test  Perform libraries tests.
"
}

prepare_build_dir() {
    if [ ! -d "$1/build" ]; then
        mkdir $1/build
    fi
    rm -rf $1/build/*
}
PATCH_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PATCH_DIR="${PATCH_DIR}/patches"
CLUSTER=""
NA_LAYER=""
CORES=""
SOURCE=""
INSTALL=""

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
	-j|--compilecores)
    CORES="$2"
    shift # past argument
    shift # past value
    ;;
    -t|--test)
    PERFORM_TEST=true
    shift
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

# deal with positional arguments
if [[ ( -z ${1+x} ) || ( -z ${2+x} ) ]]; then
    echo "Positional arguments missing."
    usage_short
    exit
fi
SOURCE="$( readlink -f "${1}" )"
INSTALL="$( readlink -f "${2}" )"

# deal with optional arguments
if [ "${NA_LAYER}" == "" ]; then
	echo "Defaulting NAPLUGIN to 'all'"
	NA_LAYER="all"
fi
if [ "${CORES}" == "" ]; then
	CORES=$(grep -c ^processor /proc/cpuinfo)
	echo "CORES = ${CORES} (default)"
else
	if [ ! "${CORES}" -gt "0" ]; then
		echo "CORES set to ${CORES} which is invalid.
Input must be numeric and greater than 0."
		usage_short
		exit
	else
		echo CORES    = "${CORES}"
	fi
fi
if [ "${NA_LAYER}" == "cci" ] || [ "${NA_LAYER}" == "bmi" ] || [ "${NA_LAYER}" == "ofi" ] || [ "${NA_LAYER}" == "all" ]; then
	echo NAPLUGIN = "${NA_LAYER}"
else
    echo "No valid plugin selected"
    usage_short
    exit
fi
if [[ "${CLUSTER}" != "" ]]; then
	if [[ ( "${CLUSTER}" == "mogon1" ) || ( "${CLUSTER}" == "fh2" ) || ( "${CLUSTER}" == "mogon2" ) ]]; then
		echo CLUSTER  = "${CLUSTER}"
    else
        echo "${CLUSTER} cluster configuration is invalid. Exiting ..."
        usage_short
        exit
    fi
else
    echo "No cluster configuration set."
fi

#LOG=/tmp/adafs_install.log
#echo "" &> $LOG
USE_BMI="-DNA_USE_BMI:BOOL=OFF"
USE_CCI="-DNA_USE_CCI:BOOL=OFF"
USE_OFI="-DNA_USE_OFI:BOOL=OFF"

echo "Source path = '$1'";
echo "Install path = '$2'";

mkdir -p ${SOURCE}

######### From now on exits on any error ########
set -e

# Set cluster dependencies first
if [[ ( "${CLUSTER}" == "mogon1" ) || ( "${CLUSTER}" == "fh2" ) || ( "${CLUSTER}" == "mogon2" ) ]]; then
    # get libtool
    echo "############################################################ Installing:  libtool"
    CURR=${SOURCE}/libtool
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    ../configure --prefix=${INSTALL}
    make -j${CORES}
    make install
    # compile libev
    echo "############################################################ Installing:  libev"
    CURR=${SOURCE}/libev
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    ../configure --prefix=${INSTALL}
    make -j${CORES}
    make install
    # compile gflags
    echo "############################################################ Installing:  gflags"
    CURR=${SOURCE}/gflags
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    cmake -DCMAKE_INSTALL_PREFIX=${INSTALL} -DCMAKE_BUILD_TYPE:STRING=Release ..
    make -j${CORES}
    make install
    # compile zstd
    echo "############################################################ Installing:  zstd"
    CURR=${SOURCE}/zstd/build/cmake
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    cmake -DCMAKE_INSTALL_PREFIX=${INSTALL} -DCMAKE_BUILD_TYPE:STRING=Release ..
    make -j${CORES}
    make install
    echo "############################################################ Installing:  lz4"
    CURR=${SOURCE}/lz4
	cd ${CURR}
    make -j${CORES}
    make DESTDIR=${INSTALL} PREFIX="" install
    echo "############################################################ Installing:  snappy"
    CURR=${SOURCE}/snappy
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    cmake -DCMAKE_INSTALL_PREFIX=${INSTALL} -DCMAKE_BUILD_TYPE:STRING=Release ..
    make -j${CORES}
    make install
fi

if [ "$NA_LAYER" == "bmi" ] || [ "$NA_LAYER" == "all" ]; then
    USE_BMI="-DNA_USE_BMI:BOOL=ON"
    echo "############################################################ Installing:  BMI"
    # BMI
    CURR=${SOURCE}/bmi
    prepare_build_dir ${CURR}
    cd ${CURR}
    ./prepare
    cd ${CURR}/build
    ../configure --prefix=${INSTALL} --enable-shared --disable-static --disable-karma --enable-bmi-only --enable-fast --disable-strict
    make -j${CORES}
    make install
fi

if [ "$NA_LAYER" == "cci" ] || [ "$NA_LAYER" == "all" ]; then
    USE_CCI="-DNA_USE_CCI:BOOL=ON"
    echo "############################################################ Installing:  CCI"
    # CCI
    CURR=${SOURCE}/cci
    prepare_build_dir ${CURR}
    cd ${CURR}
    # patch hanging issue
    echo "########## ADA-FS injection: Applying cci hanging patch"
    git apply ${PATCH_DIR}/cci_hang_final.patch
    echo "########## ADA-FS injection: Disabling cci debug mode/devel mode entirely"
    git apply ${PATCH_DIR}/cci_remove_devel_mode.patch
    ./autogen.pl
    cd ${CURR}/build
if [[ ("${CLUSTER}" == "mogon1") || ("${CLUSTER}" == "fh2") ]]; then
    ../configure --with-verbs --prefix=${INSTALL} LIBS="-lpthread"
else
    ../configure --prefix=${INSTALL} LIBS="-lpthread"
fi

    echo "########## ADA-FS injection: Replacing any remaining CFLAGS with '-g -O2' that are added by cci although debug mode is disabled with '-O3'"
    find . -type f -exec sed -i 's/-g -O2/-O3/g' {} \;
    make -j${CORES}
    make install
    [ "${PERFORM_TEST}" ] && make check
fi

if [ "$NA_LAYER" == "ofi" ] || [ "$NA_LAYER" == "all" ]; then
    USE_OFI="-DNA_USE_OFI:BOOL=ON"
    # Mogon2 already has libfabric installed in a version that Mercury supports.
    if [[ ("${CLUSTER}" != "mogon2") ]]; then
        echo "############################################################ Installing:  LibFabric"
        #libfabric
        CURR=${SOURCE}/libfabric
        prepare_build_dir ${CURR}
        cd ${CURR}
        ./autogen.sh
        cd ${CURR}/build
        ../configure --prefix=${INSTALL}
        make -j${CORES}
        make install
        [ "${PERFORM_TEST}" ] && make check
    fi
fi

echo "############################################################ Installing:  Mercury"

# Mercury
CURR=${SOURCE}/mercury
# check for specific Mercury version which has to be newer than from 25-02-2018
MERCURY_VERSION=$(cd ${CURR} && git log --since 02-25-2018 | wc -l)
echo $MERCURY_VERSION
if [ ${MERCURY_VERSION} -eq 0 ]; then
    echo "########## Mercury version is too old. Pulling new version ..."
    cd $CURR && git checkout d015745ce25d839b8b46e68c11a7d8278423a46b
fi
prepare_build_dir ${CURR}
cd ${CURR}
if [ "$NA_LAYER" == "cci" ] || [ "$NA_LAYER" == "all" ]; then
    # patch cci verbs addr lookup error handling
    echo "########## Applying cci addr lookup error handling patch"
    git apply ${PATCH_DIR}/mercury_cci_verbs_lookup.patch
fi
cd ${CURR}/build
cmake -DMERCURY_USE_SELF_FORWARD:BOOL=ON -DMERCURY_USE_CHECKSUMS:BOOL=OFF -DBUILD_TESTING:BOOL=ON \
-DMERCURY_USE_BOOST_PP:BOOL=ON -DBUILD_SHARED_LIBS:BOOL=ON -DCMAKE_INSTALL_PREFIX=${INSTALL} \
-DCMAKE_BUILD_TYPE:STRING=Release -DMERCURY_USE_EAGER_BULK:BOOL=ON ${USE_BMI} ${USE_CCI} ${USE_OFI} ../
make -j${CORES}
make install

echo "############################################################ Installing:  Argobots"

# Argobots
CURR=${SOURCE}/argobots
prepare_build_dir ${CURR}
cd ${CURR}
./autogen.sh
cd ${CURR}/build
../configure --prefix=${INSTALL}
make -j${CORES}
make install
[ "${PERFORM_TEST}" ] && make check

echo "############################################################ Installing:  Abt-snoozer"
# Abt snoozer
CURR=${SOURCE}/abt-snoozer
prepare_build_dir ${CURR}
cd ${CURR}
./prepare.sh
cd ${CURR}/build
../configure --prefix=${INSTALL} PKG_CONFIG_PATH=${INSTALL}/lib/pkgconfig
make -j${CORES}
make install
[ "${PERFORM_TEST}" ] && make check

#echo "############################################################ Installing:  Abt-IO"
## Abt IO
#CURR=${SOURCE}/abt-io
#prepare_build_dir ${CURR}
#cd ${CURR}
#echo "########## ADA-FS injection: Applying abt-io c++ template clash patch"
#git apply ${PATCH_DIR}/abt_io_cplusplus_template_clash.patch
#./prepare.sh
#cd ${CURR}/build
#../configure --prefix=${INSTALL} PKG_CONFIG_PATH=${INSTALL}/lib/pkgconfig
#make -j${CORES}
#make install
#[ "${PERFORM_TEST}" ] && make check  # The tests create so huge files that breaks memory :D

echo "############################################################ Installing:  Margo"
# Margo
CURR=${SOURCE}/margo
prepare_build_dir ${CURR}
cd ${CURR}
./prepare.sh
cd ${CURR}/build
../configure --prefix=${INSTALL} PKG_CONFIG_PATH=${INSTALL}/lib/pkgconfig CFLAGS="-Wall -O3"
make -j${CORES}
make install
[ "${PERFORM_TEST}" ] && make check

echo "############################################################ Installing:  Rocksdb"
# Rocksdb
CURR=${SOURCE}/rocksdb
cd ${CURR}
make clean
make USE_RTTI=1 -j${CORES} static_lib
make INSTALL_PATH=${INSTALL} install

echo "Done"
