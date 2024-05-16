#!/bin/sh

if [ -z $1 ]
then
	return 1
fi
writefile=$1

if [ -z $2 ]
then
	return 1
fi
writestr=$2

if [ ! -d $writefile ]
then
	directory=$(dirname "$writefile")
	mkdir -p $directory
fi

echo $writestr > $writefile
