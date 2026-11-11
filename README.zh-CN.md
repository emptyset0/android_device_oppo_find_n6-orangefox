# OPPO Find N6 OrangeFox Recovery

这是 OPPO Find N6（设备代号 `swangoose`）的非官方 OrangeFox R12 移植，参考 Find X9 Ultra 的 OrangeFox 和原生流程。实测型号为 **PLP110**，系统为 CPH2765 APC A.40，使用兼容的 Canoe 启动链。其他 N6 型号只做了家族标识适配，不代表已验收。

**[直接下载可刷镜像](https://github.com/emptyset0/android_device_oppo_find_n6-orangefox/releases/download/v0.1.0/orangefox_r12.0_oppo_find_n6_v0.1.0_20261110.img)** · [sha256sums.txt](https://github.com/emptyset0/android_device_oppo_find_n6-orangefox/releases/download/v0.1.0/sha256sums.txt) · [发布说明](https://github.com/emptyset0/android_device_oppo_find_n6-orangefox/releases/tag/v0.1.0)。GitHub 自动生成的 `Source code` 压缩包不含可刷镜像。镜像只用于目标 Recovery 槽，不是解锁包、转区包，也不改 GPT、ABL 或 EFISP。只有在你已确认有独立恢复路径时才刷入。

已实测外屏、触摸、原生 PIN 解密、读写 MTP、OTG（NTFS/exFAT）、FastbootD、手电筒和振动；原生备份／恢复用专用测试目标验证过，同版本 APC OTA 通过，未验证跨版本 OTA。内屏默认关闭。带线重启有可能回落到 USB 2.0，Android 中也已复现。

此版修复待机锁屏 CPU 温度，并隐藏无内置安装包的 Magisk 选项。实际安装了 Magisk 时仍可管理其模块；`magiskboot` 和「刷入当前 OrangeFox」保留。

KernelSU 不占用 Recovery ramdisk：内置所需资产超出预算。仓库提供已在 APC A.40 文件上验证的 [离线 LKM 修补方法](docs/KERNELSU.md)，不会向手机写入。当前假回锁／green 链路不能根据 Recovery 的启动结果推断可接受修改过的 `init_boot`。

[构建说明](docs/BUILD.md) · [English](README.md)
