
TARGET_NAME := minivmac

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

ifeq ($(platform), unix)
   CC = gcc
   TARGET := libretro-minivmac.so
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
	CXXFLAGS += $(CFLAGS)
	CPPFLAGS += $(CFLAGS)
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
   TARGET := libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
else
   CC = gcc
   TARGET := retro-hatari.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=libretro/link.T -Wl,--no-undefined
endif

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g
else
   CFLAGS += -O3
endif

EMU = minivmac/minivmac/src
GLUE = minivmac

CORE_SRCS = \
	$(EMU)/MINEM68K.o \
	$(EMU)/GLOBGLUE.o \
	$(EMU)/M68KITAB.o \
	$(EMU)/VIAEMDEV.o \
	$(EMU)/VIA2EMDV.o \
	$(EMU)/IWMEMDEV.o \
	$(EMU)/SCCEMDEV.o \
	$(EMU)/RTCEMDEV.o \
	$(EMU)/ROMEMDEV.o \
	$(EMU)/SCSIEMDV.o \
	$(EMU)/SONYEMDV.o \
	$(EMU)/SCRNEMDV.o \
	$(EMU)/VIDEMDEV.o \
	$(EMU)/ADBEMDEV.o \
	$(EMU)/ASCEMDEV.o \
	$(EMU)/MOUSEMDV.o \
	$(EMU)/PROGMAIN.o \
	$(GLUE)/MYOSGLUE.o

BUILD_APP =  $(CORE_SRCS) 

HINCLUDES := -I./$(EMU) -I./$(GLUE) -Ilibretro 

OBJECTS := libretro/libretro-vmac.o libretro/vmac-mapper.o libretro/vkbd.o \
	libretro/graph.o libretro/diskutils.o libretro/fontmsx.o  \
	$(BUILD_APP)

CFLAGS += -DMAC2=1 -std=gnu99  -O3 -finline-functions -funroll-loops  -fsigned-char  \
	-Wno-strict-prototypes -ffast-math -fomit-frame-pointer -fno-strength-reduce  -fno-builtin -finline-functions -s -fPIC

CXXFLAGS  +=	$(CFLAGS) -std=gnu++0x
CPPFLAGS += $(CFLAGS)


all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "** BUILDING $(TARGET) FOR PLATFORM $(platform) **"
	$(CC) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)  
	@echo "** BUILD SUCCESSFUL! GG NO RE **"

%.o: %.c
	$(CC) $(CFLAGS) $(HINCLUDES) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

