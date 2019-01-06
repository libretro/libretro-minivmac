DEBUG = 0

CORE_DIR := .

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

else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
else
   CC = gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=libretro/link.T -Wl,--no-undefined
endif

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g
else
   CFLAGS += -O3
endif

SOURCES_C   := \
				 $(CORE_DIR)/minivmac/src/MINEM68K.c \
				 $(CORE_DIR)/minivmac/src/GLOBGLUE.c \
				 $(CORE_DIR)/minivmac/src/M68KITAB.c \
				 $(CORE_DIR)/minivmac/src/VIAEMDEV.c \
				 $(CORE_DIR)/minivmac/src/VIA2EMDV.c \
				 $(CORE_DIR)/minivmac/src/IWMEMDEV.c \
				 $(CORE_DIR)/minivmac/src/SCCEMDEV.c \
				 $(CORE_DIR)/minivmac/src/RTCEMDEV.c \
				 $(CORE_DIR)/minivmac/src/ROMEMDEV.c \
				 $(CORE_DIR)/minivmac/src/SCSIEMDV.c \
				 $(CORE_DIR)/minivmac/src/SONYEMDV.c \
				 $(CORE_DIR)/minivmac/src/SCRNEMDV.c \
				 $(CORE_DIR)/minivmac/src/VIDEMDEV.c \
				 $(CORE_DIR)/minivmac/src/ADBEMDEV.c \
				 $(CORE_DIR)/minivmac/src/ASCEMDEV.c \
				 $(CORE_DIR)/minivmac/src/MOUSEMDV.c \
				 $(CORE_DIR)/minivmac/src/PROGMAIN.c \
				 $(CORE_DIR)/minivmac/src/OSGLUERETRO.c \
				 $(CORE_DIR)/libretro/libretro-vmac.c \
				 $(CORE_DIR)/libretro/vmac-mapper.c \
				 $(CORE_DIR)/libretro/vkbd.c \
				 $(CORE_DIR)/libretro/graph.c \
				 $(CORE_DIR)/libretro/diskutils.c \
				 $(CORE_DIR)/libretro/fontmsx.c

HINCLUDES := -I$(CORE_DIR)/minivmac/src \
			    -I$(CORE_DIR)/minivmac/cfg \
				 -I$(CORE_DIR)/libretro 

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
			 -D__LIBRETRO__

LDFLAGS  += -lm 

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)  

%.o: %.c
	$(CC) $(fpic) $(CFLAGS) $(HINCLUDES) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

