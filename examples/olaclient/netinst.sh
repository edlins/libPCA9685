#!/bin/sh
# executed by `post-install.txt` as `chroot /rootfs find /tmp/extras -type f -exec sh {} \;`
# executed by user as `sudo ./netinst.sh`

# olaclient depends on libPCA9685 so that should be installed first
# for simplicity, this script will not duplicate what was already done

echo ""
echo "=== Adding olaclient ==="

# install libola-dev
echo ""
echo "= Adding libola-dev"
/usr/bin/apt-get -y --no-install-recommends install libola-dev

# add olad to i2c group
echo ""
echo "= Adding user olad to group i2c"
adduser --debug olad i2c

# build and install olaclient
cd /usr/local/src/libPCA9685
echo ""
echo "= Building olaclient"
mkdir build
cd build
cmake ..
make olaclient
echo ""
echo "= Installing olaclient"
cd examples/olaclient
make install

# configure olad and plugins...
# have to maintain copies of the plugins because olad doesn't build them until first run
echo ""
echo "= Configuring olad and plugins"
cp -v /usr/local/src/libPCA9685/examples/olaclient/olad/conf/* /etc/ola
chown olad.olad /etc/ola/*
grep 'enabled' /etc/ola/*

# configure dhclient
echo ""
echo "= Configuring dhclient"
cp -v /usr/local/src/libPCA9685/examples/olaclient/olad/dhclient-exit-hooks /etc/dhcp/dhclient-exit-hooks
