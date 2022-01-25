/*
	MYOSGLUE.c

	Copyright (C) 2009 Michael Hanni, Christian Bauer,
	Stephan Kochen, Paul C. Pratt, and others

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

/*
	MY Operating System GLUE. (Minimal for Libretro system)
*/

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>

#include <time.h>
#include <stdlib.h>

#include "CNFGRAPI.h"
#include "SYSDEPNS.h"
#include "ENDIANAC.h"

#include "MYOSGLUE.h"

#include "STRCONST.h"
#include "DATE2SEC.h"

extern char RETRO_ROM[512];
extern char RETRO_DIR[512];

LOCALVAR ui5b TimeDelta;
LOCALVAR int64_t current_us;

/* Forward declarations */
void DoEmulateOneTick(void);
blnr InitEmulation(void);
void DoEmulateExtraTime(void);
void retro_sound(void);
int RunOnEndOfSixtieth(void);
void retro_audio_cb(short l, short r);

/* --- some simple utilities --- */

GLOBALPROC MyMoveBytes(anyp srcPtr, anyp destPtr, si5b byteCount)
{
	(void) memcpy((char *)destPtr, (char *)srcPtr, byteCount);
}

/* --- control mode and internationalization --- */

#define NeedCell2PlainAsciiMap 1

#include "INTLCHAR.h"

LOCALVAR ui3p ScalingTabl = nullpr;

LOCALVAR char *d_arg = NULL;
LOCALVAR char *n_arg = NULL;

#if CanGetAppPath
LOCALVAR char *app_parent = NULL;
LOCALVAR char *app_name = NULL;
#endif

LOCALFUNC tMacErr ChildPath(char *x, char *y, char **r)
{
	tMacErr err = mnvm_miscErr;
	int nx = strlen(x);
	int ny = strlen(y);
	{
		if ((nx > 0) && ('/' == x[nx - 1])) {
			--nx;
		}
		{
			int nr = nx + 1 + ny;
			char *p = malloc(nr + 1);
			if (p != NULL) {
				char *p2 = p;
				(void) memcpy(p2, x, nx);
				p2 += nx;
				*p2++ = '/';
				(void) memcpy(p2, y, ny);
				p2 += ny;
				*p2 = 0;
				*r = p;
				err = mnvm_noErr;
			}
		}
	}

	return err;
}

#if UseActvCode || IncludeSonyNew
LOCALFUNC tMacErr FindOrMakeChild(char *x, char *y, char **r)
{
	tMacErr err = mnvm_miscErr;

	return err;
}
#endif

LOCALPROC MyMayFree(char *p)
{
	if (NULL != p) {
		free(p);
	}
}

/* --- sending debugging info to file --- */

#if dbglog_HAVE

#define dbglog_ToStdErr 0

#if ! dbglog_ToStdErr
LOCALVAR FILE *dbglog_File = NULL;
#endif

LOCALFUNC blnr dbglog_open0(void)
{
#if dbglog_ToStdErr
	return trueblnr;
#else
	dbglog_File = fopen("dbglog.txt", "w");
	return (NULL != dbglog_File);
#endif
}

LOCALPROC dbglog_write0(char *s, uimr L)
{
#if dbglog_ToStdErr
	(void) fwrite(s, 1, L, stderr);
#else
	if (dbglog_File != NULL) {
		(void) fwrite(s, 1, L, dbglog_File);
	}
#endif
}

LOCALPROC dbglog_close0(void)
{
#if ! dbglog_ToStdErr
	if (dbglog_File != NULL) {
		fclose(dbglog_File);
		dbglog_File = NULL;
	}
#endif
}

#endif

/* --- debug settings and utilities --- */

#if ! dbglog_HAVE
#define WriteExtraErr(s)
#else
LOCALPROC WriteExtraErr(char *s)
{
	dbglog_writeCStr("*** error: ");
	dbglog_writeCStr(s);
	dbglog_writeReturn();
}
#endif

#include "COMOSGLU.h"
#include "CONTROLM.h"

/* --- parameter buffers --- */

#if IncludePbufs
LOCALVAR void *PbufDat[NumPbufs];
#endif

#if IncludePbufs
LOCALFUNC tMacErr PbufNewFromPtr(void *p, ui5b count, tPbuf *r)
{
	tPbuf i;
	tMacErr err;

	if (! FirstFreePbuf(&i)) {
		free(p);
		err = mnvm_miscErr;
	} else {
		*r = i;
		PbufDat[i] = p;
		PbufNewNotify(i, count);
		err = mnvm_noErr;
	}

	return err;
}
#endif

#if IncludePbufs
GLOBALFUNC tMacErr PbufNew(ui5b count, tPbuf *r)
{
	tMacErr err = mnvm_miscErr;

	void *p = calloc(1, count);
	if (NULL != p) {
		err = PbufNewFromPtr(p, count, r);
	}

	return err;
}
#endif

#if IncludePbufs
GLOBALPROC PbufDispose(tPbuf i)
{
	free(PbufDat[i]);
	PbufDisposeNotify(i);
}
#endif

#if IncludePbufs
LOCALPROC UnInitPbufs(void)
{
	tPbuf i;

	for (i = 0; i < NumPbufs; ++i) {
		if (PbufIsAllocated(i)) {
			PbufDispose(i);
		}
	}
}
#endif

#if IncludePbufs
GLOBALPROC PbufTransfer(ui3p Buffer,
	tPbuf i, ui5r offset, ui5r count, blnr IsWrite)
{
	void *p = ((ui3p)PbufDat[i]) + offset;
	if (IsWrite) {
		(void) memcpy(p, Buffer, count);
	} else {
		(void) memcpy(Buffer, p, count);
	}
}
#endif

/* --- text translation --- */

