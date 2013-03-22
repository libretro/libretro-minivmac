LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

APP_DIR := ../../src

LOCAL_MODULE    := retro

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -DANDROID_ARM
endif

ifeq ($(TARGET_ARCH),x86)
LOCAL_CFLAGS +=  -DANDROID_X86
endif

ifeq ($(TARGET_ARCH),mips)
LOCAL_CFLAGS += -DANDROID_MIPS -D__mips__ -D__MIPSEL__
endif

EMU = ../minivmac/minivmac/src
GLUE = ../minivmac

CORE_SRCS = \
	$(EMU)/MINEM68K.c \
	$(EMU)/GLOBGLUE.c \
	$(EMU)/M68KITAB.c \
	$(EMU)/VIAEMDEV.c \
	$(EMU)/VIA2EMDV.c \
	$(EMU)/IWMEMDEV.c \
	$(EMU)/SCCEMDEV.c \
	$(EMU)/RTCEMDEV.c \
	$(EMU)/ROMEMDEV.c \
	$(EMU)/SCSIEMDV.c \
	$(EMU)/SONYEMDV.c \
	$(EMU)/SCRNEMDV.c \
	$(EMU)/VIDEMDEV.c \
	$(EMU)/ADBEMDEV.c \
	$(EMU)/ASCEMDEV.c \
	$(EMU)/MOUSEMDV.c \
	$(EMU)/PROGMAIN.c \
	$(GLUE)/MYOSGLUE.c

BUILD_APP =  $(CORE_SRCS) 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(EMU)  $(LOCAL_PATH)/$(GLUE)  $(LOCAL_PATH)../libretro

OBJECTS := ../libretro/libretro-vmac.c ../libretro/vmac-mapper.c ../libretro/vkbd.c \
	../libretro/graph.c ../libretro/diskutils.c ../libretro/fontmsx.c  \
	$(BUILD_APP)

LOCAL_SRC_FILES    += $(OBJECTS)

LOCAL_CFLAGS +=  -DMAC2=1 -DAND \
		-std=gnu99  -O3 -finline-functions -funroll-loops  -fsigned-char  \
		 -Wno-strict-prototypes -ffast-math -fomit-frame-pointer -fno-strength-reduce  -fno-builtin -finline-functions -s

LOCAL_LDLIBS    := -shared -Wl,--version-script=../libretro/link.T 

include $(BUILD_SHARED_LIBRARY)
