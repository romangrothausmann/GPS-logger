//#########################################################################
// File: find_x.h
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 12.11.2006
//#########################################################################
// Compiler: AVR-GCC 3.4.3
//#########################################################################

#ifndef __FINDX_H
#define __FINDX_H

#define _MAX_NAME	40		// Max. length of long filenames + 1.
					// This should be 256, but i dont want
					// to use an unsigned int.
					// Maybe 128 or 64 Bytes are also enough
        				// for a microcontroller DOS.
        				// Change it here if you have problems
        				// with free RAM.

struct FindFile
{
 	unsigned char ff_attr;		// file attributes like file, dir
 					// long name ,hidden,system and readonly flags are ignored
 	unsigned long ff_fsize;		// filesize of the file ( not directory ! )
	char ff_name[13];		// 8.3 DOS filename with '.' in it and \0 at the end for fopen()
#ifdef USE_FINDLONG
        char ff_longname[_MAX_NAME];	// The very long filename.
#endif
#ifdef USE_FAT32
 	unsigned long newposition;	// position of this file entry in current directory (1 means first file)
 	unsigned long lastposition;	// position of last file entry found in current directory (1 means first file)
#else
 	unsigned int newposition;	// position of this file entry in current directory (1 means first file)
 	unsigned int lastposition;	// position of last file entry found in current directory (1 means first file)
 					// does also count ".", ".." entrys !
 					// does not count long filename entrys and volume id
#endif
};

extern struct FindFile ffblk;

extern unsigned char Findfirst(void);
extern unsigned char Findnext(void);

#endif //__FINDX_H
