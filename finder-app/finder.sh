#!/bin/sh
filesdir=$1
searchstr=$2

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

if [ ! -d "$filesdir" ]
then
	echo "Not a directory"
	exit 1
fi

nfiles=$(ls $filesdir | wc -l)
cd "$filesdir"
nmatch=$(grep -r "$searchstr" * | wc -l)

echo "The number of files are ${nfiles} and the number of matching lines are ${nmatch}"
