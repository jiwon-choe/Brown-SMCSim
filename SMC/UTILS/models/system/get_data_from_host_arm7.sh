echo "get-script for ARMv7"
cd /
rm -rf /work/
mkdir -p /work/
cd /tmp/
mkdir mt
echo "Creating node for /dev/sdb1"
mknod /dev/sdb1 b 8 17
echo "Mounting /dev/sdb1"
mount /dev/sdb1 ./mt
echo "Move all files to /work/"
mv ./mt/* /work/
sync
umount ./mt
rm -rf ./mt
echo "Done!"
