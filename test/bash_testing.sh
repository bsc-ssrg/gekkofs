#!/bin/bash
ROOT=${1}
DIR=${ROOT}/nonexist_deep0

echo Removing a nonexist file

rm -rvf ${DIR}/nonexist_deep1
retval=$?

echo Remove non exising directory result : $retval

if [ $retval -ne 0 ]; then
        exit 1
fi


if [ -d $DIR/nonexist2_deep1 ]; then
        echo Nonexist2 exists -error-
        exit 1
fi

mkdir -vp $DIR/nonexist2_deep1 

retval=$?
echo Creating directory nonexist_deep0/nonexist2_deep1 result : $retval

if [ $retval -ne 0 ]; then
        exit 1
fi

if [ -d $DIR/nonexist2_deep1 ]; then
        echo nonexist_deep0/nonexist2_deep1 exists -ok!-
        rm -vrf ${DIR}/nonexist2_deep1/*
        retval=$?
        echo Removing content inside nonexist2_deep1 result : $retval
        if [ $retval -ne 0 ]; then
                exit 1
        fi


        rm -vrf ${DIR}/nonexist2_deep1
        retval=$?
        echo Removing directory nonexist2_deep1 using rm : $retval
        if [ $retval -ne 0 ]; then
                exit 1
        fi

fi

if [ -d $DIR/nonexist2_deep1 ]; then
        echo nonexist2_deep1 exist -error- 
        exit 1
fi

rmdir -v ${DIR}/nonexist_deep1
retval=$?
echo rmdir result for a nonexisting directory, expect 1, result: $retval

if [ $retval -ne 1 ]; then
        exit 1
fi

if [ -d ${DIR}/nonexist_deep1 ]; then
        echo nonexist exist -error-
        exit 1
fi

rm -vrf ${ROOT}/*
retval=$?
echo removing all contents of ${ROOT}, result: $retval

if [ $retval -ne 0 ]; then
        exit 1
fi

if [ -d ${DIR} ]; then
        echo directory not empty -error-
        exit 1
fi

echo All ok!
exit 0

