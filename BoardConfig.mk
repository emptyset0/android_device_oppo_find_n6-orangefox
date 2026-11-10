# SPDX-License-Identifier: Apache-2.0
# Initial PLP110 A.36 source-built recovery. Not device tested.
DEVICE_PATH := device/oppo/plp110
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_VARIANT := generic
TARGET_NO_BOOTLOADER := true
TARGET_BOOTLOADER_BOARD_NAME := canoe
TARGET_BOARD_PLATFORM := canoe
QCOM_BOARD_PLATFORMS += canoe
TARGET_OTA_ASSERT_DEVICE := PLP120,PLP110,CPH2765,OP61A9L1,OP6271L1,plp110,plp120
BUILD_BROKEN_DUP_RULES := true
BUILD_BROKEN_ELF_PREBUILT_PRODUCT_COPY_FILES := true
# Keep OEM layout: kernel is supplied by boot_a, not by recovery_a.
TARGET_PREBUILT_KERNEL := $(DEVICE_PATH)/prebuilt/kernel
BOARD_KERNEL_IMAGE_NAME := Image
BOARD_BOOT_HEADER_VERSION := 4
BOARD_KERNEL_PAGESIZE := 4096
BOARD_MKBOOTIMG_ARGS += --header_version 4 --pagesize 4096
BOARD_EXCLUDE_KERNEL_FROM_RECOVERY_IMAGE := true
BOARD_RAMDISK_USE_LZ4 := true
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 104857600
BOARD_FLASH_BLOCK_SIZE := 4096
# Do not generate or replace boot, vendor_boot, dtbo, vbmeta, or super.
BOARD_AVB_ENABLE := true
AB_OTA_UPDATER := true
AB_OTA_PARTITIONS := \
    abl aop aop_config bluetooth boot cpucp cpucp_dtb dcp devcfg dsp dtbo \
    engineering_cdt featenabler hyp hyp_ac_config imagefv init_boot keymaster \
    modem multiimgoem multiimgqti my_bigball my_carrier my_company \
    my_engineering my_heytap my_manifest my_preload my_product my_region \
    my_stock odm oplus_sec pdp pdp_cdb product pvmfw qupfw recovery \
    secretkeeper shrm soccp soccp_dcd soccp_debug splash spuservice system \
    system_dlkm system_ext tme_config tme_fw tme_seq_patch tz tz_ac_config \
    tz_qti_config uefi uefisecapp vbmeta vbmeta_system vbmeta_vendor vendor \
    vendor_boot vendor_dlkm xbl xbl_ac_config xbl_config xbl_ramdump
BOARD_SUPER_PARTITION_SIZE := 20233322496
BOARD_SUPER_PARTITION_GROUPS := qti_dynamic_partitions
BOARD_QTI_DYNAMIC_PARTITIONS_SIZE := 20229128192
BOARD_QTI_DYNAMIC_PARTITIONS_PARTITION_LIST := system system_ext vendor product my_product odm my_engineering vendor_dlkm system_dlkm system_dlkm_oki my_stock my_heytap my_carrier my_region my_company my_preload my_bigball my_manifest
TARGET_COPY_OUT_VENDOR := vendor
TARGET_COPY_OUT_ODM := odm
BOARD_ODMIMAGE_FILE_SYSTEM_TYPE := erofs
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true
TARGET_USES_MKE2FS := true
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/recovery.fstab
TARGET_RECOVERY_PIXEL_FORMAT := RGBX_8888
TW_THEME := portrait_hdpi
TW_DEVICE_VERSION := Find-N6
# X9U native crypto/storage options; N6 HAL integration required before testing.
BOARD_USES_METADATA_PARTITION := true
BOARD_USES_QCOM_FBE_DECRYPTION := true
TW_INCLUDE_CRYPTO := true
TW_INCLUDE_CRYPTO_FBE := true
TW_USE_FSCRYPT_POLICY := 2
TW_EXCLUDE_MTP :=
TW_NO_HAPTICS := false
TW_SUPPORT_INPUT_AIDL_HAPTICS := true
OF_FL_PATH1 := /sys/class/leds/yellow:flash-0
TW_NO_NETWORK := true
TW_NO_SCREEN_BLANK := true
# OEM AC306/AC140 panel overlays: max 4094, conservative default 925.
# Panel selection/folding and the sysfs node remain untested on device.
TW_BRIGHTNESS_PATH := /sys/class/backlight/panel1-backlight/brightness
TW_MAX_BRIGHTNESS := 4094
TW_DEFAULT_BRIGHTNESS := 925
TW_INCLUDE_FASTBOOTD := true
TW_ENABLE_ALL_PARTITION_TOOLS := false
TW_USE_DMCTL := true
TW_INCLUDE_REPACKTOOLS := false
TARGET_USES_LOGD := true
TWRP_INCLUDE_LOGCAT := true
PLATFORM_VERSION := 16
PLATFORM_VERSION_LAST_STABLE := 16
PLATFORM_SECURITY_PATCH := 2026-03-01
VENDOR_SECURITY_PATCH := 2026-03-01
BOOT_SECURITY_PATCH := 2026-03-01

# Export the selected TWRP/OrangeFox options to Soong-built components.
include vendor/twrp/config/BoardConfigTWRP.mk

TW_SKIP_ADDITIONAL_FSTAB := true

# Explicit executable modes inside the isolated touch root.
TARGET_FS_CONFIG_GEN += $(DEVICE_PATH)/config.fs

# libminuitwrp is a system-variant Soong library. Explicitly install the new
# output into the Recovery ramdisk; otherwise incremental images retain a stale
# copy. This existing OrangeFox relink mechanism also adds its module dependency.
# Installed by the explicit plp110-minuitwrp-recovery module (Android.mk),
# whose actual file dependency survives unchanged ABI/TOC incremental builds.

# Format Data follows the X9U native OrangeFox path.
OF_FORCE_DATA_FORMAT_F2FS := 1
OF_UNBIND_SDCARD_F2FS := 1
OF_WIPE_METADATA_AFTER_DATAFORMAT := 1
OF_DISPLAY_FORMAT_FILESYSTEMS_DEBUG_INFO := 1
OF_DYNAMIC_FULL_SIZE := 20233322496
