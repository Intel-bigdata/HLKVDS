#!/bin/bash

if [ `lsblk |grep loop2|wc -l` -eq 0 ]
then 
	dd if=/dev/zero of=test.img bs=1M count=500
	losetup /dev/loop2 test.img
fi 

#echo "start test ..."
for i in `find ./ -maxdepth 1 -type f -executable -exec file -i '{}' \;| grep binary|grep -v sh|grep -v "\.o"|awk -F":" '{print $1}'`
do  
	echo "================================"
	echo "start test  "$i
	./$i --gtest_output="xml:$i.xml"
	echo "done. =========================="
done 

losetup -d /dev/loop2
echo "done."
