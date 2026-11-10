LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := plp110-hbp5
LOCAL_SRC_FILES := src/plp110_hbp5.c
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/system/bin
LOCAL_CFLAGS := -Wall -Wextra
LOCAL_STATIC_LIBRARIES := libc
# Recovery-only builds otherwise skip these system install targets. mkbootfs
# reads them via -d TARGET_OUT to apply the isolated linker's executable mode.
LOCAL_REQUIRED_MODULES += fs_config_files_system fs_config_dirs_system
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_ETC)/fs_config_files $(TARGET_OUT_ETC)/fs_config_dirs
include $(BUILD_EXECUTABLE)

# A plain copy of the SOURCE-BUILT library, not a vendor prebuilt replacement.
# Upstream relink_libraries' POST_INSTALL hook does not depend on library file
# changes when the ABI/TOC is unchanged. Own this installed path explicitly.
include $(CLEAR_VARS)
LOCAL_MODULE := plp110-minuitwrp-recovery
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_STEM := libminuitwrp.so
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/system/lib64
LOCAL_PREBUILT_MODULE_FILE := $(TARGET_OUT_SHARED_LIBRARIES)/libminuitwrp.so
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_SHARED_LIBRARIES)/libminuitwrp.so
LOCAL_REQUIRED_MODULES := libminuitwrp
include $(BUILD_PREBUILT)
