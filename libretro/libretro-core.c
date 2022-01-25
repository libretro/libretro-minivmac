#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#ifdef WIIU
#include <features_cpu.h>
#endif

#include "libretro.h"
#include "libretro-core.h"
#include "MACkeymap.h"
#include "vkbd.i"

#define CORE_NAME "Mini vMac"

extern int MOUSE_EMULATED;
extern int SHOWKEY;

/* Forward declarations */
int app_init(void);
int app_free(void);
int app_render(int poll);

void Emu_init(void);
void Emu_uninit(void);
void minivmac_main_exit(void);
void emu_reset(void);

void RunEmulatedTicksToTrueTime(void);
void ScreenUpdate(void);
int InitOSGLU(int argc, char **argv);
void retro_poll_event(int joyon);
void retro_key_up(int key);
void retro_key_down(int key);
void retro_loop(void);

/* Variables */
#ifdef _WIN32
char slash = '\\';
#else
char slash = '/';
#endif

bool retro_load_ok = false;

char RETRO_DIR[512];

char DISKA_NAME[512]="\0";
char DISKB_NAME[512]="\0";
char TAPE_NAME[512]="\0";
char RETRO_ROM[512];


int cpuloop=1;

#ifdef FRONTEND_SUPPORTS_RGB565
uint16_t *Retro_Screen;
uint16_t bmp[WINDOW_SIZE];
uint16_t save_Screen[WINDOW_SIZE];
#else
unsigned int *Retro_Screen;
unsigned int bmp[WINDOW_SIZE];
unsigned int save_Screen[WINDOW_SIZE];
#endif 

int minivmac_statusbar=0;

short signed int SNDBUF[1024*2];

/* FIXME: handle 50/60 */
int snd_sampler = 22255 / 60;

char RPATH[512];

int pauseg=0;
int want_quit=0;
int minivmac_kbdtype=0;

int CROP_WIDTH;
int CROP_HEIGHT;
int VIRTUAL_WIDTH;

int	retrow=640;
int	retroh=480;

unsigned minivmac_devices[ 2 ];

extern int RETROSTATUS;

const char *retro_save_directory;
const char *retro_system_directory;
const char *retro_content_directory;
char retro_system_data_directory[512];;

/*static*/ retro_input_state_t input_state_cb;
/*static*/ retro_input_poll_t input_poll_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

static char CMDFILE[512];

int loadcmdfile(char *argv)
{
   int res  = 0;
   FILE *fp = fopen(argv,"r");

   if (fp)
   {
      if ( fgets (CMDFILE , 512 , fp) != NULL )
         res=1;	
      fclose (fp);
   }

   return res;
}

int HandleExtension(char *path,char *ext)
{
   int len = strlen(path);

   if (len >= 4 &&
         path[len-4] == '.' &&
         path[len-3] == ext[0] &&
         path[len-2] == ext[1] &&
         path[len-1] == ext[2])
      return 1;

   return 0;
}

//Args for experimental_cmdline
static char ARGUV[64][1024];
static unsigned char ARGUC=0;

// Args for Core
static char XARGV[64][1024];
static const char* xargv_cmd[64];
int PARAMCOUNT=0;

extern int  skel_main(int argc, char *argv[]);
void parse_cmdline( const char *argv );

void Add_Option(const char* option)
{
   static int first=0;

   if(first==0)
   {
      PARAMCOUNT=0;	
      first++;
   }

   sprintf(XARGV[PARAMCOUNT++],"%s",option);
}

int pre_main(const char *argv)
{
   int i=0;
   bool Only1Arg;

   if (strlen(argv) > strlen("cmd"))
   {
      if( HandleExtension((char*)argv,"cmd") || HandleExtension((char*)argv,"CMD"))
         i=loadcmdfile((char*)argv);     
   }

   if(i==1)
      parse_cmdline(CMDFILE);      
   else
      parse_cmdline(argv); 

   Only1Arg = (strcmp(ARGUV[0],CORE_NAME) == 0) ? 0 : 1;

   for (i = 0; i<64; i++)
      xargv_cmd[i] = NULL;


   if(Only1Arg)
   {
      Add_Option(CORE_NAME);
      /*
         if (strlen(RPATH) >= strlen("crt"))
         if(!strcasecmp(&RPATH[strlen(RPATH)-strlen("crt")], "crt"))
         Add_Option("-cartcrt");
         */
      Add_Option(RPATH/*ARGUV[0]*/);
   }
   else
   {
      // Pass all cmdline args
      for(i = 0; i < ARGUC; i++)
         Add_Option(ARGUV[i]);
   }

   for (i = 0; i < PARAMCOUNT; i++)
      xargv_cmd[i] = (char*)(XARGV[i]);

   // skel_main(PARAMCOUNT,( char **)xargv_cmd); 
   InitOSGLU(PARAMCOUNT,( char **)xargv_cmd); 

   xargv_cmd[PARAMCOUNT - 2] = NULL;

   return 0;
}

