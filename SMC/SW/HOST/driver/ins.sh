insmod pim.ko

LAST_PIM_DEVICE=`expr $1 - 1`


for i in `seq 0 $LAST_PIM_DEVICE`
do
    mknod /dev/PIM$i c 250 $i
done
