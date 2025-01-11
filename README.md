# VGM-data-xtractor

Extract data blocks from vgm files.

# Compiling

To compile it yourself, you need CMake:
```
cmake -B build -S .
```
This command will create the executable in `vgm-data-xtractor/build/vgm-data-xtractor`

# Running webassembly vgm-data-xtractor

To compile it using webassembly, you use PLATFORM=Web:
```
cmake -B build -S . -DPLATFORM=Web
```
And to execute vgmdata in the browser, you need a http server. Just execute this inside the `docs` directory:
```
python -m http.server 8080
```
and point the browser to `localhost:8080` or you can just access the latest version [here](https://pvmm.github.io/vgm-data-xtractor/)

# Missing features

* data block decompression;
