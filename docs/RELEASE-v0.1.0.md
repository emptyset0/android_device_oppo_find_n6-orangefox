Initial public release of this unofficial Find N6 port.

## Download

- **[orangefox_r12.0_oppo_find_n6_v0.1.0_20261110.img — flashable 100 MiB image](https://github.com/emptyset0/android_device_oppo_find_n6-orangefox/releases/download/v0.1.0/orangefox_r12.0_oppo_find_n6_v0.1.0_20261110.img)**
- [sha256sums.txt](https://github.com/emptyset0/android_device_oppo_find_n6-orangefox/releases/download/v0.1.0/sha256sums.txt)

The automatic **Source code** ZIP and tar.gz are not flashable and do not contain the Recovery image.

### Features

- Outer display and touch, native FBE and read/write MTP, OTG, ADB/FastbootD, backup/restore and the existing A/B OTA flow.
- Balanced CPU frequency at the PIN prompt, automatic dim/timeout and wake, and a working lock-screen CPU temperature display.
- No unavailable Magisk add-on choices. Retains `magiskboot`, Flash current OrangeFox, ordinary ZIP installation and the native manager for installed Magisk modules. KernelSU is not bundled.

### Tested setup

PLP110 on CPH2765 APC A.40 with a compatible Canoe boot chain. The A-slot flash matched a full readback; Android and Recovery cold booted. The real lock screen and add-on pages were checked. Other regional N6 variants and cross-version OTA have not been separately validated. This image preserves the OEM AVB envelope; it is not a bootloader unlock or conversion package.
