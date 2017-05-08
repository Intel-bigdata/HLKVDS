#!/bin/bash

if [ `lsblk |grep loop2|wc -l` -eq 0 ]
then 
	dd if=/dev/zero of=test.img bs=1M count=500
	losetup /dev/loop2 test.img
fi 

#echo "start test ..."
for i in `find test/ -maxdepth 1 -type f -executable -exec file -i '{}' \;| grep binary|grep -v sh|grep -v "\.o"|awk -F":" '{print $1}'`
do  
	#echo "================================"
	echo "start test  "$i
	./$i --gtest_filter=-test_rmd.IntKeyTest  --gtest_output="xml:$i.xml"
	echo "done. =========================="
done 
echo "done."
