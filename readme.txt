这个工程用的是 Telink TC32（TLSR8258）的 make 构建系统，所以编译命令就是 make；Windows 下仓库自带了一个 GNU Make 可执行文件，直接用它即可。

Windows（当前环境）

bat
cd C:\Users\tyh21\Desktop\blozi-etag\Firmware
.\makeit.exe


Linux / WSL / macOS

bash
cd Firmware
make
（makefile 里会自动判断平台：Windows 用 tc32_windows\bin\，Linux 用 tc32_linux/bin/）

产物

Firmware\ATC_Paper.bin —— 最终固件（编译时自动调 tl_firmware_tools.py add_crc 加了 CRC）
Firmware\out\ATC_Paper.elf / ATC_Paper.lst / 各个 .o，以及 out\ATC_Paper_crc.hex
关键编译参数（Firmware\makefile）

芯片：-DCHIP_TYPE=CHIP_TYPE_8258
编译器：tc32-elf-gcc（-O2 -std=gnu99 -ffunction-sections -fdata-sections ...）
链接脚本：static_src\boot.link，库 -llt_8258
其它常用目标

.\makeit.exe clean　清编译产物
.\makeit.exe hex　额外生成 out\ATC_Paper_crc.hex
.\makeit.exe combine_fw　合并 boot 生成 out\ATC_Paper_with_boot.bin
.\makeit.exe flash　调用 Telink_Tools.py 烧录（需指定 DOWNLOAD_PORT）