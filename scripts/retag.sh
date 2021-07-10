#!/bin/bash

git pull
if [ $(git tag | wc -l) -gt 1 ] ;
then
   echo "too many existing tags"
   exit -1
fi

#git tag -fa $(git tag) -m"updated text"
#git push origin --tags -f
