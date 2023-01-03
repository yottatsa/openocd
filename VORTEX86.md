# Vortex86

How to run and test

    ./bootstrap
    ./configure --enable-dummy --enable-ftdi --enable-usb-blaster --enable-verbose
    make
    ./src/openocd -f tcl/interface/dummy.cfg -f tcl/cpu/vortex86.cfg
