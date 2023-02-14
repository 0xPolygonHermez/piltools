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
# Docker version
All commands are run from the root piltools repo folder.

## Build
```sh
make docker
```

## Sample usage
```sh
docker run -it --rm \
    -v $PWD/sample:/app/sample \
    -v $PWD/config:/app/config \
    -p 8080:8080 \
    piltools \
    pilserver sample/fibonacci.commit -c sample/fibonacci.const -p sample/fibonacci_main.pil.json -u sample/fibonacci.input.json -C config/server.conf
```

```sh
docker run -it --rm \
    -v $PWD/sample:/app/sample \
    -v $PWD/config:/app/config \
    -p 8080:8080 \
    piltools \
    pilverify sample/fibonacci.commit -c sample/fibonacci.const -p sample/fibonacci_main.pil.json -u sample/fibonacci.input.json
```
It's possible add flag -s \<filename> to **save** evaluation on a file, or flag -l \<filename\> to load evaluation from file.
```sh
pilverify build/proof/zkevm.commit -c build/proof/zkevm.const -p build/proof/pil/main.pil.json -s build/proof/zkevm.expr
```
## License
Licensed under either of

Apache License, Version 2.0, (LICENSE-APACHE or http://www.apache.org/licenses/LICENSE-2.0)
MIT license (LICENSE-MIT or http://opensource.org/licenses/MIT) at your option.
Contribution
Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in the work by you, as defined in the Apache-2.0 license, shall be dual licensed as above, without any additional terms or conditions.

### Disclaimer
This code has not yet been audited, and should not be used in any production systems.
