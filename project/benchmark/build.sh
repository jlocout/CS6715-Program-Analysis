
Action=$1

if [ "$Action" == "clean" ]; then
	for item in bench*
	do
		cd $item 
		make clean
		rm -f demo.*
		rm -f *.dot
		rm -rf *.png
		cd -
	done
	
	exit 0
fi

for item in bench*
do
	cd $item && make clean && make && cd -
done