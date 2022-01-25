#include "libretro.h"
#include "libretro-core.h"

extern retro_input_poll_t input_poll_cb;
extern retro_input_state_t input_state_cb;

/* Forward declarations */
void save_bkg(void);
void Screen_SetFullUpdate(int scr);
void retro_key_up(int key);
void retro_key_down(int key);
void retro_mouse(int dx, int dy);
void retro_mouse_but(int down);

/* EMU FLAGS */
int SHOWKEY=-1;
int SHIFTON=-1;
int KBMOD=-1;
int RSTOPON=-1;
int CTRLON=-1;
int NPAGE=-1;
int KCOL=1;
int SND=1;
int vkey_pressed;
unsigned char MXjoy[2]; // joy
char Core_Key_Sate[512];
char Core_old_Key_Sate[512];
int MOUSE_EMULATED=-1,PAS=4;
int slowdown=0;
int pushi=0; //mouse button
int minivmacmouse_enable=1;
unsigned int cur_port = 2;
bool num_locked = false;

extern int minivmac_kbdtype;
extern bool retro_load_ok;

void emu_reset(void)
{
   //machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
}

void Keymap_KeyUp(int symkey)
{
   if (symkey == RETROK_NUMLOCK)
      num_locked = false;
   else 
      retro_key_up(symkey);		
}

void Keymap_KeyDown(int symkey)
{
   //FIXME detect retroarch hotkey
   switch (symkey)
   {
      case RETROK_F9:		// F9: toggle vkbd
         break;
      case RETROK_F10:	// F10: 
         pauseg=1;
         save_bkg();
         break;
      case RETROK_F11:	// F11:
         break;
      case RETROK_F12:	// F12: Reset
         //machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
         break;
      default:
         retro_key_down(symkey);
         break;
   }
}

void app_vkb_handle(void)
{
   static int oldi=-1;
   int i;

   if(oldi!=-1)
   {  
      retro_key_up(oldi);    
      oldi=-1;
   }

   if(vkey_pressed==-1)
      return;

   i=vkey_pressed;
   vkey_pressed=-1;


   if(i==-1)
      oldi=-1;
   if(i==-2)
   {
      NPAGE=-NPAGE;
      oldi=-1;
   }
   else if(i==-3)
      oldi=-1;
   else if(i==-4)
   {
      /* VKbd show/hide */
      oldi=-1;
      SHOWKEY=-SHOWKEY;
   }
   else if(i==-5)
      oldi=-1;
   else
   {
      if(i==-10) //SHIFT
      {
         //if(SHIFTON == 1)kbd_handle_keyup(RETROK_RSHIFT);
         //else kbd_handle_keydown(RETROK_LSHIFT);
         SHIFTON=-SHIFTON;

         oldi=-1;
      }
      else if(i==-11) //CTRL
      {     
         //if(CTRLON == 1)kbd_handle_keyup(RETROK_LCTRL);
         //else kbd_handle_keydown(RETROK_LCTRL);
         CTRLON=-CTRLON;

         oldi=-1;
      }
      else if(i==-12) //RSTOP
      {
         //if(RSTOPON == 1)kbd_handle_keyup(RETROK_ESCAPE);
         //else kbd_handle_keydown(RETROK_ESCAPE); 
         RSTOPON=-RSTOPON;

         oldi=-1;
      }
      else if(i==-13) //GUI
      {     
         pauseg=1;
         save_bkg();
         //SHOWKEY=-SHOWKEY;
         //Screen_SetFullUpdate(0);
         oldi=-1;
      }
      else if(i==-14) //JOY PORT TOGGLE
      {    
         //cur joy toggle
         cur_port++;
         if(cur_port>2)
            cur_port=1;
         SHOWKEY=-SHOWKEY;
         oldi=-1;
      }
      else
      {
         oldi=i;
         retro_key_down(i);
      }
   }
}

/* Core input Key (not GUI)  */
void Core_Processkey(void)
{
   int i;

   if (minivmac_kbdtype==0)
      return;

   for (i = 0; i < 320; i++)
      Core_Key_Sate[i]=input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0,i) ? 0x80: 0;

   if(memcmp( Core_Key_Sate,Core_old_Key_Sate , sizeof(Core_Key_Sate) ) )
   {
      for (i = 0; i < 320; i++)
      {
         if(Core_Key_Sate[i] && Core_Key_Sate[i]!=Core_old_Key_Sate[i]  )
         {	
            if(i==RETROK_LALT)
               continue;
            Keymap_KeyDown(i);
         }	
         else if (!Core_Key_Sate[i] && Core_Key_Sate[i]!=Core_old_Key_Sate[i]  )
         {
            if(i==RETROK_LALT)
               continue;
            Keymap_KeyUp(i);
         }	
      }
   }

   memcpy(Core_old_Key_Sate,Core_Key_Sate , sizeof(Core_Key_Sate) );
}

static int transform_axis(int val)
{
	int sign = val >= 0 ? +1 : -1;
	int absval = val >= 0 ? val : -val;
	int deadzone_val = 15 * 0x7fff / 100;
	if (absval < deadzone_val)
		return 0;
	return sign * ((absval - deadzone_val) * 5 * 10)
		/ (0x7fff - deadzone_val);
}

