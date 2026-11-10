# SPDX-License-Identifier: Apache-2.0
DEVICE_PATH := device/oppo/plp110
$(call inherit-product, $(SRC_TARGET_DIR)/product/base.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, vendor/twrp/config/common.mk)
PRODUCT_SHIPPING_API_LEVEL := 36
BOARD_SHIPPING_API_LEVEL := 202504
PRODUCT_USE_DYNAMIC_PARTITIONS := true
# Recovery must understand the existing super, not generate a replacement.
PRODUCT_BUILD_SUPER_PARTITION := false
PRODUCT_BUILD_SUPER_EMPTY_IMAGE := false
PRODUCT_CHECK_PREBUILT_MAX_PAGE_SIZE := false
PRODUCT_SOONG_NAMESPACES += $(DEVICE_PATH)
PRODUCT_COPY_FILES += $(call find-copy-subdir-files,*,$(DEVICE_PATH)/recovery/root,recovery/root)

# Local, isolated touch bridge; no sensor HAL or DSP services.
PRODUCT_PACKAGES += plp110-hbp5

PRODUCT_PACKAGES += plp110-minuitwrp-recovery

# Match N6 Virtual A/B capability properties.
PRODUCT_SYSTEM_PROPERTIES += \
    ro.virtual_ab.enabled=true \
    ro.virtual_ab.compression.enabled=true \
    ro.virtual_ab.userspace.snapshots.enabled=true \
    ro.virtual_ab.compression.xor.enabled=true
