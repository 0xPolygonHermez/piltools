# Pil Tools
Pil Tools in C++ to verify and browse in evaluations

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
# Build
```sh
sudo apt update
sudo apt install build-essential
sudo apt install libomp-dev
sudo apt install nlohmann-json3-dev
sudo apt install libgmp-dev
sudo apt install libreadline-dev

git clone https://github.com/ithewei/libhv.git
cd libhv
ls
./configure
make
sudo make install

```
