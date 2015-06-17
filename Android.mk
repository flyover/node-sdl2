LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := node-sdl2
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(subst \,/,$(shell cd $(LOCAL_PATH) && node -e "require('nan')"))
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_SRC_FILES := node-sdl2.cc
LOCAL_STATIC_LIBRARIES := node
LOCAL_SHARED_LIBRARIES := SDL2
include $(BUILD_STATIC_LIBRARY)

