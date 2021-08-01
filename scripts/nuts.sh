#!/bin/bash

if [ $EUID -ne 0 ]
then
    echo -e "$(tput setaf 1)[ERROR]$(tput sgr0) This script must be run as root!" 1>&2
    exit 1
fi

if [ "$#" -lt 2 ]
then
    echo "$(tput setaf 1)[ERROR]$(tput sgr0) Not enough arguments."
    echo "Usage: $0 <filename> <mount point> [filename size (ex. 1GB, 500MB)]"
    exit 1
fi

FILENAME="$1"
MNTPOINT="$2"
SIZE="$3"

fallocate -l "$SIZE" "$FILENAME"
sudo mkfs.ext3 "$FILENAME"

if lsmod | grep loop &> /dev/null
then
    sudo modprobe loop
fi

mkdir -p "$MNTPOINT"
sudo mount -o loop "$FILENAME" "$MNTPOINT"

