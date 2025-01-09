# VGMdata
Extract sample data from vgm file.
```
$ cmake -B build -S .
```

# Running vgmdata compiled to webassembly
To execute vgmdata in the browser, you need a http server. Just execute this inside the `build/web` directory:
```
python -m http.server 8080
```
and point the browser to `localhost:8080` and click on the **vgmdata.html** file.
