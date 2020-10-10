LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

CORE_DIR := $(LOCAL_PATH)/..
EMU      := $(CORE_DIR)/minivmac/src
CFG      := $(CORE_DIR)/minivmac/cfg

LOCAL_MODULE    := retro
LOCAL_CFLAGS    :=

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -DANDROID_ARM
endif

ifeq ($(TARGET_ARCH),x86)
LOCAL_CFLAGS +=  -DANDROID_X86
endif

ifeq ($(TARGET_ARCH),mips)
LOCAL_CFLAGS += -DANDROID_MIPS -D__mips__ -D__MIPSEL__
endif

include $(CORE_DIR)/Makefile.common

BUILD_APP =  $(CORE_SRCS) 

LOCAL_SRC_FILES    := $(SOURCES_CXX) $(SOURCES_C)

LOCAL_CFLAGS +=  $(INCFLAGS) \
					  -DMAC2=1 \
					  -DAND \
					  -std=gnu99 \
					  -O3 \
					  -finline-functions \
					  -funroll-loops \
					  -fsigned-char  \
					  -Wno-strict-prototypes \
					  -ffast-math \
					  -fomit-frame-pointer \
					  -fno-strength-reduce \
					  -fno-builtin \
					  -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565 \
					  -finline-functions -s \


LOCAL_LDLIBS    := -shared -Wl,--version-script=../libretro/link.T 

include $(BUILD_SHARED_LIBRARY)
