
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES+= \
	main.c rc522.c rfid.c spidev_nfc.c  
LOCAL_C_INCLUDES += \
    external/jhead \
    external/tremor/Tremor \
    frameworks/base/core/jni \
    $(PV_INCLUDES) \
    $(JNI_H_INCLUDE) \
    $(call include-path-for, corecg graphics)

LOCAL_SHARED_LIBRARIES := \
		 libcutils libutils libgui liblog 
LOCAL_CFLAGS += -DWRITE

LOCAL_MODULE := nfc_test
LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))

