DEBUG = 0

CORE_DIR := .
GUI        := $(CORE_DIR)/libretro/nukleargui

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

TARGET_NAME := minivmac

ifeq ($(platform), unix)
   CC = gcc
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=libretro/link.T -Wl,--no-undefined -fPIC
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib

# Classic Platforms ####################
# Platform affix = classic_<ISA>_<µARCH>
# Help at https://modmyclassic.com/comp

# (armv7 a7, hard point, neon based) ### 
# NESC, SNESC, C64 mini 
else ifeq ($(platform), classic_armv7_a7)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
    SHARED := -shared -Wl,--version-script=libretro/link.T  -Wl,--no-undefined -fPIC
	CFLAGS += -Ofast \
	-flto=4 -fwhole-program -fuse-linker-plugin \
	-fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-stack-protector -fno-ident -fomit-frame-pointer \
	-falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
	-fmerge-all-constants -fno-math-errno \
	-marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	ASFLAGS += $(CFLAGS)
	ifeq ($(shell echo `$(CC) -dumpversion` "< 4.9" | bc -l), 1)
	  CFLAGS += -march=armv7-a
	else
	  CFLAGS += -march=armv7ve
	  # If gcc is 5.0 or later
	  ifeq ($(shell echo `$(CC) -dumpversion` ">= 5" | bc -l), 1)
	    LDFLAGS += -static-libgcc -static-libstdc++
	  endif
	endif
#######################################
# tvOS
else ifeq ($(platform), tvos-arm64)
	TARGET := $(TARGET_NAME)_libretro_tvos.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
	CFLAGS += -DIOS
	ifeq ($(IOSSDK),)
		IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
	endif
