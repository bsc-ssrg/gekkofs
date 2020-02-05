#!/bin/bash
function has_substring() {
   [[ "$1" != "${2/$1/}" ]]
}

ROOT=${1}

echo Creating two large files in tmp

dd if=/dev/urandom of=/tmp/large_file_01 bs=1M count=4

dd if=/dev/urandom of=/tmp/large_file_02 bs=1M count=4

echo Copy files to gekko
cp /tmp/large_file_01 ${ROOT}/

cp /tmp/large_file_02 ${ROOT}/

stat ${ROOT}/large_file_01
retval=$?
if [ $retval -ne 0 ]; then
	echo "File is not copied"
	exit 1
fi
stat ${ROOT}/large_file_02

if [ $retval -ne 0 ]; then
	echo "File is not copied"
	exit 1
fi

echo Concatenating two gekko files
cat ${ROOT}/large_file_01 >> ${ROOT}/large_file_02
retval=$?

if [ $retval -ne 0 ]; then
	echo Something wrong on concatenation
        exit 1
fi


stat ${ROOT}/large_file_02

retval=$?
echo Stat should be 8M

if [ $retval -ne 0 ]; then
        exit 1
fi
stat=`stat ${ROOT}/large_file_02`

if has_substring ${stat} "8388608"
then
	echo All Ok
else
	echo Size not updated
	exit 1
fi
exit 0
