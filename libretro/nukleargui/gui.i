/* nuklear - v1.00 - public domain */

//floppy option
static char DF8NAME[512]="Choose Content\0";
static char DF9NAME[512]="Choose Content\0";

#define GET_DRIVE(code) ((code)&0x0F)
#define NUMB(a) (sizeof(a) / sizeof(*a))
#define GUIRECT nk_rect(5,25, 364+10, 222) //nk_rect(10,30, 364, 212)

typedef enum
{
	GUI_NONE = 0,
	GUI_BROWSE ,
	GUI_VKBD ,
	GUI_MAIN ,

} GUI_ACTION;

int GUISTATE=GUI_NONE;

extern int pauseg;
extern int NPAGE,SHIFTON;
extern int vkey_pressed;
extern int minivmac_statusbar;
extern unsigned int cur_port;

extern char DISKA_NAME[512];
extern char DISKB_NAME[512];
extern char TAPE_NAME[512];

extern void emu_reset(void);
extern int HandleExtension(char *path,char *ext);
extern void set_drive_type(int drive,int val);
extern void retro_shutdown_core(void);

static int
gui(struct file_browser *browser,struct nk_context *ctx)
{
    struct nk_rect total_space;

    /* window flags */
    static int border = nk_true;
    static int resize = nk_true;
    static int movable = nk_true;
    static int no_scrollbar = nk_false;
    static nk_flags window_flags = 0;
    static int minimizable = nk_false;
    static int title = nk_true;
    int tmpval;

    /* window flags */
    window_flags = 0;

    if (border) window_flags |= NK_WINDOW_BORDER;
    if (resize) window_flags |= NK_WINDOW_SCALABLE;
    if (movable) window_flags |= NK_WINDOW_MOVABLE;
    if (no_scrollbar || (pauseg==1 && LOADCONTENT==1) ) window_flags |= NK_WINDOW_NO_SCROLLBAR;
    if (minimizable) window_flags |= NK_WINDOW_MINIMIZABLE;
    if (title) window_flags |= NK_WINDOW_TITLE;

    if(pauseg==1 && SHOWKEY==1)SHOWKEY=-1;
    if(pauseg==0 && SHOWKEY==1)GUISTATE=GUI_VKBD;
    if(pauseg==1 && SHOWKEY==-1 && LOADCONTENT==1)GUISTATE=GUI_BROWSE;
    if(pauseg==1 && SHOWKEY==-1 && LOADCONTENT!=1)GUISTATE=GUI_MAIN;

    switch(GUISTATE){

	case GUI_VKBD:

		if (nk_begin(ctx,"Minivmac keyboard", GUIRECT, window_flags)){
		#include "vkboard.i"
	    	nk_end(ctx);
		}
		break;
	
	case GUI_BROWSE:

		if (nk_begin(ctx,"File Select", GUIRECT,NK_WINDOW_TITLE| NK_WINDOW_BORDER|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_MOVABLE)){
		#include "filebrowser.i"
		nk_end(ctx);
		}	
		break;

	case GUI_MAIN:

		if (nk_begin(ctx,"Minivmac GUI", GUIRECT, window_flags)){
		#include "c64menu.i"
		nk_end(ctx);
		}
		break;

	default:				
		break;

    }

    if(pauseg==1 && SHOWKEY==1)SHOWKEY=-1;
    if(pauseg==0 && SHOWKEY==1)GUISTATE=GUI_VKBD;
    if(pauseg==1 && SHOWKEY==-1 && LOADCONTENT==1)GUISTATE=GUI_BROWSE;
    if(pauseg==1 && SHOWKEY==-1 && LOADCONTENT!=1)GUISTATE=GUI_MAIN;

return GUISTATE;
}

