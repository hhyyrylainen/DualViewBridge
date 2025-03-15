# DualViewBridge

The bridge that DualView webextension requires to be able to
communicate with the installed DualView version.

Currently only supported on Linux.

## Building

A standard Cmake build after dependencies (c++ 14 compiler) are
installed.

```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

Note that on some distros the location Firefox loads bridge files from
is different so that might need adjusting.

## Config

After building and installing a config file needs to be placed at
`~/.local/share/dualview/bridge.json` following this template:

```json
{
    "workDir": "/home/USERNAME/bin/",
    "dualviewExecutable": "/home/USERNAME/bin/dualviewpp",
    "logging": false
}
```
