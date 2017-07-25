ifneq ($(filter librecovery_updater_nexell,$(TARGET_RECOVERY_UPDATER_LIBS)),)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := bootable/recovery
LOCAL_SRC_FILES := recovery.cpp
LOCAL_MODULE := librecovery_updater_nexell
include $(BUILD_STATIC_LIBRARY)
endif
