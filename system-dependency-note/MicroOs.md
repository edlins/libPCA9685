# Some changes are needed to the system

i2c-dev must be loaded on boot


    echo "i2c-dev" > /etc/modules-load.d/i2c-dev.conf
    echo "i2c-bcm2708" > /etc/modules-load.d/i2c-bcm2708.conf