else ifneq (,$(findstring ios,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib

ifeq ($(IOSSDK),)
	IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif

DEFINES := -DIOS
CC = cc -arch armv7 -isysroot $(IOSSDK)
LD = cc -arch armv7 -isysroot $(IOSSDK)

ifeq ($(platform),ios9)
	CC	+= -miphoneos-version-min=8.0
	CXXFLAGS += -miphoneos-version-min=8.0
else
	CC	+= -miphoneos-version-min=5.0
	CXXFLAGS += -miphoneos-version-min=5.0
endif

# Nintendo Game Cube
else ifeq ($(platform), ngc)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	CFLAGS += -DGEKKO -DHW_DOL -mrvl -mcpu=750 -meabi -mhard-float
	HAVE_RZLIB := 1
	STATIC_LINKING=1

# QNX / BLackberry
else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_qnx.so
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
	CC = qcc -Vgcc_ntoarmv7le
	CXX = QCC -Vgcc_ntoarmv7le
# Nintendo Wii
else ifeq ($(platform), wii)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	CFLAGS += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
	HAVE_RZLIB := 1
	STATIC_LINKING=1

# Nintendo Switch (libnx)
else ifeq ($(platform), libnx)
    include $(DEVKITPRO)/libnx/switch_rules
    EXT=a
    TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
    DEFINES := -DSWITCH=1 -U__linux__ -U__linux -DRARCH_INTERNAL
    CFLAGS	:=	 $(DEFINES) -g -O3 \
                 -fPIE -I$(LIBNX)/include/ -ffunction-sections -fdata-sections -ftls-model=local-exec -Wl,--allow-multiple-definition -specs=$(LIBNX)/switch.specs
    CFLAGS += $(INCDIRS)
    CFLAGS	+=	-D__SWITCH__ -DHAVE_LIBNX -march=armv8-a -mtune=cortex-a57 -mtp=soft
    CXXFLAGS := $(ASFLAGS) $(CFLAGS) -fno-rtti -std=gnu++11
    CFLAGS += -std=gnu11
    STATIC_LINKING = 1

# Nintendo WiiU
else ifeq ($(platform), wiiu)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	CFLAGS += -DGEKKO -DWIIU -DHW_RVL -mwup -mcpu=750 -meabi -mhard-float
	HAVE_RZLIB := 1
	STATIC_LINKING=1
#######################################
# CTR/3DS
else ifeq ($(platform), ctr)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
	CFLAGS += -DARM11 -D_3DS
	CFLAGS += -march=armv6k -mtune=mpcore -mfloat-abi=hard
	CFLAGS += -Wall -mword-relocations
	CFLAGS += -fomit-frame-pointer -ffast-math
	HAVE_RZLIB := 1
	DISABLE_ERROR_LOGGING := 1
	ARM = 1
	STATIC_LINKING=1

# RETROFW
else ifeq ($(platform), retrofw)
	EXT ?= so
	TARGET := $(TARGET_NAME)_libretro.$(EXT)
	CC = /opt/retrofw-toolchain/usr/bin/mipsel-linux-gcc
	AR = /opt/retrofw-toolchain/usr/bin/mipsel-linux-ar
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
	CFLAGS += -ffast-math -march=mips32 -mtune=mips32 -mhard-float
	LIBS = -lm

# Emscripten
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_$(platform).bc
	fpic := -fPIC
	SHARED := -shared -r
	STATIC_LINKING=1

# PS2
else ifeq ($(platform), ps2)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = mips64r5900el-ps2-elf-gcc
	AR = mips64r5900el-ps2-elf-ar
	CFLAGS += -G0 -DPS2 -DUSE_RGB565 -DABGR1555
	CXXFLAGS += -G0 -DPS2 -DUSE_RGB565 -DABGR1555
	STATIC_LINKING=1

# PSP
else ifeq ($(platform), psp1)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = psp-gcc
	AR = psp-ar
	CFLAGS += -G0 -DPSP -DUSE_RGB565
	CXXFLAGS += -G0 -DPSP -DUSE_RGB565
	STATIC_LINKING=1

# Playstation Vita
else ifeq ($(platform), vita)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = arm-vita-eabi-gcc
	AR = arm-vita-eabi-ar
	CXXFLAGS += -Wl,-q -Wall -O3
	CFLAGS += -DVITA=1
	STATIC_LINKING=1
# RS90
else ifeq ($(platform), rs90)
   TARGET := $(TARGET_NAME)_libretro.so
   CC = /opt/rs90-toolchain/usr/bin/mipsel-linux-gcc
   CXX = /opt/rs90-toolchain/usr/bin/mipsel-linux-g++
   AR = /opt/rs90-toolchain/usr/bin/mipsel-linux-ar
   fpic := -fPIC
   SHARED := -shared -Wl,-version-script=$(CORE_DIR)/link.T
   PLATFORM_DEFINES := -DCC_RESAMPLER -DCC_RESAMPLER_NO_HIGHPASS
   CFLAGS += -fomit-frame-pointer -ffast-math -march=mips32 -mtune=mips32
   CXXFLAGS += $(CFLAGS)

# GCW0
else ifeq ($(platform), gcw0)
	TARGET := $(TARGET_NAME)_libretro.so
	CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
	AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=link.T -Wl,-no-undefined

	DISABLE_ERROR_LOGGING := 1
	CFLAGS += -march=mips32 -mtune=mips32r2 -mhard-float
	LIBS = -lm

else
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=libretro/link.T -Wl,--no-undefined
endif

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g
else
   CFLAGS += -O3
endif

include Makefile.common

OBJECTS := $(SOURCES_C:.c=.o)

CFLAGS += -DMAC2=1 \
			 -std=gnu99 \
			 -O3 \
			 -finline-functions \
			 -funroll-loops \
			 -fsigned-char \
			 -Wno-strict-prototypes \
			 -ffast-math \
			 -fomit-frame-pointer \
			 -fno-strength-reduce \
			 -fno-builtin \
			 -finline-functions \
			 -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565

LDFLAGS  += -lm 

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(platform), emscripten)
	$(LD) $(fpic) $(SHARED) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)
else ifeq ($(STATIC_LINKING),1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CC) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)  
endif

%.o: %.c
	$(CC) $(fpic) $(CFLAGS) $(INCFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

