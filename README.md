# Keylogger daemon for Linux-based OS

This package provides all you need to compile, install and execute a Keylogger daemon that runs in the background in Linux-based OS environments. As the name suggests, its purpose is to silently listen for keyboard buttons that are pressed during the normal users' activity. The latter is then saved into a buffer efficiently managed by the daemon which, upon approaching its maximum capacity (or when the deamon is stopped), flushes (append) its content into a file named `keyboard.log` created in the directory `/var/log`.

To run this service you need superuser privileges as they are required to read from `/dev/input/eventX`. You also need such priviledges in order to read, update, move or delete the file `keyboard.log` so as not to allow any unpriviledged user to access sensible information.

We want to point out that the daemon is currently designed to work with the IT keyboard layout and if you are intended to use a differen one (*e.g.*, EN, ES, DE, etc.) you have to make some changes into `src/keylogger.h`.

## Compilation

Before compile source code you must identify which is the number `X` of the input event `eventX` associated with the used keyboard. To do this it is sufficient to launch `cat /proc/bus/input/devices` and find the device name corresponding to the wanted keyboard. Once found the relative entry, `eventX` can be read from the list of Handlers. Also, you are free to chose a `name` for the deamon that differs from the default one if you desire to use a non suspicious string.

Number `X` must then be assigned to the varibale `event` declared at the top of the `Makefile` in order to correctly compile the daemon. Similarly, the daemon `name` can be changed within the `Makefile`. Thus, compilation can be accomplished by launching

```sh
make
```

This will generate a `bin` folder devoted to maintain the executable and the object files. Then you have to run the following command in order to install (requires higher proiviledges) the executable within the `/usr/sbin` directory

```sh
sudo make install
```

On the contrary, to uninstall the executable from `/usr/sbin`, launch

```sh
sudo make uninstall
```

Finally, to clean the content of the binary folder it is sufficient to launch

```sh
make clean
```

The latter will remove the `bin` folder and its content.

## Usage

Once the executable have been installed as described before, starting, stopping and checking the current status of the daemon is a very simple task

```sh
kb [start|stop|status]
```

## Note

The purpose of this project is solely to present a solution for recording the users' activity in a Linux-based system. We decline any responsibility related to malicious use of this service.