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