#if IncludePbufs
/* this is table for Windows, any changes needed for X? */
LOCALVAR const ui3b Native2MacRomanTab[] = {
	0xAD, 0xB0, 0xE2, 0xC4, 0xE3, 0xC9, 0xA0, 0xE0,
	0xF6, 0xE4, 0xB6, 0xDC, 0xCE, 0xB2, 0xB3, 0xB7,
	0xB8, 0xD4, 0xD5, 0xD2, 0xD3, 0xA5, 0xD0, 0xD1,
	0xF7, 0xAA, 0xC5, 0xDD, 0xCF, 0xB9, 0xC3, 0xD9,
	0xCA, 0xC1, 0xA2, 0xA3, 0xDB, 0xB4, 0xBA, 0xA4,
	0xAC, 0xA9, 0xBB, 0xC7, 0xC2, 0xBD, 0xA8, 0xF8,
	0xA1, 0xB1, 0xC6, 0xD7, 0xAB, 0xB5, 0xA6, 0xE1,
	0xFC, 0xDA, 0xBC, 0xC8, 0xDE, 0xDF, 0xF0, 0xC0,
	0xCB, 0xE7, 0xE5, 0xCC, 0x80, 0x81, 0xAE, 0x82,
	0xE9, 0x83, 0xE6, 0xE8, 0xED, 0xEA, 0xEB, 0xEC,
	0xF5, 0x84, 0xF1, 0xEE, 0xEF, 0xCD, 0x85, 0xF9,
	0xAF, 0xF4, 0xF2, 0xF3, 0x86, 0xFA, 0xFB, 0xA7,
	0x88, 0x87, 0x89, 0x8B, 0x8A, 0x8C, 0xBE, 0x8D,
	0x8F, 0x8E, 0x90, 0x91, 0x93, 0x92, 0x94, 0x95,
	0xFD, 0x96, 0x98, 0x97, 0x99, 0x9B, 0x9A, 0xD6,
	0xBF, 0x9D, 0x9C, 0x9E, 0x9F, 0xFE, 0xFF, 0xD8
};
#endif

#if IncludePbufs
LOCALFUNC tMacErr NativeTextToMacRomanPbuf(char *x, tPbuf *r)
{
	if (NULL == x) {
		return mnvm_miscErr;
	} else {
		ui3p p;
		ui5b L = strlen(x);

		p = (ui3p)malloc(L);
		if (NULL == p) {
			return mnvm_miscErr;
		} else {
			ui3b *p0 = (ui3b *)x;
			ui3b *p1 = (ui3b *)p;
			int i;

			for (i = L; --i >= 0; ) {
				ui3b v = *p0++;
				if (v >= 128) {
					v = Native2MacRomanTab[v - 128];
				} else if (10 == v) {
					v = 13;
				}
				*p1++ = v;
			}

			return PbufNewFromPtr(p, L, r);
		}
	}
}
#endif

#if IncludePbufs
/* this is table for Windows, any changes needed for X? */
LOCALVAR const ui3b MacRoman2NativeTab[] = {
	0xC4, 0xC5, 0xC7, 0xC9, 0xD1, 0xD6, 0xDC, 0xE1,
	0xE0, 0xE2, 0xE4, 0xE3, 0xE5, 0xE7, 0xE9, 0xE8,
	0xEA, 0xEB, 0xED, 0xEC, 0xEE, 0xEF, 0xF1, 0xF3,
	0xF2, 0xF4, 0xF6, 0xF5, 0xFA, 0xF9, 0xFB, 0xFC,
	0x86, 0xB0, 0xA2, 0xA3, 0xA7, 0x95, 0xB6, 0xDF,
	0xAE, 0xA9, 0x99, 0xB4, 0xA8, 0x80, 0xC6, 0xD8,
	0x81, 0xB1, 0x8D, 0x8E, 0xA5, 0xB5, 0x8A, 0x8F,
	0x90, 0x9D, 0xA6, 0xAA, 0xBA, 0xAD, 0xE6, 0xF8,
	0xBF, 0xA1, 0xAC, 0x9E, 0x83, 0x9A, 0xB2, 0xAB,
	0xBB, 0x85, 0xA0, 0xC0, 0xC3, 0xD5, 0x8C, 0x9C,
	0x96, 0x97, 0x93, 0x94, 0x91, 0x92, 0xF7, 0xB3,
	0xFF, 0x9F, 0xB9, 0xA4, 0x8B, 0x9B, 0xBC, 0xBD,
	0x87, 0xB7, 0x82, 0x84, 0x89, 0xC2, 0xCA, 0xC1,
	0xCB, 0xC8, 0xCD, 0xCE, 0xCF, 0xCC, 0xD3, 0xD4,
	0xBE, 0xD2, 0xDA, 0xDB, 0xD9, 0xD0, 0x88, 0x98,
	0xAF, 0xD7, 0xDD, 0xDE, 0xB8, 0xF0, 0xFD, 0xFE
};
#endif

#if IncludePbufs
LOCALFUNC blnr MacRomanTextToNativePtr(tPbuf i, blnr IsFileName,
	ui3p *r)
{
	ui3p p;
	void *Buffer = PbufDat[i];
	ui5b L = PbufSize[i];

	p = (ui3p)malloc(L + 1);
	if (p != NULL) {
		ui3b *p0 = (ui3b *)Buffer;
		ui3b *p1 = (ui3b *)p;
		int j;

		if (IsFileName) {
			for (j = L; --j >= 0; ) {
				ui3b x = *p0++;
				if (x < 32) {
					x = '-';
				} else if (x >= 128) {
					x = MacRoman2NativeTab[x - 128];
				} else {
					switch (x) {
						case '/':
						case '<':
						case '>':
						case '|':
						case ':':
							x = '-';
						default:
							break;
					}
				}
				*p1++ = x;
			}
			if ('.' == p[0]) {
				p[0] = '-';
			}
		} else {
			for (j = L; --j >= 0; ) {
				ui3b x = *p0++;
				if (x >= 128) {
					x = MacRoman2NativeTab[x - 128];
				} else if (13 == x) {
					x = '\n';
				}
				*p1++ = x;
			}
		}
		*p1 = 0;

		*r = p;
		return trueblnr;
	}
	return falseblnr;
}
#endif

LOCALPROC NativeStrFromCStr(char *r, char *s)
{
	ui3b ps[ClStrMaxLength];
	int i;
	int L;

	ClStrFromSubstCStr(&L, ps, s);

	for (i = 0; i < L; ++i) {
		r[i] = Cell2PlainAsciiMap[ps[i]];
	}

	r[L] = 0;
}

/* --- drives --- */

#define NotAfileRef NULL

LOCALVAR FILE *Drives[NumDrives]; /* open disk image files */
#if IncludeSonyGetName || IncludeSonyNew
LOCALVAR char *DriveNames[NumDrives];
#endif

