# Pil Tools
###Pil Tools in C++
to verify and query

## pilverify
syntax:
```sh
build/pilverify [-m] <commitfile> -c <constfile> -p <pil.json> [-v] [-V <verifyFileByOmegas>] [-j <verifyFileByPols>]
```
## pilserver
syntax:
```sh
build/pilserver [-m] <commitfile> -c <constfile> -p <pil.json> [-l] <exprfile> -C <serverConfig.conf>
```

example of use:
```sh
build/pilserver ../data/v0.3.0.0-rc.1-n21/zkevm.commit -p ../data/v0.3.0.0-rc.1-n21/main.pil.json -c ../data/v0.3.0.0-rc.1-n21/zkevm.const -l ../data/v0.3.0.0-rc.1-n21/zkevm.expr.bin -C config/server.conf
```

## fibonacci sample
syntax:
```sh
build/pilserver sample/fibonacci.commit -c sample/fibonacci.const -p sample/fibonacci_main.pil.json -u sample/fibonacci.input.json -C config/server.conf
```

example of use:
```sh
build/pilverify sample/fibonacci.commit -c sample/fibonacci.const -p sample/fibonacci_main.pil.json -u sample/fibonacci.input.json
```

# Build
```sh
sudo apt update
sudo apt install build-essential
sudo apt install libomp-dev
sudo apt install nlohmann-json3-dev
sudo apt install libgmp-dev
sudo apt install libreadline-dev

# out of folder piltools
git clone https://github.com/ithewei/libhv.git
cd libhv
./configure
make
sudo make install
cd ..

# go to piltools folder
cd piltools
git submodule init
git submodule update
make
```
