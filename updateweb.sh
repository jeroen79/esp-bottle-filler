#/bin/bash
cp ./web/dist/index.html ./components/bottle-filler/index.html
gzip -f ./components/bottle-filler/index.html 

cp ./web/dist/manifest.json ./components/bottle-filler/manifest.json