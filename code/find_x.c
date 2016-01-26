//###########################################################
// File: find_x.c
//
// Find first file in a directory and give back filename as
// a NULL terminated string. Also fileattr and filesize.
//
// ffblk.ff_name[]   	8.3 DOS name with '.' in it and \0 at the end
// ffblk.ff_longname[]  Long filename with \0 at the end
// ffblk.ff_attr     	ATTR_FILE or ATTR_DIRECTORY
// ffblk.ff_fsize    	Filesize, 0 if directory
//
// Use this data to find next file, next file, ... in a directory.
//
// Necessary to make a "dir" or "ls" command. Or opening files
// with fopen() without knowing the filename of the first,next,next... file.
//
// This doesn't work without initialized global variable FirstDirCluster
// which points to the current directory.
//
// 12.11.2006 Most code simply merged into dir.c
//
// 26.12.2005 Found a bug in constructing the long name. If it had exactly
//            13 Bytes, parts of the last findnext() are in ffblk.ff_longname. 
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 12.11.2006
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: AVR-GCC 3.4.5
//#########################################################################
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dos.h"

#ifdef USE_FINDFILE

struct FindFile ffblk; 	//make this global to avoid parameter passing


//###########################################################
// finds the first file or dir entry in current directory
// returns 0 if no file is found
// returns 1 if a file is found
//
// check ffblk.ff_attr if you have a file or a dir entry
//
unsigned char Findfirst(void)
//###########################################################
{
 unsigned char i;
 
 //delete last data
 for(i=0; i<13; i++) ffblk.ff_name[i]=0; //clean last filename

 ffblk.ff_fsize=0; 	      		//no filesize
 ffblk.ff_attr=ATTR_NO_ATTR; 	      	//no file attr
 ffblk.newposition=0;                 	//no position
 ffblk.lastposition=0;                	//no position
 ffblk.ff_name[8]='.';                  //insert dot between filename and fileextension
 
 return Findnext();
}

//###########################################################
// finds the next file or dir entry in current directory
// returns 0 if no next file is found
// returns 1 if a next file is found
//
// always starts to search from the beginning of a directory !
//
// check ffblk.f_attr if you have a file or a dir entry
//
// NEVER call this before calling findfirst()
// your program crashes and your hardware will be destroyed
//
unsigned char Findnext(void)
//###########################################################
{
#ifdef USE_FINDLONG
 unsigned char i;
#endif

 unsigned char result;

 ffblk.newposition=0;                 	//no position for next search

#ifdef USE_FINDLONG
 for(i=0; i<_MAX_NAME; i++) ffblk.ff_longname[i]=0;	// clean the long filename.
#endif

 if(FirstDirCluster<2) //are we in rootdirectory ?
  {
   result=ScanRootDir(NULL);
  }
 else                  //we are in a subdirectory
  {
   result=ScanSubDir(FirstDirCluster,NULL);
  }

 if(result==FULL_MATCH) return 1;
 else return 0;
}

#endif // #ifdef USE_FINDFILE


