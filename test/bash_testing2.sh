#!/bin/bash
ROOT=${1}

echo Creating a file from a echo with content foo
echo "foo" > $ROOT/a.out

echo "Testing if file exists"

if [ -f $ROOT/a.out ]; then
     echo "File exists"
     echo "Testing if the content is correct"
     if grep -Fxq "foo" $ROOT/a.out
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