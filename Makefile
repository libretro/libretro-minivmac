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

ifneq (,$(findstring unix,$(platform)))
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=libretro/link.T -Wl,--no-undefined -fPIC
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib

   ifeq ($(CROSS_COMPILE),1)
	TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
	CFLAGS   += $(TARGET_RULE)
	CPPFLAGS += $(TARGET_RULE)
	CXXFLAGS += $(TARGET_RULE)
	LDFLAGS  += $(TARGET_RULE)
   endif

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
	CC = cc -arch arm64 -isysroot $(IOSSDK)
	LD = cc -arch arm64 -isysroot $(IOSSDK)
	CC	+= -mappletvos-version-min=11.0
	CXXFLAGS += -mappletvos-version-min=11.0
else ifneq (,$(findstring ios,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib

	ifeq ($(IOSSDK),)
		IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
	endif

	DEFINES := -DIOS

	ifeq ($(platform),ios9)
		CC = cc -arch armv7 -isysroot $(IOSSDK)
		LD = cc -arch armv7 -isysroot $(IOSSDK)
		CC	+= -miphoneos-version-min=8.0
		CXXFLAGS += -miphoneos-version-min=8.0
	else
		CC = cc -arch arm64 -isysroot $(IOSSDK)
		LD = cc -arch arm64 -isysroot $(IOSSDK)
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
	CFLAGS += -DGEKKO -DWIIU -DHW_RVL -D__wiiu__ -DHW_WUP -ffunction-sections -fdata-sections -mcpu=750 -meabi -mhard-float
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

# Miyoo
else ifeq ($(platform), miyoo)
   TARGET := $(TARGET_NAME)_libretro.so
   CC = /opt/miyoo/usr/bin/arm-linux-gcc
   AR = /opt/miyoo/usr/bin/arm-linux-ar
   fpic := -fPIC
   SHARED := -shared -Wl,-version-script=$(CORE_DIR)/link.T
   CFLAGS += -ffast-math -mcpu=arm926ej-s

# Windows MSVC 2010 x64
else ifeq ($(platform), windows_msvc2010_x64)
	CC  = cl.exe
	CXX = cl.exe

	CFLAGS += -wd4711 -wd4514 -wd4820 -DNO_VA_COPY=1
	CXXFLAGS += -wd4711 -wd4514 -wd4820 -DNO_VA_COPY=1

	PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin/amd64"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
	LIB := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/lib/amd64")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")

	WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -io '[A-Z]:\\.*')
	WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -io '[A-Z]:\\.*')

	WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
	WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
	WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\x64")

	INCFLAGS_PLATFORM = -I"$(WindowsSdkDirInc)"
	export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKGlIncludeDir);$(CORE_DIR)/libretro-common/include/compat/msvc
	export LIB := $(LIB);$(WindowsSDKLibDir)

	TARGET := $(TARGET_NAME)_libretro.dll
	LDFLAGS += -DLL
	LIBS = 

# Windows MSVC 2010 x86
else ifeq ($(platform), windows_msvc2010_x86)
	CC  = cl.exe
	CXX = cl.exe

	CFLAGS += -wd4711 -wd4514 -wd4820 -DNO_VA_COPY=1
	CXXFLAGS += -wd4711 -wd4514 -wd4820 -DNO_VA_COPY=1

	PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS100COMNTOOLS)../../VC/lib")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")

        WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -io '[A-Z]:\\.*')
        WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -io '[A-Z]:\\.*')

	WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
	WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
	WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib")

	INCFLAGS_PLATFORM = -I"$(WindowsSdkDirInc)"
	export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKGlIncludeDir);$(CORE_DIR)/libretro-common/include/compat/msvc
	export LIB := $(LIB);$(WindowsSDKLibDir)

	TARGET := $(TARGET_NAME)_libretro.dll
	LDFLAGS += -DLL
	LIBS = 

# Windows MSVC 2005 x86
else ifeq ($(platform), windows_msvc2005_x86)
	CC  = cl.exe
	CXX = cl.exe

	CFLAGS += -wd4711 -wd4514 -wd4820 -DNO_VA_COPY=1
	CXXFLAGS += -wd4711 -wd4514 -wd4820 -DNO_VA_COPY=1

	PATH := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../IDE")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/include")
	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS80COMNTOOLS)../../VC/lib")
	BIN := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin")

        WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\MicrosoftSDK\InstalledSDKs\8F9E5EF3-A9A5-491B-A889-C58EFFECE8B3" -v "Install Dir" | grep -o '[A-Z]:\\.*')

        WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
        WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib")

        INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKAtlIncludeDir);$(WindowsSDKCrtIncludeDir);$(WindowsSDKGlIncludeDir);$(WindowsSDKMfcIncludeDir);libretro/msvc/msvc-2005
        LIB := $(LIB);$(WindowsSDKLibDir)

	export INCLUDE := $(INCLUDE);$(INETSDK)/Include;$(CORE_DIR)/libretro-common/include/compat/msvc
	export LIB := $(LIB);$(WindowsSdkDir);$(INETSDK)/Lib
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL
	CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
	LIBS = 

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

CFLAGS += -DMAC2=1 -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565

ifeq (,$(findstring msvc,$(platform)))
CFLAGS += -std=gnu99 \
                         -O3 \
                         -finline-functions \
                         -funroll-loops \
                         -fsigned-char \
                         -Wno-strict-prototypes \
                         -ffast-math \
                         -fomit-frame-pointer \
                         -fno-strength-reduce \
                         -fno-builtin \
                         -finline-functions
endif
LDFLAGS  += -lm 

OBJOUT   = -o 
ifneq (,$(findstring msvc,$(platform)))
	OBJOUT = -Fo
endif

all: $(TARGET)


$(TARGET): $(OBJECTS)
ifneq (,$(findstring msvc,$(platform)))
	link.exe -out:$@ $(OBJECTS) $(LDFLAGS) $(LIBS)
else ifeq ($(STATIC_LINKING),1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CC) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)  
endif

%.o: %.c
	$(CC) $(fpic) $(CFLAGS) $(INCFLAGS) -c $(OBJOUT)$@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

