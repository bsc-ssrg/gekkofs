#!/bin/bash
ROOT=${1}

echo Creating a file from a echo with content foo
echo "foo" > $ROOT/a.out

if [ -f $ROOT/a.out ]; 
     then
     echo File exists
     echo is the content correct?
     if grep -Fxq "foo" $ROOT/a.out
     then
            echo "String found"
     else
            echo "String not found"
            exit 1
     fi
     else
        echo File does not exists
        exit 1
fi

exit 0