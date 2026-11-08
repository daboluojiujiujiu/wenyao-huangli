# 把它装到自己的设备上

准备 M5StickS3、可传输数据的 USB-C 线和 Arduino CLI。刷写会替换原固件，请先备份需要保留的内容。本项目不要求烧写 eFuse。

在仓库根目录执行。开发板使用 M5Stack 的包，不要选成普通 ESP32 或 StickC Plus。

```sh
index=https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
arduino-cli core update-index --additional-urls "$index"
arduino-cli core install m5stack:esp32@3.3.7 --additional-urls "$index"
mkdir -p .deps
git clone https://github.com/m5stack/M5GFX .deps/M5GFX
git -C .deps/M5GFX checkout d91077b9a607b59404e4e4a49f775c792bfae382
git clone https://github.com/m5stack/M5Unified .deps/M5Unified
git -C .deps/M5Unified checkout 8530f5377d782e4a25a6c482de2e71c3f75ca8eb
arduino-cli compile --fqbn m5stack:esp32:m5stack_sticks3 --library .deps/M5GFX --library .deps/M5Unified --output-dir build firmware/wenyao_huangli_hotfix
arduino-cli board list
```

核对端口确实属于目标设备后，将下面的 PORT 换成实际端口：

```sh
arduino-cli upload -p PORT --fqbn m5stack:esp32:m5stack_sticks3 --input-dir build firmware/wenyao_huangli_hotfix
```

进入黄历的日期功能，选择手机校时，连接热点 **HuangLi**，打开 **192.168.8.8** 并同步。关机重开后请核对日期。
