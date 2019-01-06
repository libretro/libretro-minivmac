#include "libretro.h"

#include "libretro-vmac.h"

#include "MACkeymap.h"

const char *retro_system_directory;

extern unsigned short int bmp[TEX_WIDTH * TEX_HEIGHT];
extern int pauseg,SND ,snd_sampler;
extern short signed int SNDBUF[1024*2];
extern char RPATH[512];
extern char RETRO_DIR[512];
extern char RETRO_ROM[512];

#include "cmdline.c"

extern void update_input(void);
extern void texture_init(void);

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
//static retro_input_poll_t input_poll_cb;
//static retro_input_state_t input_state_cb;

static void keyboard_cb(bool down, unsigned keycode, uint32_t character, uint16_t mod)
{
	unsigned char cpck=SDLKeyToMACScanCode[keycode];

  	// printf( "Down: %s, Code: %d, Char: %u, Mod: %u. ,(%d)\n",
  	//       down ? "yes" : "no", keycode, character, mod,cpck);

	if (keycode>=320);
	else{
		
		if(down && cpck!=0xff)		
			retro_key_down(cpck);//IKBD_PressSTKey(cpck,1);	
		else if(!down && cpck!=0xff)
			retro_key_up(cpck);//IKBD_PressSTKey(cpck,0);

	}

}

void retro_init(void)
{
    	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    	{
    		fprintf(stderr, "RGB565 is not supported.\n");
    		return false;
    	}

    	struct retro_keyboard_callback cb = { keyboard_cb };
    	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);

	const char *system_dir      = NULL;

	// if defined, use the system directory			
   	if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
		retro_system_directory=system_dir; 

   	if(retro_system_directory==NULL)
      		sprintf(RETRO_DIR, "%s\0",".");
   	else
      		sprintf(RETRO_DIR, "%s\0", retro_system_directory);

	printf("Retro SYSTEM_DIRECTORY %s\n",retro_system_directory);

	texture_init();
}

void retro_deinit(void)
{	 
	UnInitOSGLU();
	//environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, 0);    
}

unsigned retro_api_version(void)
{
   	return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   	(void)port;
   	(void)device;
}

void retro_get_system_info(struct retro_system_info *info)
{
   	memset(info, 0, sizeof(*info));
   	info->library_name     = "MiniVMAC";
   	info->library_version  = "v1";
   	info->valid_extensions = "dsk|DSK|img|IMG|ZIP|zip";
   	info->need_fullpath    = true;//false;
   	//info->valid_extensions = NULL; // Anything is fine, we don't care.
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   	struct retro_game_geometry geom = { TEX_WIDTH, TEX_HEIGHT, TEX_WIDTH, TEX_HEIGHT,4.0 / 3.0 };
   	struct retro_system_timing timing = { 60.0, 22255.0 }; /* = round(7833600 * 2 / 704) */
   
   	info->geometry = geom;
   	info->timing   = timing;
}
 
void retro_set_environment(retro_environment_t cb)
{
   	environ_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   	audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   	audio_batch_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   	video_cb = cb;
}
/*
void retro_set_input_poll(retro_input_poll_t cb)
{
   	input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   	input_state_cb = cb;
}
*/

void retro_reset(void){}

void retro_audio_cb( short l, short r){
	audio_cb(l,r);
}

void retro_run(void)
{
   	int x;

	if(pauseg==0){
		
		retro_loop();
		ScreenUpdate();
		update_input();		
	}
	else enter_gui();
	
/*
	if(SND==1){
   		signed short int *p=(signed short int *)SNDBUF;
   		for(x=0;x<snd_sampler;x++)audio_cb(*p++,*p++);
		//audio_batch_cb(p, snd_sampler);
	}
*/
   	video_cb(bmp,TEX_WIDTH, TEX_HEIGHT, TEX_WIDTH << 1);

}

bool retro_load_game(const struct retro_game_info *info)
{
    	const char *full_path;
/*
    	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    	{
    		fprintf(stderr, "RGB565 is not supported.\n");
    		return false;
    	}

    	struct retro_keyboard_callback cb = { keyboard_cb };
    	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
*/
    	(void)info;

    	full_path = info->path;

    	strcpy(RPATH,full_path);

	pre_main(RPATH);

    	return true;
}

void retro_unload_game(void){}

unsigned retro_get_region(void)
{
   	return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   	(void)type;
   	(void)info;
   	(void)num;
   	return false;
}

size_t retro_serialize_size(void)
{
   	return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   	return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
	return false;
}

void *retro_get_memory_data(unsigned id)
{
   	(void)id;
   	return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   	(void)id;
   	return 0;
}

void retro_cheat_reset(void) {}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   	(void)index;
   	(void)enabled;
   	(void)code;
}

