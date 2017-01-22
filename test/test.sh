#!/bin/bash

echo "start test ..."
for i in `find -type f -executable -exec file -i '{}' \; | grep 'x-executable; charset=binary'`
do 
	echo $i
done 
echo "done."
