# OrangeFox Recovery for OPPO Find N6

Unofficial OrangeFox R12 port for OPPO Find N6. The device tree, source patches and packaging tools are here.

**[Download the flashable image from Releases](https://github.com/emptyset0/android_device_oppo_find_n6-orangefox/releases)**. Use the checksum and release notes attached to the same version as the image.

GitHub's automatic **Source code** archives do not contain the flashable image.

**Device codename:** `swangoose` (OPPO Find N6). **Tested variant:** PLP110, running CPH2765 APC A.40 with a compatible Canoe boot chain. The tree retains the stock OTA identifiers `PLP110`, `PLP120`, `CPH2765`, `OP61A9L1` and `OP6271L1`; that is not a claim of hardware testing on every regional model. This is a Recovery image, not a conversion ROM or a bootloader unlock package.

## Working and validation status

| Feature | Status |
| --- | --- |
| Cover display and touch | Tested; inner panel is intentionally off by default. |
| Data decryption and MTP | Native Fox/AOSP metadata and FBE; PIN and read/write MTP tested on PLP110. |
| ADB and FastbootD | Tested; the bootloader Fastboot is a separate Canoe/ABL feature. |
| USB OTG, exFAT and NTFS | Both filesystem formats read on real OTG media. |
| Backup and restore | Native paths tested with disposable targets; not a full user-data restore test. |
| ZIP/OTA | Same-version APC A/B update tested. Deferred activation restores the other slot's ABL before activation; cross-version OTA is unverified. |
| Vibration, flashlight, display idle | Tested. PIN idle uses the balanced governor; timeout and touch wake verified. |
| KernelSU | Separate offline LKM image preparation only; not a built-in one-tap installer. See [KernelSU](docs/KERNELSU.md). |

USB can fall back to High-Speed across reboots, including on Android. See the versioned release notes for fixes. Magisk add-on choices without a bundled installer are hidden; the installed Magisk module manager and `magiskboot` remain available.

## Install and recover

Use the 100 MiB `.img` attached to the chosen release, not the raw build output. The gzip ramdisk fits the N6 limit and preserves the original recovery AVB envelope; it is not newly OEM-signed. A stock locked bootloader is not an installation path. Before writing anything, identify the active slot and prove that your recovery/rollback path is available. Canoe Bootloader Fastboot and Fox FastbootD are **not** interchangeable.

On the tested setup, Canoe could back up, write and fully read back a single `recovery_<slot>` partition. Save its original image and verify its hash. Flash only the intended Recovery slot, read back and compare to `sha256sums.txt`, and verify both normal boot and Recovery. Do not replace `abl`, `efisp`, `persist`, GPT or the other slot as part of this installation. Intermittent Canoe USB enumeration and the absence of independently verified EDL write access mean A/B is not, by itself, a guaranteed rescue path.

## Build

See [Build notes](docs/BUILD.md) for the pinned Fox 16 manifest, source patch application, OEM input, and the separate N6 recovery envelope step.

## Credits

- [OrangeFox Recovery Project](https://gitlab.com/OrangeFox) and Team Win.
- [koaaN's Find X9 Ultra (lighthouse) OrangeFox device tree](https://github.com/koaaN/android_device_oppo_lighthouse-orangefox), the device-port reference.
- [gbl_root_canoe](https://github.com/SuperTurtleDev/gbl_root_canoe) for the compatible Canoe boot path.
- [KernelSU](https://github.com/tiann/KernelSU) for the optional offline LKM workflow.

Upstream source and patches retain their respective licenses. OEM runtime files required by this hardware are not checked into this source repository.
