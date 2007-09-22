//#########################################################################
// File: dir.h
//
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 12.11.2006
//#########################################################################
// Compiler: AVR-GCC 3.4.5
//#########################################################################

#ifndef __DIR_H
#define __DIR_H

extern unsigned char Mkdir(char *name);
extern unsigned char Chdir(char *name);
extern unsigned char MakeNewFileEntry(void);
extern unsigned char UpdateFileEntry(void);
extern unsigned char ScanRootDir(char *name);
extern unsigned char ScanOneDirectorySector(unsigned long sector, char *name);
extern unsigned char SearchRootDir(void);

#ifdef USE_FAT32
 extern unsigned char SearchSubDir(unsigned long cluster);
 extern unsigned char ScanSubDir(unsigned long startcluster, char *name);
#else
 extern unsigned char SearchSubDir(unsigned int cluster);
 extern unsigned char ScanSubDir(unsigned int startcluster, char *name);
#endif

extern unsigned char SearchDirSector(unsigned long sector);
extern void MakeFileName(char *inname, char *outname, char *outext);

#endif //__DIR_H