LOCALPROC InitDrives(void)
{
	/*
		This isn't really needed, Drives[i] and DriveNames[i]
		need not have valid values when not vSonyIsInserted[i].
	*/
	tDrive i;

	for (i = 0; i < NumDrives; ++i) {
		Drives[i] = NotAfileRef;
#if IncludeSonyGetName || IncludeSonyNew
		DriveNames[i] = NULL;
#endif
	}
}

GLOBALFUNC tMacErr vSonyTransfer(blnr IsWrite, ui3p Buffer,
	tDrive Drive_No, ui5r Sony_Start, ui5r Sony_Count,
	ui5r *Sony_ActCount)
{
	tMacErr err = mnvm_miscErr;
	FILE *refnum = Drives[Drive_No];
	ui5r NewSony_Count = 0;

	if (0 == fseek(refnum, Sony_Start, SEEK_SET)) {
		if (IsWrite) {
			NewSony_Count = fwrite(Buffer, 1, Sony_Count, refnum);
		} else {
			NewSony_Count = fread(Buffer, 1, Sony_Count, refnum);
		}

		if (NewSony_Count == Sony_Count) {
			err = mnvm_noErr;
		}
	}

	if (nullpr != Sony_ActCount) {
		*Sony_ActCount = NewSony_Count;
	}

	return err; /*& figure out what really to return &*/
}

GLOBALFUNC tMacErr vSonyGetSize(tDrive Drive_No, ui5r *Sony_Count)
{
	tMacErr err = mnvm_miscErr;
	FILE *refnum = Drives[Drive_No];
	long v;

	if (0 == fseek(refnum, 0, SEEK_END)) {
		v = ftell(refnum);
		if (v >= 0) {
			*Sony_Count = v;
			err = mnvm_noErr;
		}
	}

	return err; /*& figure out what really to return &*/
}


LOCALFUNC tMacErr vSonyEject0(tDrive Drive_No, blnr deleteit)
{
	FILE *refnum = Drives[Drive_No];

	DiskEjectedNotify(Drive_No);

	fclose(refnum);
	Drives[Drive_No] = NotAfileRef; /* not really needed */

#if IncludeSonyGetName || IncludeSonyNew
	{
		char *s = DriveNames[Drive_No];
		if (NULL != s) {
			if (deleteit) {
				remove(s);
			}
			free(s);
			DriveNames[Drive_No] = NULL; /* not really needed */
		}
	}
#endif

	return mnvm_noErr;
}

GLOBALFUNC tMacErr vSonyEject(tDrive Drive_No)
{
	return vSonyEject0(Drive_No, falseblnr);
}

#if IncludeSonyNew
GLOBALFUNC tMacErr vSonyEjectDelete(tDrive Drive_No)
{
	return vSonyEject0(Drive_No, trueblnr);
}
#endif

LOCALPROC UnInitDrives(void)
{
	tDrive i;

	for (i = 0; i < NumDrives; ++i) {
		if (vSonyIsInserted(i)) {
			(void) vSonyEject(i);
		}
	}
}

#if IncludeSonyGetName
GLOBALFUNC tMacErr vSonyGetName(tDrive Drive_No, tPbuf *r)
{
	char *drivepath = DriveNames[Drive_No];
	if (NULL == drivepath) {
		return mnvm_miscErr;
	} else {
		char *s = strrchr(drivepath, '/');
		if (NULL == s) {
			s = drivepath;
		} else {
			++s;
		}
		return NativeTextToMacRomanPbuf(s, r);
	}
}
#endif

LOCALFUNC blnr Sony_Insert0(FILE *refnum, blnr locked,
	char *drivepath)
{
	tDrive Drive_No;
	blnr IsOk = falseblnr;

	if (! FirstFreeDisk(&Drive_No)) {
		MacMsg(kStrTooManyImagesTitle, kStrTooManyImagesMessage,
			falseblnr);
	} else {
		/* printf("Sony_Insert0 %d\n", (int)Drive_No); */


		{
			Drives[Drive_No] = refnum;
			DiskInsertNotify(Drive_No, locked);

#if IncludeSonyGetName || IncludeSonyNew
			{
				ui5b L = strlen(drivepath);
				char *p = malloc(L + 1);
				if (p != NULL) {
					(void) memcpy(p, drivepath, L + 1);
				}
				DriveNames[Drive_No] = p;
			}
#endif

			IsOk = trueblnr;
		}
	}

	if (! IsOk) {
		fclose(refnum);
	}

	return IsOk;
}

/*LOCALFUNC*/ blnr Sony_Insert1(char *drivepath, blnr silentfail)
{
	blnr locked = falseblnr;
	/* printf("Sony_Insert1 %s\n", drivepath); */
	FILE *refnum = fopen(drivepath, "rb+");
	if (NULL == refnum) {
		locked = trueblnr;
		refnum = fopen(drivepath, "rb");
	}
	if (NULL == refnum) {
		if (! silentfail) {
			MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
		}
	} else {
		return Sony_Insert0(refnum, locked, drivepath);
	}
	return falseblnr;
}

LOCALFUNC blnr Sony_Insert2(char *s)
{
	char *d =
#if CanGetAppPath
		(NULL == d_arg) ? app_parent :
#endif
		d_arg;
	blnr IsOk = falseblnr;

	if (NULL == d) {
		IsOk = Sony_Insert1(s, trueblnr);
	} else {
		char *t;

		if (mnvm_noErr == ChildPath(d, s, &t)) {
			IsOk = Sony_Insert1(t, trueblnr);
			free(t);
		}
	}

	return IsOk;
}

LOCALFUNC blnr LoadInitialImages(void)
{
	if (! AnyDiskInserted()) {
		int n = NumDrives > 9 ? 9 : NumDrives;
		int i;
		char s[] = "disk?.dsk";

		for (i = 1; i <= n; ++i) {
			s[4] = '0' + i;
			if (! Sony_Insert2(s)) {
				/* stop on first error (including file not found) */
				return trueblnr;
			}
		}
	}

	return trueblnr;
}

