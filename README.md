# VGM-data-xtractor
Extract data blocks from vgm file.
```
$ cmake -B build -S .
```

# Running vgmdata compiled to webassembly
To execute vgmdata in the browser, you need a http server. Just execute this inside the `docs` directory:
```
python -m http.server 8080
```
and point the browser to `localhost:8080` or you can access the latest version at: https://pvmm.github.io/vgm-data-xtractor/
