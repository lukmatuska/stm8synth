# STM8S 项目模板

这是适用于 STM8S 的项目模板。使用 SDCC 作为编译器。

项目模板默认主控：STM8S103F3

项目信息：

- 主控：STM8Sxx
- 烧录器：STLink
- 编译器：SDCC

SDCC 可以通过插件的 "安装实用工具功能" 进行安装。

## 切换芯片型号

1. 修改宏定义：芯片型号由 **宏定义** 决定，因此需要修改默认的宏定义，可用选项如下

    |宏定义|RAM大小|FLASH大小|EEPROM大小|
    |----|----|----|----|
    |STM8S003|1KB|8KB|128B|
    |STM8S001|1KB|8KB|128B|
    |STM8S005|2KB|32KB|128B|
    |STM8S007|6KB|64KB|128B|
    |STM8S103|1KB|4KB~8KB|640B|
    |STM8S105|2KB|16KB~32KB|1KB|
    |STM8S207|6KB|32KB~128KB|1KB~2KB|
    |STM8S208|6KB|32KB~128KB|2KB|
    |STM8S903|1KB|8KB|640B|


2. 添加外设 .c 文件，该工程默认适配 STM8S103F3，因此 项目资源 `Libraries` 目录下仅添加了 STMS103F3 支持的外设 c 文件

    因此当你更换芯片后需要自行添加或者移除相关的外设 .c 文件

