#!/bin/bash
set -x

ROOT=${1}

echo "Creating a file from with content foo" in ${ROOT}

echo "foo" > ${ROOT}/a.out

echo "Testing if file exists"

if [ -f ${ROOT}/a.out ]; then
     echo "File exists"
     echo "Testing if the content is correct"
     if grep -Fxq "foo" ${ROOT}/a.out
     then
            echo "String found"
     else
            echo "String not found"
            exit 1
     fi
else
     echo "File does not exists"
     exit 1
fi

echo "all test done"

exit 0