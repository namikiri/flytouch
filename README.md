# FlyPOS Pro Serial TouchScreen driver
This is an approach to make a Linux driver for FlyPOS Pro's touch screen driver. This poor point-of-sale machine lacks the support in general and has none of it for Linux but it was too well engineered to be lost in times without any support. It's quite old but with Linux on it you can give it a second life.

## Build
Simple as hell:

```
$ cd flytouch
$ make
```

## Install
After you build the driver simply install it with `make install` as root user:
```
# make install
```

It will install the driver into system and do the neccessary things related to drivers in Linux.

## Usage
You'll also need a patched `inputattach` from LinuxConsole Tools (I've done it in [linuxconsole](https://github.com/namikiri/linuxconsole) repo). Just clone it and `make` & install it like you'd do it with the original tool.

After building the tool run:

```
# inputattach -fly /dev/ttyS4
```

I believe that all FlyPOS Pro devices have their touch screens on `/dev/ttyS4` but it can be some other port. While the tool is running try touching the screen: the cursor should move somewhere near your finger. The position of your touches likely will be inaccurate, use `xinput_calibrator` to make them more precise.  

If it works, you can run `inputattach` as a daemon:

```
# inputattach --daemon -fly /dev/ttyS4
```

## Uninstall
To uninstall the driver, run as root while being in the project's directory:
```
make uninstall
```

## Known issues
As this driver isn't in the Linux kernel mainstream, it doesn't have a unique SERIO ID, so I've chose the `0xe4`. It far future it might conflict with some ID from another device. I'm sure that Torvalds or whoever approves the patches wouldn't accept mine so if it happens, find the SERIO_FLYTOUCH macro definition on the top of `flytouch.c` file and change it to some unused value. Don't forget to do the same in [serio-ids.h](https://github.com/namikiri/linuxconsole/blob/master/utils/serio-ids.h) from `linuxconsole` repo.

## TODO
- add this driver to the mainstream