// Core input (not GUI) 
int Core_PollEvent(void)
{
   //   RETRO        B    Y    SLT  STA  UP   DWN  LEFT RGT  A    X    L    R    L2   R2   L3   R3
   //   INDEX        0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15
   //   C64          BOOT VKB  M/J  R/S  UP   DWN  LEFT RGT  B1   GUI  F7   F1   F5   F3   SPC  1 

   int i;
   static int jbt[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   static int vbt[16]={0x1C,0x39,0x01,0x3B,0x01,0x02,0x04,0x08,0x80,0x40,0x15,0x31,0x24,0x1F,0x6E,0x6F};
   static int kbt[4]={0,0,0,0};

   // MXjoy[0]=0;
   if(!retro_load_ok)
      return 1;
   input_poll_cb();

   int mouse_l;
   int mouse_r;
   int16_t mouse_x,mouse_y;
   mouse_x=mouse_y=0;

   if(SHOWKEY==-1 && pauseg==0)
      Core_Processkey();

   // F9 vkbd
   i=0;
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L) && kbt[i]==0)
      kbt[i]=1;
   else if ( kbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L))
   {
      kbt[i]=0;
      SHOWKEY=-SHOWKEY;
   }

   // F10 GUI
   i=1;
   if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F10) && kbt[i]==0)
      kbt[i]=1;
   else if ( kbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F10) )
   {
      kbt[i]=0;
      pauseg=1;
      save_bkg();
   }


   if(pauseg==0){ // if emulation running

      if(minivmac_devices[0]==RETRO_DEVICE_MINIVMAC_JOYSTICK)
      {
         //shortcut for joy mode only

         i=1;// Y -> vkbd toggle
         if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && jbt[i]==0 )
            jbt[i]=1;
         else if ( jbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
         {
            jbt[i]=0;
            SHOWKEY=-SHOWKEY;
            //Screen_SetFullUpdate(0);  
         }

         i=3;// START -> Gui toggle
         if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && jbt[i]==0 )
            jbt[i]=1;
         else if ( jbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
         {
            jbt[i]=0;
            pauseg=1;
            save_bkg();  
         }

         i=10;// L -> Flip next
         if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && jbt[i]==0 )
            jbt[i]=1;
         else if ( jbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
         {
            jbt[i]=0;
            //fliplist_attach_head(8, 1); 
         }

         i=11;// R -> Flip prev
         if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && jbt[i]==0 )
            jbt[i]=1;
         else if ( jbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
         {
            jbt[i]=0;
            //fliplist_attach_head(8, 0);
         }

         i=12;// L2 -> toggle joy port
         if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && jbt[i]==0 )
            jbt[i]=1;
         else if ( jbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
         {
            jbt[i]=0;
            cur_port++;
            if(cur_port>2)cur_port=1; 
         }
         /*
            i=13;// R2 -> Flip prev
            if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && jbt[i]==0 )
            jbt[i]=1;
            else if ( jbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
            {
            jbt[i]=0;
            fliplist_attach_head(8, 0);
            }
            */
      }//if vice_devices=joy

   }//if pauseg=0

   i=2;//mouse/joy toggle
   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && jbt[i]==0 )
      jbt[i]=1;
   else if ( jbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
   {
      jbt[i]=0;
      MOUSE_EMULATED=-MOUSE_EMULATED;	  
   }

   if(slowdown>0)
      return 0;

   if(MOUSE_EMULATED==1)
   {
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
         mouse_x += PAS;
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
         mouse_x -= PAS;
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
         mouse_y += PAS;
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
         mouse_y -= PAS;
      mouse_x += transform_axis(input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X));
      mouse_y += transform_axis(input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y));
      mouse_l=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
      mouse_r=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
   }
   else
   {
      mouse_x    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      mouse_y    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
      mouse_l    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
      mouse_r    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
   }

   //slowdown=1;

   static int mmbL=0,mmbR=0;

   if(mmbL==0 && mouse_l)
   {
      mmbL=1;		
      pushi=1;
   }
   else if(mmbL==1 && !mouse_l)
   {
      mmbL=0;
      pushi=0;
   }

   if(mmbR==0 && mouse_r)
      mmbR=1;		
   else if(mmbR==1 && !mouse_r)
      mmbR=0;

   if(pauseg==0 && minivmacmouse_enable && SHOWKEY==-1 )
   {
      retro_mouse((int)mouse_x, (int)mouse_y);
      retro_mouse_but(mmbL);
   }

   return 1;
}

unsigned char joystick_value[2]={0,0};

void retro_poll_event(int joyon)
{
   Core_PollEvent();

   if (joyon) /* retro joypad take control over keyboard joy */
   {
      unsigned char j = joystick_value[cur_port];

      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
         j |= 0x01;
      else
         j &= ~0x01;
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
         j |= 0x02;
      else
         j &= ~0x02;

      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) )
         j |= 0x04;
      else
         j &=~ 0x04;

      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) )
         j |= 0x08;
      else
         j &= ~0x08;

      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) )
         j |= 0x10;
      else
         j &= ~0x10;

      joystick_value[cur_port] = j;

   }

}

