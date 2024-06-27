#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
MAKEFLAGS="-j4"

OUTPUTDIR="${1:-"/tmp/aeld"}"
CROSS_COMPILE_BINDIR=$(dirname $(which ${CROSS_COMPILE}readelf))
CROSS_COMPILE_BASEDIR=${CROSS_COMPILE_BINDIR%\/*}
PWD=$(pwd)

[ ${PWD##*\/} == "finder-app" ] && SCRIPTDIR=$(dirname $PWD) || SCRIPTDIR=$PWD

[ ! -d $OUTPUTDIR ] && mkdir -p ${OUTPUTDIR} || { echo "Unable to create ${OUTPUTDIR} directory..exiting.."; exit 1; }

#install missing pre-requisites
sudo apt update  &>/dev/null
declare -a package_list=("ruby cmake git build-essential bsdmainutils sudo wget bc u-boot-tools kmod cpio flex bison libssl-dev psmisc")
for pkg in ${package_list[@]}
do
     dpkg -s "$pkg" &>/dev/null && echo "Packged $pkg is installed" || { echo "installing $pkg..";DEBIAN_FRONTEND="noninteractive" TZ="America/New_york" sudo apt install -y --no-install-recommends $pkg &>/dev/null; }
done
sudo apt install -y qemu-system-arm &>/dev/null

cd "$OUTPUTDIR"
if [ ! -d "${OUTPUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTPUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}  1>/dev/null
fi
if [ ! -e ${OUTPUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION} &>/dev/null

    #clean any old artifacts
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper 1>/dev/null

    #defconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig 1> /dev/null
    
    # Kernel,module and device tree builds
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all 1> /dev/null
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules 1> /dev/null
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs 1> /dev/null
fi

echo "Adding the Image in outdir"
cp arch/${ARCH}/boot/Image ${OUTPUTDIR}/.

echo "Creating the staging directory for the root filesystem"
cd "$OUTPUTDIR"
if [ -d "${OUTPUTDIR}/rootfs" ]
then
    echo "Deleting rootfs directory at ${OUTPUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTPUTDIR}/rootfs
fi

mkdir -p "${OUTPUTDIR}/rootfs"
cd $OUTPUTDIR/rootfs
mkdir -p {bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr,var}
mkdir -p usr/{sbin,bin,lib}
mkdir -p var/log

cd "$OUTPUTDIR"
if [ ! -d "${OUTPUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
else
    cd busybox
fi

make distclean && make defconfig 
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX="${OUTPUTDIR}/rootfs" 1> /dev/null
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX="${OUTPUTDIR}/rootfs" install 1> /dev/null

echo "Library dependencies"
while read -r line;do
    libname=$(echo "$line"| cut -d':' -f2 |  cut -d '[' -f2 | cut -d']' -f1 | cut -d' ' -f2)
    libname=${libname##*\/}
    file=$(find $CROSS_COMPILE_BASEDIR -name $libname)
    libdir=${file%\/*}
    libdir=${libdir##*\/}
    cp $file ${OUTPUTDIR}/rootfs/${libdir}/.
done < <(${CROSS_COMPILE}readelf -a ${OUTPUTDIR}/rootfs/bin/busybox | grep "program interpreter")
while read -r line;do
    libname=$(echo "$line"| cut -d':' -f2 |  cut -d '[' -f2 | cut -d']' -f1 | cut -d' ' -f2)
    libname=${libname##*\/}
    file=$(find $CROSS_COMPILE_BASEDIR -name $libname)
    libdir=${file%\/*}
    libdir=${libdir##*\/}
    cp $file ${OUTPUTDIR}/rootfs/${libdir}/.
done < <(${CROSS_COMPILE}readelf -a ${OUTPUTDIR}/rootfs/bin/busybox | grep "Shared library")

sudo mknod -m 666 "${OUTPUTDIR}/rootfs"/dev/console c 5 1
sudo mknod -m 666 "${OUTPUTDIR}/rootfs"/dev/null c 1 3

make -C $SCRIPTDIR/finder-app clean
make -C $SCRIPTDIR/finder-app CROSS_COMPILE=${CROSS_COMPILE}


# on the target rootfs
mkdir -p ${OUTPUTDIR}/rootfs/home/conf
cp $SCRIPTDIR/finder-app/finder.sh $SCRIPTDIR/finder-app/autorun-qemu.sh $SCRIPTDIR/finder-app/finder-test.sh $SCRIPTDIR/finder-app/writer $SCRIPTDIR/finder-app/writer.sh ${OUTPUTDIR}/rootfs/home
cp $SCRIPTDIR/conf/username.txt $SCRIPTDIR/conf/assignment.txt ${OUTPUTDIR}/rootfs/home/conf
sed -i -e 's/cat ..\//cat /'  ${OUTPUTDIR}/rootfs/home/finder-test.sh

cd "$OUTPUTDIR"/rootfs 
find . | cpio -H newc -ov --owner root:root > ${OUTPUTDIR}/initramfs.cpio
gzip -f ${OUTPUTDIR}/initramfs.cpio
