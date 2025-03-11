#!/bin/sh
writefile=$1
writestr=$2
if [ $# -ne 2 ]
then
	if [ $# -eq 1 ]
	then
		echo "You have missed the 2nd parameter" 
	elif [ $# -eq 0 ]
	then
		echo "You have missed 1st and 2nd parameters"
	else 
		echo "Too many paramaters"
	fi  
	
	exit 1
fi

dirname=$(dirname "$writefile")
mkdir $dirname

if [ $? -eq 1 ]
then
    mkdir $(dirname $dirname)
    mkdir $dirname
fi

echo "$writestr" >> "$writefile"

if [ $? -eq 1 ]
then 
    echo "Error creating the file"
fi
