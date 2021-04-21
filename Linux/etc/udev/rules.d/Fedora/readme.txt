From OH1KH:

Add the following lines to /etc/udev/rules.d/92-persistent-usb.rules:

# Radioddity
SUBSYSTEM=="usb", ATTRS{idVendor}=="15a2", ATTRS{idProduct}=="0073", MODE="666"
ACTION=="add", SUBSYSTEM=="tty", ATTRS{idVendor}=="1fc9", ATTRS{idProduct}=="0094", SYMLINK+="OpenGD77"
