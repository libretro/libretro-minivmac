#ifndef LIBRETRO_CORE_H
#define LIBRETRO_CORE_H 1

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <stdbool.h>

//#define MATRIX(a,b) (((a) << 3) | (b))

// DEVICE MINIVMAC
#define RETRO_DEVICE_MINIVMAC_KEYBOARD RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_KEYBOARD, 0)
#define RETRO_DEVICE_MINIVMAC_JOYSTICK RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)

extern unsigned minivmac_devices[ 2 ];

#define TEX_WIDTH 640
#define TEX_HEIGHT 480

//TYPES

typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;

typedef int8_t Sint8;
typedef int16_t Sint16;
typedef int32_t Sint32;
typedef int64_t Sint64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define WINDOW_SIZE (640*480)

#define RGB565(r, g, b)  (((r) << (5+6)) | ((g) << 6) | (b))

#ifdef FRONTEND_SUPPORTS_RGB565
#define M16B
#endif

#ifdef FRONTEND_SUPPORTS_RGB565
	extern uint16_t *Retro_Screen;
	extern uint16_t bmp[WINDOW_SIZE];
	#define PIXEL_BYTES 1
	#define PIXEL_TYPE UINT16
	#define PITCH 2	
#else
	extern unsigned int *Retro_Screen;
	extern unsigned int bmp[WINDOW_SIZE];
	#define PIXEL_BYTES 2
	#define PIXEL_TYPE UINT32
	#define PITCH 4	
#endif 


//VKBD
#define NPLGN 10
#define NLIGN 5
#define NLETT 5

typedef struct {
	char norml[NLETT];
	char shift[NLETT];
	int val;	
} Mvk;

extern Mvk MVk[NPLGN*NLIGN*2];

//STRUCTURES
typedef struct{
     signed short x, y;
     unsigned short w, h;
} retro_Rect;

typedef struct{
     unsigned char *pixels;
     unsigned short w, h,pitch;
} retro_Surface;

typedef struct{
     unsigned char r,g,b;
} retro_pal;

//VARIABLES
extern int pauseg; 
extern int CROP_WIDTH;
extern int CROP_HEIGHT;
extern int VIRTUAL_WIDTH;
extern int retrow ; 
extern int retroh ;
extern int minivmac_statusbar;
extern struct retro_vfs_interface *vfs_interface;

//FUNCS
extern void mainloop_retro(void);
extern long GetTicks(void);
#endif