#if IncludeSonyNew
LOCALFUNC blnr WriteZero(FILE *refnum, ui5b L)
{
#define ZeroBufferSize 2048
	ui5b i;
	ui3b buffer[ZeroBufferSize];

	memset(&buffer, 0, ZeroBufferSize);

	while (L > 0) {
		i = (L > ZeroBufferSize) ? ZeroBufferSize : L;
		if (fwrite(buffer, 1, i, refnum) != i) {
			return falseblnr;
		}
		L -= i;
	}
	return trueblnr;
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDisk0(ui5b L, char *drivepath)
{
	blnr IsOk = falseblnr;
	FILE *refnum = fopen(drivepath, "wb+");
	if (NULL == refnum) {
		MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
	} else {
		if (WriteZero(refnum, L)) {
			IsOk = Sony_Insert0(refnum, falseblnr, drivepath);
			refnum = NULL;
		}
		if (refnum != NULL) {
			fclose(refnum);
		}
		if (! IsOk) {
			(void) remove(drivepath);
		}
	}
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDisk(ui5b L, char *drivename)
{
	char *d =
#if CanGetAppPath
		(NULL == d_arg) ? app_parent :
#endif
		d_arg;

	if (NULL == d) {
		MakeNewDisk0(L, drivename); /* in current directory */
	} else {
		tMacErr err;
		char *t = NULL;
		char *t2 = NULL;

		//if (mnvm_noErr == (err = FindOrMakeChild(d, "out", &t)))
		if (mnvm_noErr == (err = ChildPath(t, drivename, &t2)))
		{
			MakeNewDisk0(L, t2);
		}

		MyMayFree(t2);
		MyMayFree(t);
	}
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDiskAtDefault(ui5b L)
{
	char s[ClStrMaxLength + 1];

	NativeStrFromCStr(s, "untitled.dsk");
	MakeNewDisk(L, s);
}
#endif

/* --- ROM --- */
#ifdef AND
LOCALVAR char *rom_path = "/mnt/sdcard/vmac/MacIIx.ROM";
#else
LOCALVAR char *rom_path = NULL;
#endif

LOCALFUNC tMacErr LoadMacRomFrom(char *path)
{
	tMacErr err;
	FILE *ROM_File;
	int File_Size;

	ROM_File = fopen(path, "rb");
	if (NULL == ROM_File) {
		err = mnvm_fnfErr;
	} else {
		File_Size = fread(ROM, 1, kROM_Size, ROM_File);
		if (File_Size != kROM_Size) {
			if (feof(ROM_File)) {
				err = mnvm_eofErr;
			} else {
				err = mnvm_miscErr;
			}
		} else {
			err = mnvm_noErr;
		}
		fclose(ROM_File);
	}

	return err;
}

#if 0
#include <pwd.h>
#include <unistd.h>
#endif

LOCALFUNC tMacErr FindUserHomeFolder(char **r)
{
	tMacErr err = mnvm_fnfErr;
	char *s = getenv("HOME");

#if 0
	if (NULL == s) {
		struct passwd *user = getpwuid(getuid());
		if (user != NULL) {
			s = user->pw_dir;
		}
	} else
#endif
	{
		*r = s;
		err = mnvm_noErr;
	}

	return err;
}

LOCALFUNC tMacErr LoadMacRomFromHome(void)
{
	tMacErr err;
	char *s;
	char *t = NULL;
	char *t2 = NULL;
	char *t3 = NULL;

	if (mnvm_noErr == (err = FindUserHomeFolder(&s)))
	if (mnvm_noErr == (err = ChildPath(s, ".gryphel", &t)))
	if (mnvm_noErr == (err = ChildPath(t, "mnvm_rom", &t2)))
	if (mnvm_noErr == (err = ChildPath(t2, RomFileName, &t3)))
	{
		err = LoadMacRomFrom(t3);
	}

	MyMayFree(t3);
	MyMayFree(t2);
	MyMayFree(t);

	return err;
}

#if CanGetAppPath
LOCALFUNC tMacErr LoadMacRomFromAppPar(void)
{
	tMacErr err;
	char *d =
#if CanGetAppPath
		(NULL == d_arg) ? app_parent :
#endif
		d_arg;
	char *t = NULL;

	if (NULL == d) {
		err = mnvm_fnfErr;
	} else {
		if (mnvm_noErr == (err = ChildPath(d, RomFileName,
			&t)))
		{
			err = LoadMacRomFrom(t);
		}
	}

	MyMayFree(t);

	return err;
}
#endif

LOCALFUNC blnr LoadMacRom(void)
{
	tMacErr err;
	
	sprintf(RETRO_ROM,"%s/MacIIx.ROM\0",RETRO_DIR);
	if (mnvm_fnfErr == (err = LoadMacRomFrom(RETRO_ROM)))
	{
		sprintf(RETRO_ROM,"%s/MacII.ROM\0",RETRO_DIR);
		if (mnvm_fnfErr == (err = LoadMacRomFrom(RETRO_ROM)))
		if ((NULL == rom_path)
			|| (mnvm_fnfErr == (err = LoadMacRomFrom(rom_path))))
		if (mnvm_fnfErr == (err = LoadMacRomFromHome()))
#if CanGetAppPath
		if (mnvm_fnfErr == (err = LoadMacRomFromAppPar()))
#endif
		if (mnvm_fnfErr == (err = LoadMacRomFrom(RomFileName)))
		{
		}
	}

	if (mnvm_noErr != err) {
		if (mnvm_fnfErr == err) {
			MacMsg(kStrNoROMTitle, kStrNoROMMessage, trueblnr);
		} else if (mnvm_eofErr == err) {
			MacMsg(kStrShortROMTitle, kStrShortROMMessage,
				trueblnr);
		} else {
			MacMsg(kStrNoReadROMTitle, kStrNoReadROMMessage,
				trueblnr);
		}

		SpeedStopped = trueblnr;
	}

	return trueblnr; /* keep launching Mini vMac, regardless */
}

#if UseActvCode

#define ActvCodeFileName "act_1"

LOCALFUNC tMacErr ActvCodeFileLoad(ui3p p)
{
	tMacErr err;
	char *s;
	char *t = NULL;
	char *t2 = NULL;
	char *t3 = NULL;

	if (mnvm_noErr == (err = FindUserHomeFolder(&s)))
	if (mnvm_noErr == (err = ChildPath(s, ".gryphel", &t)))
	if (mnvm_noErr == (err = ChildPath(t, "mnvm_act", &t2)))
	if (mnvm_noErr == (err = ChildPath(t2, ActvCodeFileName, &t3)))
	{
		FILE *Actv_File;
		int File_Size;

		Actv_File = fopen(t3, "rb");
		if (NULL == Actv_File) {
			err = mnvm_fnfErr;
		} else {
			File_Size = fread(p, 1, ActvCodeFileLen, Actv_File);
			if (File_Size != ActvCodeFileLen) {
				if (feof(Actv_File)) {
					err = mnvm_eofErr;
				} else {
					err = mnvm_miscErr;
				}
			} else {
				err = mnvm_noErr;
			}
			fclose(Actv_File);
		}
	}

	MyMayFree(t3);
	MyMayFree(t2);
	MyMayFree(t);

	return err;
}

LOCALFUNC tMacErr ActvCodeFileSave(ui3p p)
{
	tMacErr err;
	char *s;
	char *t = NULL;
	char *t2 = NULL;
	char *t3 = NULL;

	//if (mnvm_noErr == (err = FindUserHomeFolder(&s)))
	//if (mnvm_noErr == (err = FindOrMakeChild(s, ".gryphel", &t)))
	//if (mnvm_noErr == (err = FindOrMakeChild(t, "mnvm_act", &t2)))
	if (mnvm_noErr == (err = ChildPath(t2, ActvCodeFileName, &t3)))
	{
		FILE *Actv_File;
		int File_Size;

		Actv_File = fopen(t3, "wb+");
		if (NULL == Actv_File) {
			err = mnvm_fnfErr;
		} else {
			File_Size = fwrite(p, 1, ActvCodeFileLen, Actv_File);
			if (File_Size != ActvCodeFileLen) {
				err = mnvm_miscErr;
			} else {
				err = mnvm_noErr;
			}
			fclose(Actv_File);
		}
	}

	MyMayFree(t3);
	MyMayFree(t2);
	MyMayFree(t);

	return err;
}

#endif /* UseActvCode */

/* --- video out --- */

inline unsigned short RGBA8888_to_RGB565 (unsigned rgb)
{	
	rgb=rgb>>8;

    return 
        (((rgb >> 19) & 0x1f) << 11) |
        (((rgb >> 10) & 0x3f) <<  5) |
        (((rgb >>  3) & 0x1f)      );
}

#if 0
#if (0 != vMacScreenDepth) && (vMacScreenDepth < 4)
LOCALPROC SetUpColorTabl(void)
{
	int i;
	int j;
	int k;
	ui5p p4;

	p4 = (ui5p)ScalingTabl;
	for (i = 0; i < 256; ++i) {
		for (k = 1 << (3 - vMacScreenDepth); --k >= 0; ) {
			j = (i >> (k << vMacScreenDepth)) & (CLUT_size - 1);

			*p4++ = (((long)CLUT_reds[j] & 0xFF00) << 8)
				| ((long)CLUT_greens[j] & 0xFF00)
				| (((long)CLUT_blues[j] & 0xFF00) >> 8);

		}
	}
}
#endif
#endif

GLOBALPROC UpdateScreen(ui3p* destination, si4b top, si4b left,
                              si4b bottom, si4b right)
{
	int i;
	int j;
	ui5b t0;
    
	UnusedParam(left);
	UnusedParam(right);

#if 0 != vMacScreenDepth
	if (UseColorMode) {
#if vMacScreenDepth < 4
		ui5b CLUT_final[CLUT_size];
#endif
		ui3b *p1 = GetCurDrawBuff()
        + (ui5r)vMacScreenByteWidth * top;

        unsigned short *p2 = (unsigned short *)destination
        + (ui5r)vMacScreenWidth * top;

#if vMacScreenDepth < 4

/*
		if (! ColorTransValid) {
			SetUpColorTabl();
			ColorTransValid = trueblnr;
		}
*/

		for (i = 0; i < CLUT_size; ++i) {
			CLUT_final[i] = (((long)CLUT_reds[i] & 0xFF00) << 16)
            | (((long)CLUT_greens[i] & 0xFF00) << 8)
            | ((long)CLUT_blues[i] & 0xFF00);
		}

#endif
        
		for (i = bottom - top; --i >= 0; ) {
#if 4 == vMacScreenDepth
			for (j = vMacScreenWidth; --j >= 0; ) {
				t0 = do_get_mem_word(p1);
				p1 += 2;
				*p2++ =
                ((t0 & 0x7C00) << 17) |
                ((t0 & 0x7000) << 12) |
                ((t0 & 0x03E0) << 14) |
                ((t0 & 0x0380) << 9) |
                ((t0 & 0x001F) << 11) |
                ((t0 & 0x001C) << 6);
#if 0
                ((t0 & 0x7C00) << 9) |
                ((t0 & 0x7000) << 4) |
                ((t0 & 0x03E0) << 6) |
                ((t0 & 0x0380) << 1) |
                ((t0 & 0x001F) << 3) |
                ((t0 & 0x001C) >> 2);
#endif
			}
#elif 5 == vMacScreenDepth
			for (j = vMacScreenWidth; --j >= 0; ) {
				t0 = do_get_mem_long(p1);
				p1 += 4;
				*p2++ = t0 << 8;
			}
#else
			for (j = vMacScreenByteWidth; --j >= 0; ) {
				t0 = *p1++;
#if 1 == vMacScreenDepth
				*p2++ = CLUT_final[t0 >> 6];
				*p2++ = CLUT_final[(t0 >> 4) & 0x03];
				*p2++ = CLUT_final[(t0 >> 2) & 0x03];
				*p2++ = CLUT_final[t0 & 0x03];
#elif 2 == vMacScreenDepth
				*p2++ = CLUT_final[t0 >> 4];
				*p2++ = CLUT_final[t0 & 0x0F];
#elif 3 == vMacScreenDepth
				*p2++ = RGBA8888_to_RGB565(CLUT_final[t0]);	
			
#endif
			}
#endif
		}
	} else
#endif

	{
		int k;
		ui3b *p1 = GetCurDrawBuff()
        + (ui5r)vMacScreenMonoByteWidth * top;		
        	unsigned short *p2 = (unsigned  short*)destination + (ui5r)vMacScreenWidth * top;
        
		for (i = bottom - top; --i >= 0; ) {
			for (j = vMacScreenMonoByteWidth; --j >= 0; ) {
				t0 = *p1++;
				for (k = 8; --k >= 0; ) {					
					*p2++ = ((t0 >> k) & 0x01) - 1;
				}
			}
		}
		
	}


}
#include "libretro-core.h"

void ScreenUpdate () {
	si4b top, left, bottom, right;

	top = 0;
	left = 0;
	bottom = vMacScreenHeight;
	right = vMacScreenWidth;

	int changesWidth = vMacScreenWidth;
	int changesHeight =vMacScreenHeight;
	int changesSize = changesWidth * changesHeight;
	int i,x,y;

#ifdef FRONTEND_SUPPORTS_RGB565
	unsigned short int* px = (unsigned short int *)&bmp[0];
#else
	unsigned int* px = (unsigned int *)&bmp[0];
#endif 

        UpdateScreen((anyp*)px, top, left, bottom, right);	
	ScreenClearChanges();

	return;
}

LOCALVAR blnr CurSpeedStopped = trueblnr;

GLOBALVAR ui3b CurMouseButton = falseblnr;

/* --- mouse --- */


void retro_mouse(int dx, int dy)
{
	HaveMouseMotion = trueblnr;
	MyMousePositionSetDelta(dx, dy);
	
}

void retro_mouse_but(int down){

	CurMouseButton = down?trueblnr:falseblnr;
	MyMouseButtonSet(CurMouseButton);
}
/* cursor hiding */

/* --- keyboard input --- */

void retro_key_down(int key){

	Keyboard_UpdateKeyMap2(key, trueblnr);
}

void retro_key_up(int key){

	Keyboard_UpdateKeyMap2(key, falseblnr);
}

extern char RPATH[512];
int DSKLOAD=1; 

GLOBALOSGLUPROC DoneWithDrawingForTick(void)
{

}

void retro_init_time(void)
{
	time_t Current_Time;
	struct tm *s;

	(void) time(&Current_Time);
	s = localtime(&Current_Time);
	TimeDelta = Date2MacSeconds(s->tm_sec, s->tm_min, s->tm_hour,
				    s->tm_mday, 1 + s->tm_mon, 1900 + s->tm_year);
}

//retro loop
void retro_loop(void)
{
	current_us += 1000000 / 60;
   if(DSKLOAD==1)
   {
      (void) Sony_Insert1(RPATH, falseblnr);
      DSKLOAD=0;
   }

   //CheckForSystemEvents();
   //CheckForSavedTasks();
   if (ForceMacOff)
      return;

   RunOnEndOfSixtieth();
   DoEmulateExtraTime();
}

GLOBALOSGLUPROC WaitForNextTick(void) { }

/* --- time, date, location --- */

LOCALVAR ui5b TrueEmulatedTime = 0;
LOCALVAR ui5b CurEmulatedTime = 0;


#define TicksPerSecond 1000000

LOCALVAR ui5b NewMacDateInSeconds;

LOCALVAR ui5b LastTimeSec;
LOCALVAR ui5b LastTimeUsec;

LOCALPROC GetCurrentTicks(void)
{
   LastTimeSec = current_us / 1000000;
   LastTimeUsec = current_us % 1000000;
   NewMacDateInSeconds = TimeDelta + LastTimeSec;
}

#define MyInvTimeStep 16626 /* TicksPerSecond / 60.14742 */

LOCALVAR ui5b NextTimeSec;
LOCALVAR ui5b NextTimeUsec;

LOCALPROC IncrNextTime(void)
{
   NextTimeUsec += MyInvTimeStep;
   if (NextTimeUsec >= TicksPerSecond)
   {
      NextTimeUsec -= TicksPerSecond;
      NextTimeSec += 1;
   }
}

LOCALPROC InitNextTime(void)
{
	NextTimeSec = LastTimeSec;
	NextTimeUsec = LastTimeUsec;
	IncrNextTime();
}

LOCALPROC StartUpTimeAdjust(void)
{
	GetCurrentTicks();
	InitNextTime();
}

LOCALFUNC si5b GetTimeDiff(void)
{
	return ((si5b)(LastTimeSec - NextTimeSec)) * TicksPerSecond
		+ ((si5b)(LastTimeUsec - NextTimeUsec));
}

LOCALPROC UpdateTrueEmulatedTime(void)
{
	si5b TimeDiff;

	GetCurrentTicks();

	TimeDiff = GetTimeDiff();
	if (TimeDiff >= 0)
   {
		if (TimeDiff > 4 * MyInvTimeStep)
      {
         /* emulation interrupted, forget it */
         ++TrueEmulatedTime;
         InitNextTime();
      }
      else
      {
			do {
				++TrueEmulatedTime;
				IncrNextTime();
				TimeDiff -= TicksPerSecond;
			} while (TimeDiff >= 0);
		}
	}
   else if (TimeDiff < - 2 * MyInvTimeStep)
   {
		/* clock goofed if ever get here, reset */
		InitNextTime();
	}
}

LOCALFUNC blnr CheckDateTime(void)
{
   if (CurMacDateInSeconds != NewMacDateInSeconds)
   {
      CurMacDateInSeconds = NewMacDateInSeconds;
      return trueblnr;
   }
   return falseblnr;
}

LOCALFUNC blnr InitLocationDat(void)
{
	GetCurrentTicks();
	CurMacDateInSeconds = NewMacDateInSeconds;

	return trueblnr;
}

/* --- sound --- */

#if MySoundEnabled

#define kLn2SoundBuffers 4 /* kSoundBuffers must be a power of two */
#define kSoundBuffers (1 << kLn2SoundBuffers)
#define kSoundBuffMask (kSoundBuffers - 1)

#define DesiredMinFilledSoundBuffs 3
	/*
		if too big then sound lags behind emulation.
		if too small then sound will have pauses.
	*/

#define kLnOneBuffLen 9
#define kLnAllBuffLen (kLn2SoundBuffers + kLnOneBuffLen)
#define kOneBuffLen (1UL << kLnOneBuffLen)
#define kAllBuffLen (1UL << kLnAllBuffLen)
#define kLnOneBuffSz (kLnOneBuffLen + kLn2SoundSampSz - 3)
#define kLnAllBuffSz (kLnAllBuffLen + kLn2SoundSampSz - 3)
#define kOneBuffSz (1UL << kLnOneBuffSz)
#define kAllBuffSz (1UL << kLnAllBuffSz)
#define kOneBuffMask (kOneBuffLen - 1)
#define kAllBuffMask (kAllBuffLen - 1)
#define dbhBufferSize (kAllBuffSz + kOneBuffSz)

LOCALVAR tpSoundSamp TheSoundBuffer = nullpr;
LOCALVAR ui4b ThePlayOffset;
LOCALVAR ui4b TheFillOffset;
LOCALVAR ui4b TheWriteOffset;
LOCALVAR ui4b MinFilledSoundBuffs;

LOCALPROC MySound_Start0(void)
{
	/* Reset variables */
	ThePlayOffset = 0;
	TheFillOffset = 0;
	TheWriteOffset = 0;
	MinFilledSoundBuffs = kSoundBuffers;
}

GLOBALFUNC tpSoundSamp MySound_BeginWrite(ui4r n, ui4r *actL)
{
   ui4b ToFillLen       = kAllBuffLen - (TheWriteOffset - ThePlayOffset);
   ui4b WriteBuffContig =
      kOneBuffLen - (TheWriteOffset & kOneBuffMask);

   if (WriteBuffContig < n)
      n = WriteBuffContig;
   if (ToFillLen < n)
   {
      /* overwrite previous buffer */
      TheWriteOffset -= kOneBuffLen;
   }

   *actL = n;
   return TheSoundBuffer + (TheWriteOffset & kAllBuffMask);
}

LOCALVAR blnr wantplaying = falseblnr;

GLOBALPROC MySound_EndWrite(ui4r actL)
{
   TheWriteOffset += actL;

   if (0 == (TheWriteOffset & kOneBuffMask))
   {
      /* just finished a block */

      if (wantplaying)
      {
         TheFillOffset = TheWriteOffset;
         retro_sound();			

      }
      else if (((TheWriteOffset - ThePlayOffset) >> kLnOneBuffLen) < 12)
      {
         /* just wait */
      }
      else
      {
         TheFillOffset = TheWriteOffset;
         wantplaying = trueblnr;
         retro_sound();			
      }
   }

}

blnr MySound_EndWrite0(ui4r actL)
{
   blnr v;

   TheWriteOffset += actL;

   if (0 != (TheWriteOffset & kOneBuffMask))
      v = falseblnr;
   else
   {
      /* just finished a block */

      TheFillOffset = TheWriteOffset;

      v = trueblnr; //retro_sound();	
   }

   return v;
}

LOCALPROC MySound_SecondNotify0(void)
{
   if (MinFilledSoundBuffs > DesiredMinFilledSoundBuffs)
      ++CurEmulatedTime;
   else if (MinFilledSoundBuffs < DesiredMinFilledSoundBuffs)
      --CurEmulatedTime;
   MinFilledSoundBuffs = kSoundBuffers;
}

#define SOUND_SAMPLERATE 22255 /* = round(7833600 * 2 / 704) */

#endif

void retro_sound(void)
{
#if MySoundEnabled
   ui3p NextPlayPtr;
   ui4b PlayNowSize = 0;
   ui4b MaskedFillOffset = ThePlayOffset & kOneBuffMask;

   if (MaskedFillOffset != 0)
   {
      /* take care of left overs */
      PlayNowSize = kOneBuffLen - MaskedFillOffset;
      NextPlayPtr = TheSoundBuffer + (ThePlayOffset & kAllBuffMask);
   }
   else if (0 != ((TheFillOffset - ThePlayOffset) >> kLnOneBuffLen))
   {
      PlayNowSize = kOneBuffLen;
      NextPlayPtr = TheSoundBuffer + (ThePlayOffset & kAllBuffMask);
   }

   if (0 != PlayNowSize)
   {
      int i;

      unsigned char * sptr=NextPlayPtr;
      signed short rsnd=0;

      for(i=0;i<PlayNowSize;i++)
      {
         rsnd=(signed short int) ( ((int)(*sptr++))-128)<<8;

         if(rsnd!=0)retro_audio_cb(rsnd,rsnd);
      }

      ThePlayOffset += PlayNowSize;		

   }
#endif
}

/* --- basic dialogs --- */

LOCALPROC CheckSavedMacMsg(void)
{
   /* called only on quit, if error saved but not yet reported */
   if (nullpr != SavedBriefMsg)
   {
      char briefMsg0[ClStrMaxLength + 1];
      char longMsg0[ClStrMaxLength + 1];

      NativeStrFromCStr(briefMsg0, SavedBriefMsg);
      NativeStrFromCStr(longMsg0, SavedLongMsg);

      fprintf(stderr, "%s\n", briefMsg0);
      fprintf(stderr, "%s\n", longMsg0);

      SavedBriefMsg = nullpr;
   }
}

/* --- clipboard --- */
#undef IncludeHostTextClipExchange

GLOBALFUNC tMacErr HTCEexport(tPbuf i){}
GLOBALFUNC tMacErr HTCEimport(tPbuf *r){}

//#define UseMotionEvents 1

#if UseMotionEvents
LOCALVAR blnr CaughtMouse = falseblnr;
#endif

#if MayNotFullScreen
LOCALVAR int SavedTransH;
LOCALVAR int SavedTransV;
#endif

LOCALVAR int my_argc;
LOCALVAR char **my_argv;

LOCALVAR char *display_name = NULL;

LOCALFUNC blnr Screen_Init(void)
{
	ColorModeWorks = trueblnr;

	return 1;
}

enum
{
   kMagStateNormal,

   kNumMagStates
};

#define kMagStateAuto kNumMagStates

LOCALVAR blnr WantRestoreCursPos = falseblnr;
LOCALVAR ui4b RestoreMouseH;
LOCALVAR ui4b RestoreMouseV;

#if VarFullScreen && EnableMagnify
enum
{
   kWinStateWindowed,
   kNumWinStates
};
#endif

#if VarFullScreen && EnableMagnify
LOCALVAR int WinMagStates[kNumWinStates];
#endif

#if VarFullScreen
LOCALPROC ToggleWantFullScreen(void)
{
	WantFullScreen = ! WantFullScreen;

}
#endif

LOCALPROC CheckForSavedTasks(void)
{
}

/* --- command line parsing --- */

LOCALFUNC blnr ScanCommandLine(void)
{
   char *pa;
   int i = 1;

label_retry:
   if (i < my_argc) {
      pa = my_argv[i++];
      if ('-' == pa[0]) {
         if ((0 == strcmp(pa, "--display"))
               || (0 == strcmp(pa, "-display")))
         {
            if (i < my_argc) {
               display_name = my_argv[i++];
               goto label_retry;
            }
         } else
            if ((0 == strcmp(pa, "--rom"))
                  || (0 == strcmp(pa, "-r")))
            {
               if (i < my_argc) {
                  rom_path = my_argv[i++];
                  goto label_retry;
               }
            } else
               if (0 == strcmp(pa, "-n"))
               {
                  if (i < my_argc) {
                     n_arg = my_argv[i++];
                     goto label_retry;
                  }
               } else
                  if (0 == strcmp(pa, "-d"))
                  {
                     if (i < my_argc) {
                        d_arg = my_argv[i++];
                        goto label_retry;
                     }
                  } else
#ifndef UsingAlsa
#define UsingAlsa 0
#endif

#if UsingAlsa
                     if ((0 == strcmp(pa, "--alsadev"))
                           || (0 == strcmp(pa, "-alsadev")))
                     {
                        if (i < my_argc) {
                           alsadev_name = my_argv[i++];
                           goto label_retry;
                        }
                     } else
#endif
#if 0
                        if (0 == strcmp(pa, "-l")) {
                           SpeedValue = 0;
                           goto label_retry;
                        } else
#endif
                        {
                           MacMsg(kStrBadArgTitle, kStrBadArgMessage, falseblnr);
                        }
      } else {
         (void) Sony_Insert1(pa, falseblnr);
         goto label_retry;
      }
   }

   return trueblnr;
}

/* --- main program flow --- */

//LOCALVAR ui5b OnTrueTime = 0;

GLOBALFUNC blnr ExtraTimeNotOver(void)
{
	UpdateTrueEmulatedTime();
	return TrueEmulatedTime == OnTrueTime;
}

/* --- platform independent code can be thought of as going here --- */

#include "PROGMAIN.h"

LOCALPROC RunEmulatedTicksToTrueTime(void)
{
   si3b n = OnTrueTime - CurEmulatedTime;

   if (n > 0)
   {
      if (CheckDateTime())
      {
#if MySoundEnabled
         //MySound_SecondNotify();
#endif
      }

      //if ((! gBackgroundFlag)
#if UseMotionEvents
      //	&& (! CaughtMouse)
#endif
      //		)
      {
         //CheckMouseState();
      }

      DoEmulateOneTick();
      ++CurEmulatedTime;

#if EnableMouseMotion && MayFullScreen
      if (HaveMouseMotion) {
         AutoScrollScreen();
      }
#endif
      //MyDrawChangesAndClear();

      if (n > 8) {
         /* emulation not fast enough */
         n = 8;
         CurEmulatedTime = OnTrueTime - n;
      }

      if (ExtraTimeNotOver() && (--n > 0)) {
         /* lagging, catch up */

         EmVideoDisable = trueblnr;

         do {
            DoEmulateOneTick();
            ++CurEmulatedTime;
         } while (ExtraTimeNotOver()
               && (--n > 0));

         EmVideoDisable = falseblnr;
      }

      EmLagTime = n;
   }
}

int  RunOnEndOfSixtieth(void)
{
   while (ExtraTimeNotOver())
   {
      struct timespec rqt;
      struct timespec rmt;

      si5b TimeDiff = GetTimeDiff();
      if (TimeDiff < 0)
      {
         rqt.tv_sec = 0;
         rqt.tv_nsec = (- TimeDiff) * 1000;
         (void) nanosleep(&rqt, &rmt);
      }
   }

   OnTrueTime = TrueEmulatedTime;
   RunEmulatedTicksToTrueTime();
}

LOCALPROC ReserveAllocAll(void)
{
#if dbglog_HAVE
   dbglog_ReserveAlloc();
#endif
   ReserveAllocOneBlock(&ROM, kROM_Size, 5, falseblnr);

   ReserveAllocOneBlock(&screencomparebuff,
         vMacScreenNumBytes, 5, trueblnr);
#if UseControlKeys
   ReserveAllocOneBlock(&CntrlDisplayBuff,
         vMacScreenNumBytes, 5, falseblnr);
#endif
#if WantScalingBuff
   ReserveAllocOneBlock(&ScalingBuff,
         ScalingBuffsz, 5, falseblnr);
#endif
#if 1//WantScalingTabl
#define ScalingTablsz 256
   ReserveAllocOneBlock(&ScalingTabl,
         ScalingTablsz, 5, falseblnr);
#endif

#if MySoundEnabled
   ReserveAllocOneBlock((ui3p *)&TheSoundBuffer,
         dbhBufferSize, 5, falseblnr);
#endif

   EmulationReserveAlloc();
}

LOCALFUNC blnr AllocMyMemory(void)
{
   uimr n;
   blnr IsOk = falseblnr;

   ReserveAllocOffset = 0;
   ReserveAllocBigBlock = nullpr;
   ReserveAllocAll();
   n = ReserveAllocOffset;
   ReserveAllocBigBlock = (ui3p)calloc(1, n);
   if (NULL == ReserveAllocBigBlock)
      MacMsg(kStrOutOfMemTitle, kStrOutOfMemMessage, trueblnr);
   else
   {
      ReserveAllocOffset = 0;
      ReserveAllocAll();
      if (n != ReserveAllocOffset)
      { 
         /* oops, program error */
      }
      else
         IsOk = trueblnr;
   }

   return IsOk;
}

LOCALPROC UnallocMyMemory(void)
{
   if (nullpr != ReserveAllocBigBlock)
      free((char *)ReserveAllocBigBlock);
}

blnr InitOSGLU(int argc, char **argv)
{
   my_argc = argc;
   my_argv = argv;

   InitDrives();
   if (AllocMyMemory())

#if dbglog_HAVE
      if (dbglog_open())
#endif
         if (ScanCommandLine())
            if (LoadMacRom())
               if (Screen_Init())
                  if (InitEmulation())
                     return trueblnr;
   return falseblnr;
}

int  UnInitOSGLU(void)
{
   if (MacMsgDisplayed)
      MacMsgDisplayOff();

#if IncludePbufs
   UnInitPbufs();
#endif
   UnInitDrives();


#if dbglog_HAVE
   dbglog_close();
#endif

   UnallocMyMemory();

   CheckSavedMacMsg();
}

