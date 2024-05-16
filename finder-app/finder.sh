#!/bin/sh

if [ -z $1 ]
then
	echo "filesdir is not specified"
	return 1
fi
filesdir=$1

if [ -z $2 ]
then
	echo "searchstr is not specified"
	return 1
fi
searchstr=$2

if [ ! -d $filesdir ]
then
	echo "$filesdir does not exist"
	return 1
fi

total=0
match=0
for entry in "$filesdir"/*
do
	t=$(cat "$entry" | grep $searchstr)
	if [ ! -z $t ]
	then
		match=$((total+1))
	fi
	total=$((total+1))
done
echo "The number of files are $total and the number of matching lines are $match"