void parse_cmdline(const char *argv)
{
   char *p,*p2,*start_of_word;
   int c,c2;
   static char buffer[512*4];
   enum states { DULL, IN_WORD, IN_STRING } state = DULL;

   strcpy(buffer,argv);
   strcat(buffer," \0");

   for (p = buffer; *p != '\0'; p++)
   {
      c = (unsigned char) *p; /* convert to unsigned char for is* functions */
      switch (state)
      {
         case DULL: /* not in a word, not in a double quoted string */
            if (isspace(c)) /* still not in a word, so ignore this char */
               continue;
            /* not a space -- if it's a double quote we go to IN_STRING, else to IN_WORD */
            if (c == '"')
            {
               state = IN_STRING;
               start_of_word = p + 1; /* word starts at *next* char, not this one */
               continue;
            }
            state = IN_WORD;
            start_of_word = p; /* word starts here */
            continue;
         case IN_STRING:
            /* we're in a double quoted string, so keep going until we hit a close " */
            if (c == '"')
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 < p; p2++, c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               ARGUC++; 

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_STRING or we handled the end above */
         case IN_WORD:
            /* we're in a word, so keep going until we get to a space */
            if (isspace(c))
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 <p; p2++,c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               ARGUC++; 

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_WORD or we handled the end above */
      }	
   }
}

long GetTicks(void)
{ // in MSec
#ifndef _ANDROID_

#if defined(WIIU)
return (cpu_features_get_time_usec());///1000;
#else
   struct timeval tv;
   gettimeofday (&tv, NULL);
   return (tv.tv_sec*1000000 + tv.tv_usec);///1000;

#endif

#else

   struct timespec now;
   clock_gettime(CLOCK_MONOTONIC, &now);
   return (now.tv_sec*1000000 + now.tv_nsec/1000);///1000;
#endif

} 

void save_bkg(void)
{
   memcpy(save_Screen,Retro_Screen,PITCH*WINDOW_SIZE);
}

void restore_bgk(void)
{
   memcpy(Retro_Screen,save_Screen,PITCH*WINDOW_SIZE);
}

void Screen_SetFullUpdate(int scr)
{
   if(scr==0 ||scr>1)
      memset(&Retro_Screen, 0, sizeof(Retro_Screen));
   if(scr>0)
      memset(&bmp,0,sizeof(bmp));
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   static const struct retro_controller_description p1_controllers[] = {
      { "Minivmac Joystick", RETRO_DEVICE_MINIVMAC_JOYSTICK },
      { "Minivmac Keyboard", RETRO_DEVICE_MINIVMAC_KEYBOARD },
   };
   static const struct retro_controller_description p2_controllers[] = {
      { "Minivmac Joystick", RETRO_DEVICE_MINIVMAC_JOYSTICK },
      { "Minivmac Keyboard", RETRO_DEVICE_MINIVMAC_KEYBOARD },
   };


   static const struct retro_controller_info ports[] = {
      { p1_controllers, 2  }, // port 1
      { p2_controllers, 2  }, // port 2
      { NULL, 0 }
   };

   cb( RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports );

   struct retro_variable variables[] = {
      {
         "minivmac_Statusbar",
         "Status Bar; disabled|enabled",
      },
      {
         "minivmac_kbdtype",
         "Keyboard Type; Callback|Poll",
      },
      { NULL, NULL },
   };

   cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

static void update_variables(void)
{

   struct retro_variable var;

   var.key = "minivmac_Statusbar";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
         if (strcmp(var.value, "enabled") == 0)
            minivmac_statusbar=1;
         if (strcmp(var.value, "disabled") == 0)
            minivmac_statusbar=0;
   }

   var.key = "minivmac_kbdtype";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
         if (strcmp(var.value, "Callback") == 0)
            minivmac_kbdtype=0;
         if (strcmp(var.value, "Poll") == 0)
            minivmac_kbdtype=1;
   }

}


void Emu_init(void)
{
#ifdef RETRO_AND
   MOUSE_EMULATED=1;
#endif 
   update_variables();
   pre_main(RPATH);
}

void Emu_uninit(void)
{
   //minivmac_main_exit();
}

