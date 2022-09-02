# Setup/Development

Clone the repo

```bash
git submodule init
git submodule update
make

make run # will open a Xephyr Window
```

## Debug

If the program is executed via `make run` its possible to attach to the executable
via the *debug.sh* script. It has to be executed as root user.
The script used cgdb as a debugger.

```bash
sudo ./debug.sh
```


