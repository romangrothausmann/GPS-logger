//#########################################################################
// File: DOSDEFS.H
//
// Central DOS configuration file for this project
//
//#########################################################################
// Last change: 22.10.2006
//#########################################################################
// Compiler: AVR-GCC 3.4.5
//#########################################################################

#ifndef __DOSDEFS_H
#define __DOSDEFS_H

// spare program memory by deciding if we want to read, write or both
//#undef DOS_READ	//undefine this if you don't want to read files with Fread()
//#undef DOS_WRITE	//undefine this if you don't want to write files with Fwrite()
                        //deleting files is also deactivated
#undef DOS_DELETE	//undefine this if you don't want to delete files
#undef DOS_READ_RAW	//undefine this if you don't want to read files with ReadFileRaw()

#undef DOS_MKDIR	//undefine this if you don't want to make subdirectories
#undef DOS_CHDIR	//undefine this if you don't want to go into subdirectories

// spare program memory by deciding if we want to use FAT12, FAT16, FAT32.
// NOT recommended if you don't know the FAT type of your drive !
//#undef USE_FAT12	//undefine this if you don't want to use FAT12
//#undef USE_FAT16	//undefine this if you don't want to use FAT16
//#undef USE_FAT32	//undefine this if you don't want to use FAT32

#undef USE_FATBUFFER	//undefine this if you don't want to use a FAT buffer
                        //needs 517 Bytes of RAM ! 

//#undef USE_FINDFILE     //undefine this if you don't want to use Findfirst(), Findnext()
#undef USE_FINDLONG     //undefine this if you don't want to get long filenames
			//from Findfirst(); Findnext();


#endif //__DOSDEFS_H
