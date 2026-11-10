#!/bin/bash
export FOX_BUILD_DEVICE=plp110
export FOX_AB_DEVICE=1
export FOX_VIRTUAL_AB_DEVICE=1
export OF_DISABLE_MIUI_SPECIFIC_FEATURES=1
export FOX_VANILLA_BUILD=1
export OF_FORCE_PREBUILT_KERNEL=1
# No automatic flash, slot switch, data format, or AVB patching.

export OF_AB_DEVICE_WITH_RECOVERY_PARTITION=1