void retro_shutdown_core(void)
{
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

void retro_reset(void)
{
   emu_reset();
}

static void keyboard_cb(bool down, unsigned keycode, uint32_t character, uint16_t mod)
{
	unsigned char cpck=SDLKeyToMACScanCode[keycode];

  	// printf( "Down: %s, Code: %d, Char: %u, Mod: %u. ,(%d)\n",
  	//       down ? "yes" : "no", keycode, character, mod,cpck);

	if (keycode>=320 || minivmac_kbdtype==1);
	else{
		
		if(down && cpck!=0xff)		
			retro_key_down(cpck);	
		else if(!down && cpck!=0xff)
			retro_key_up(cpck);

	}

}

void retro_init(void)
{    	
   const char *system_dir = NULL;

   struct retro_keyboard_callback cb = { keyboard_cb };
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
   {
      // if defined, use the system directory			
      retro_system_directory=system_dir;		
   }		   

   const char *content_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &content_dir) && content_dir)
   {
      // if defined, use the system directory			
      retro_content_directory=content_dir;		
   }			

   const char *save_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir)
   {
      // If save directory is defined use it, otherwise use system directory
      retro_save_directory = *save_dir ? save_dir : retro_system_directory;      
   }
   else
   {
      // make retro_save_directory the same in case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY is not implemented by the frontend
      retro_save_directory=retro_system_directory;
   }

   if(retro_system_directory==NULL)sprintf(RETRO_DIR, "%s",".");
   else sprintf(RETRO_DIR, "%s", retro_system_directory);

#if defined(__WIN32__) 
   sprintf(retro_system_data_directory, "%s\\data",RETRO_DIR);
#else
   sprintf(retro_system_data_directory, "%s/data",RETRO_DIR);
#endif

#ifdef FRONTEND_SUPPORTS_RGB565
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
#else
   enum retro_pixel_format fmt =RETRO_PIXEL_FORMAT_XRGB8888;
#endif

   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      fprintf(stderr, "PIXEL FORMAT is not supported.\n");
      exit(0);
   }

   static struct retro_input_descriptor inputDescriptors[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "R2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "L2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "L3" },
      { 0,                   0, 0,                         0, NULL } 
   };
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &inputDescriptors);

}

void retro_deinit(void)
{
   app_free(); 
   Emu_uninit(); 
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}


void retro_set_controller_port_device( unsigned port, unsigned device )
{
   if ( port < 2 )
   {
      minivmac_devices[ port ] = device;

      printf(" (%d)=%d \n",port,device);
   }
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = CORE_NAME;
   info->library_version  = "b36";
   info->valid_extensions = "dsk|img|zip|hvf|cmd";
   info->need_fullpath    = true;
   info->block_extract    = false;

}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   struct retro_game_geometry geom = { retrow, retroh, retrow, retroh,4.0 / 3.0 };
   struct retro_system_timing timing = { 60.0, 22255.0 };

   info->geometry = geom;
   info->timing   = timing;
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

void retro_audio_cb( short l, short r)
{
   audio_cb(l,r);
}

void retro_audiocb(signed short int *sound_buffer,int sndbufsize){
   int x; 
   if(pauseg==0)for(x=0;x<sndbufsize;x++)audio_cb(sound_buffer[x],sound_buffer[x]);	
}

void retro_blit(void)
{
   memcpy(Retro_Screen,bmp,PITCH*WINDOW_SIZE);
}

void retro_run(void)
{
   static int mfirst=1;
   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables();

   if(mfirst==1)
   {
      mfirst++;
      printf("MAIN FIRST\n");
      retro_load_ok=true;

      Emu_init();
      return;
   }

   if(pauseg==0)
   {
      retro_loop();
		ScreenUpdate();
		//update_input();
retro_poll_event(0);
      retro_blit();
      if(SHOWKEY==1)app_render(0);
   }
   else if (pauseg==1)app_render(1);
   //app_render(pauseg);

   video_cb(Retro_Screen,retrow,retroh,retrow<<PIXEL_BYTES);

   if(want_quit)retro_shutdown_core();
}

bool retro_load_game(const struct retro_game_info *info)
{
   /*
      struct retro_keyboard_callback cb = { keyboard_cb };
      environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
      */
   memset(RPATH, 0, sizeof(info->path));

   if (info && info->path)
      strncpy(RPATH, info->path, sizeof(RPATH) - 1);

   update_variables();

   app_init();

   memset(SNDBUF,0,1024*2*2);

   return true;
}

void retro_unload_game(void){

   pauseg=-1;
}

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

