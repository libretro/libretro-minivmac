

	    #define DEFHSZ 16
	    #define DEFWSZ 64

	    //misc options
	    static int showled = nk_false;
		
	    if (minivmac_statusbar) { 
		showled = nk_true;
	    }
	    else showled = nk_false;

	    //floppy option
	    static int current_drvtype = 2;
	    static int old_drvtype = 2;

	    // button toggle GUI/EMU
            nk_layout_row_dynamic(ctx, DEFHSZ, 3);
            if (nk_button_label(ctx, "Resume")){
                fprintf(stdout, "quit GUI\n");
		pauseg=0;
	    }
            if (nk_button_label(ctx, "Reset")){
                fprintf(stdout, "quit GUI & reset\n");
		pauseg=0;
		emu_reset();
	    }
            if (nk_button_label(ctx, "Quit")){
                fprintf(stdout, "quit GUI & emu\n");
		pauseg=0;
		want_quit=1;
	    }

	    //joystick options
     	    //misc options
            nk_layout_row_dynamic(ctx, DEFHSZ, 1);
            nk_checkbox_label(ctx, "Statusbar", &showled);

	    if(showled){
		if(!minivmac_statusbar)
			minivmac_statusbar = 1;
	    }
	    else if(minivmac_statusbar)
		minivmac_statusbar = 0;


	    //floppy option

	    int i;

	    for(i=0;i<2;i++)
		if(LOADCONTENT==2 && LDRIVE==(i+8));
		else if( (i==0? DISKA_NAME: DISKB_NAME)!=NULL){
		    sprintf((i==0?DF8NAME:DF9NAME),"%s",(i==0? DISKA_NAME: DISKB_NAME));
		}
		//else sprintf(LCONTENT,"Choose Content\0");
	     
            nk_layout_row_dynamic(ctx, DEFHSZ, 1);
            nk_label(ctx, "DF8:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, DEFHSZ, 1);

            if (nk_button_label(ctx, DF8NAME)){
                fprintf(stdout, "LOAD DISKA\n");
		LOADCONTENT=1;
		LDRIVE=8;
	    }

            nk_layout_row_dynamic(ctx, DEFHSZ, 1);
            nk_label(ctx, "DF9:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, DEFHSZ, 1);

            if (nk_button_label(ctx, DF9NAME)){
                fprintf(stdout, "LOAD DISKB\n");
		LOADCONTENT=1;
		LDRIVE=9;
	    }
	    if(LOADCONTENT==2 && strlen(LCONTENT) > 0){

		fprintf(stdout, "LOAD CONTENT DF%d (%s)\n",LDRIVE,LCONTENT);

		sprintf((LDRIVE==8? DISKA_NAME: DISKB_NAME),"%s",LCONTENT);
		LOADCONTENT=-1;

		//cartridge_detach_image(-1);
		//tape_image_detach(1);

		if(HandleExtension(LCONTENT,"IMG") || HandleExtension(LCONTENT,"img"))
			;//cartridge_attach_image(CARTRIDGE_CRT, LCONTENT);
		else {
/*
			if(fliplst){
				file_system_detach_disk(GET_DRIVE(LDRIVE));
				printf("Attach to flip list\n");
				file_system_attach_disk(LDRIVE, LCONTENT);
				fliplist_add_image(LDRIVE);
			}
			else if (LDRIVE==9) {
				file_system_detach_disk(GET_DRIVE(LDRIVE));
				printf("Attach DF9 disk\n");
				file_system_attach_disk(LDRIVE, LCONTENT);
			}
			else {
				printf("autostart\n");
				autostart_autodetect(LCONTENT, NULL, 0, AUTOSTART_MODE_RUN);
			}
*/
		}

	    
	    }
	    else if(LOADCONTENT==2)LOADCONTENT=-1;
	
