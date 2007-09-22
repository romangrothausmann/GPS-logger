//#########################################################################
// File: dos.h
//
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 25.10.2006
//#########################################################################
// Compiler: AVR-GCC 3.4.5
//#########################################################################

#ifndef __DOS_H
#define __DOS_H

#define SMALL_FWRITE  // less code but slower
//#define FAST_FWRITE     // more code but faster

#define SMALL_FREAD   // less code but slower
//#define FAST_FREAD      // more code but faster

// security check !
#if defined (SMALL_FWRITE) && defined (FAST_FWRITE)
 #error "Define SMALL_FWRITE or FAST_FWRITE only in dos.h, NOT both !"
#endif

#if !defined (SMALL_FWRITE) && !defined (FAST_FWRITE)
 #error "Define at least SMALL_FWRITE or FAST_FWRITE in dos.h !"
#endif

#if defined (SMALL_FREAD) && defined (FAST_FREAD)
 #error "Define SMALL_FREAD or FAST_FREAD only in dos.h, NOT both !"
#endif

#if !defined (SMALL_FREAD) && !defined (FAST_FREAD)
 #error "Define at least SMALL_FREAD or FAST_FREAD in dos.h !"
#endif

//fopen flags
#define F_CLOSED	0
#define F_READ		1
#define F_WRITE		2

#define F_ERROR		0 // dir/file operation failed
#define F_OK		1 // dir/file operation successfull

// #undef defines below in "mydefs.h" if you don't need them
// spare program memory by deciding if you want to read, write or both
#define DOS_READ	//define this if you want to read files
#define DOS_WRITE	//define this if you want to write files
#define DOS_REMOVE	//define this if you want to delete files
#define DOS_READ_RAW	//define this if you want to read files with ReadFileRaw()

#define DOS_CHDIR	//define this if you want to go into subdirectories
#define DOS_MKDIR	//define this if you want to make subdirectories

// spare program memory by deciding if we want to use FAT12, FAT16, FAT32.
// don't touch if you don't know the FAT type of your drive !
#define USE_FAT12	//define this if you want to use FAT12
#define USE_FAT16	//define this if you want to use FAT16
#define USE_FAT32	//define this if you want to use FAT32

#define USE_FATBUFFER	//define this if you want to use a FAT buffer
                        //needs 517 Bytes of RAM ! 

#define USE_FINDFILE	//define this if you want to use findfirst(); findnext();
#define USE_FINDLONG    //define this if you want to get long filenames
			//from findfirst(); findnext();

#define USE_DRIVEFREE   //define this if you want to get free and used space of your drive

#include "media.h"   // select flash medium (CF, MMC, SD, ...)
//#include "mydefs.h"  // keep the line at this place ! don't move down or delete 
#include "dosdefs.h"  // keep the line at this place ! don't move down or delete 

extern unsigned char Fopen(char *name, unsigned char flag);
extern void Fclose(void);

#ifdef DOS_READ
extern unsigned int Fread(unsigned char *buf, unsigned int count);
extern unsigned char Fgetc(void);
#endif

#ifdef DOS_WRITE	//define this if you want to write files
extern unsigned int Fwrite(unsigned char *buf, unsigned int count);
extern unsigned int Fputc(unsigned char buf);
#endif

extern void Fflush(void);
extern unsigned char Fremove(char *name);

extern unsigned char ReadFileRaw(char *name);
extern unsigned char FindName(char *name);

#define Filelength()	FileSize

//this is for easier and faster converting from byte arrays to UINT, ULONG
//ui and ul share the same memory space
union Convert {
 unsigned int  ui;
 unsigned long ul;
};

#ifdef COMPACTFLASH_CARD
 #include "compact.h"
#endif

#ifdef MMC_CARD_SPI
 #include "mmc_spi.h"
#endif

#include "fat.h"
#include "dir.h"

#ifdef USE_FINDFILE
 #include "find_x.h"
#endif

#ifdef USE_DRIVEFREE
 #include "drivefree.h"
#endif

#endif //__DOS_H
