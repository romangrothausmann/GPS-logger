//###########################################################
// File: dir.c
//
// Directory functions
//
// For FAT12, FAT16 and FAT32
// Only for first Partition
// Only for drives with 512 bytes per sector (the most)
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
// 20.11.2006 Bugfix. If WIN makes a directory for FAT32 and directory
//            is in Root directory, upperdir cluster is ZERO !
//
//#########################################################################
// Last change: 20.11.2006
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

#ifdef DOS_WRITE
#ifdef DOS_MKDIR
//###########################################################
// Make a new directory
unsigned char Mkdir(char *name)
//###########################################################
{
#ifdef USE_FAT32
 unsigned long newdircluster, upperdircluster;
#else
 unsigned int newdircluster, upperdircluster;
#endif

 unsigned long newdirsector;
 unsigned int i;
 
 if(FileFlag!=0) return F_ERROR; // no open file allowed here

 //Test if directoryname exists in currentdir
 //This worked as a chdir() if directory exists !
 // if(FindName(name)==FULL_MATCH) return F_OK;
 if(FindName(name)==FULL_MATCH) { FindName(".."); return F_OK; }


//from this point i use some variables of FILE entry !
//you have to change a lot of things if you want to use more then ONE file !

 FileAttr=ATTR_DIRECTORY;  //want to make a directory !
 MakeFileName(name,FileName,FileExt);
 if(MakeNewFileEntry())
  {
   //MakeNewFileEntry() allocates first cluster for new directory !
   //entry of new directory is written to currentdir sector
   //first cluster of new directory is returned in FileFirstCluster

   newdircluster=FileFirstCluster; 
   newdirsector=GetFirstSectorOfCluster(newdircluster);
   FileDirSector=newdirsector;

   //clean all sectors of newdir cluster (fill with 0x00)
   for(i=0; i<BYTE_PER_SEC; i++) iob[i]=0; //Fill write buffer

   for(i=0; i<secPerCluster; i++)
    {
     WriteSector(newdirsector,iob);
     newdirsector++;
    }
   //end clean all sectors of newdir cluster (fill with 0x00)

   upperdircluster=FirstDirCluster; //first upperdir cluster needed for later

   //insert "." my new dir entry with newdir firstcluster
   FileDirOffset=0; //first direntry "."
   MakeFileName(".",FileName,FileExt);
   FileFirstCluster=newdircluster;
   UpdateFileEntry();

   //insert ".." upper dir entry with upperdir firstcluster
   FileDirOffset=1;  //2nd direntry ".."
   MakeFileName("..",FileName,FileExt);
   FileFirstCluster=upperdircluster;
   UpdateFileEntry();

#ifdef USE_FATBUFFER
     WriteSector(FATCurrentSector,fatbuf); // write the FAT buffer
     FATStatus=0;
#endif
  }
 else
  {
   FileAttr=ATTR_FILE; //default
   return F_ERROR; //new dir could not be made
  } 
 
 FileAttr=ATTR_FILE; //default
 return F_OK;
}
#endif //DOS_MKDIR
#endif //DOS_WRITE
       
#ifdef DOS_CHDIR
//###########################################################
// Change to a directory
unsigned char Chdir(char *name)
//###########################################################
{
 if(FileFlag!=0) return F_ERROR; // no open file allowed here

 if(name[0]=='/')
  {
#ifdef USE_FAT12
   if(FATtype==FAT12) FirstDirCluster=0;
#endif
#ifdef USE_FAT16
   if(FATtype==FAT16) FirstDirCluster=0;
#endif
#ifdef USE_FAT32
   if(FATtype==FAT32) FirstDirCluster=FAT32RootCluster;
#endif
   return F_OK;
  }
 
 if(FindName(name)==FULL_MATCH) return F_OK;
 else return F_ERROR;
}
#endif //DOS_CHDIR

#ifdef DOS_WRITE
//###########################################################
unsigned char MakeNewFileEntry(void)
//###########################################################
{
 unsigned long tmpsector;

#ifdef USE_FAT32
 unsigned long tmpcluster,lastdircluster;
#else
 unsigned int tmpcluster,lastdircluster;
#endif

 unsigned int i;
 unsigned char result;
 
 // search for a free direntry
 if(FirstDirCluster<2) result=SearchRootDir();
 else result=SearchSubDir(FirstDirCluster);

 if(result==0) // no free direntry found. lets try to alloc and add a new dircluster
  {
   //if dir is rootdir (FAT12/16 only) you have a problem ;)
   if(FirstDirCluster<2 && (FATtype==FAT12 || FATtype==FAT16) )
    {
     return F_ERROR;
    }

   //search the last cluster of directory
   lastdircluster=FirstDirCluster;
   do
    {
     tmpcluster=GetNextClusterNumber(lastdircluster);
     if(tmpcluster < endofclusterchain) lastdircluster=tmpcluster;
    }while(tmpcluster < endofclusterchain);
   
   tmpcluster=AllocCluster(lastdircluster); //if currentdir is full alloc new cluster for dir
   if(tmpcluster==DISK_FULL) //no free clusters ?
    {
     return F_ERROR;
    }

   //fill all cluster sectors with zero
   for(i=0; i<BYTE_PER_SEC; i++) iob[i]=0; //Fill write buffer

   tmpsector=GetFirstSectorOfCluster(tmpcluster);
   for(i=0; i<secPerCluster; i++)
    {
     WriteSector(tmpsector,iob);
     tmpsector++;
    }
   
   FileDirOffset=0;              //set offset for new direntry in dirsector
   FileDirSector=GetFirstSectorOfCluster(tmpcluster);
  }     

 tmpcluster=AllocCluster(0); //alloc first cluster for file
 if(tmpcluster==DISK_FULL) //no free clusters ?
  {
   return F_ERROR;
  }

 FileFirstCluster=tmpcluster;
 FileSize=0;
 UpdateFileEntry(); //write new file entry

 return F_OK; //all ok 
}
#endif //DOS_WRITE

#ifdef DOS_WRITE
//###########################################################
unsigned char UpdateFileEntry(void)
//###########################################################
{
 struct DirEntry *di;
 
 ReadSector(FileDirSector,iob);
 di=(struct DirEntry *)&iob[FileDirOffset*32];

 strncpy(di->DIR_Name,FileName,8);
 strncpy(di->DIR_Ext,FileExt,3);

 di->DIR_Attr=FileAttr;
 di->DIR_NTres=0;
 di->DIR_CrtTimeTenth=0;

//because i have no clock give file a fixed time
 di->DIR_CrtTime= ((unsigned int)19<<11) + ((unsigned int)21<<5);      //creation time
 di->DIR_CrtDate= ((unsigned int)23<<9) + ((unsigned int)5<<5) + 10;  //creation date
 di->DIR_LastAccDate= ((unsigned int)23<<9) + ((unsigned int)5<<5) + 10;  //last access date 10.05.2003
 di->DIR_WrtTime= ((unsigned int)19<<11) + ((unsigned int)21<<5); //last write time 19:21
 di->DIR_WrtDate= ((unsigned int)23<<9) + ((unsigned int)5<<5) + 10; //last write date 10.05.2003

#ifdef USE_FAT32
 di->DIR_FstClusHI=(unsigned int)(FileFirstCluster>>16);  //first cluster high word                 
#else
 di->DIR_FstClusHI=0;  //first cluster high word                 
#endif
 di->DIR_FstClusLO=(unsigned int)(FileFirstCluster);  //first cluster low word                 
 di->DIR_FileSize=FileSize;

 WriteSector(FileDirSector,iob);
 return F_OK;
}
#endif //DOS_WRITE

//###########################################################
//search root dir for free dir entry
unsigned char SearchRootDir(void)
//###########################################################
{
// unsigned long i;
 unsigned int i;
 unsigned char result;

 result=0; // do we need this ?
  
 for(i=0; i<RootDirSectors; i++)
  {
   result=SearchDirSector(FirstRootSector+i);
   if(result!=0) break; //break sector loop
  }

 return result;
}

//###########################################################
//search sub dir for free dir entry
#ifdef USE_FAT32
 unsigned char SearchSubDir(unsigned long startcluster)
#else
 unsigned char SearchSubDir(unsigned int startcluster)
#endif
//###########################################################
{
 unsigned long tmpsector;

#ifdef USE_FAT32
 unsigned long tmpcluster;
#else
 unsigned int tmpcluster;
#endif

 unsigned char i,result;
 
 tmpcluster=startcluster;
 result=0;
 
 while(tmpcluster < endofclusterchain)
  {
   tmpsector=GetFirstSectorOfCluster(tmpcluster);
   for(i=0; i<secPerCluster; i++)
    {
     result=SearchDirSector(tmpsector+i);
     if(result!=0) break; //break sector loop
    } 

   if(result!=0) break; //break cluster loop

   tmpcluster=GetNextClusterNumber(tmpcluster);
  }

 return result;
}

//###########################################################
//search dir sector for free dir entry
unsigned char SearchDirSector(unsigned long sector)
//###########################################################
{
 unsigned int count;

 ReadSector(sector,iob); //read one directory sector.
 count=0;
 do
  {
   if(iob[count]==0 || iob[count]==0xE5)
    {
     FileDirSector=sector; //keep some values in mind
     FileDirOffset=count/32;
     return 1;
    }

   count+=32;
  }while(count<BYTE_PER_SEC);

 return 0;
}

//###########################################################
// Change to dir or find a filename (if name!=NULL)
//
// Following global variables will be set if dir or filename
// was found:
//
// For directories:
// FirstDirCluster   First cluster of directory
//
// For files:
// FileName          8 chars for filename
// FileExt           3 chars for extension
// FileDirSector     Sector which keeps direntry of the file
// FileDirOffset     Offset to direntry of the file
// FileFirstCluster  First cluster of the dir/file in FAT 
// FileSize
unsigned char ScanOneDirectorySector(unsigned long sector, char *name)
//###########################################################
{
 unsigned char by;
 unsigned int count;
 unsigned long tmp;
 struct DirEntry *di;
 struct DirEntryBuffer *dib;
 char nam[8];
 char ext[3];
 unsigned char match;

 if(name) //find filename or directory name
  {
   MakeFileName(name,nam,ext);
  }//if(name)
 
 by=ReadSector(sector,iob); //read one directory sector.
 count=0;
 do
  {
   match=NO_MATCH;
   di=(struct DirEntry *)(&iob[count]);
   //make a second pointer to iob for easier access to long filename entrys
   dib=(struct DirEntryBuffer *)di;
   
   if(di->DIR_Name[0]==0) return END_DIR; //end of directory
      
   if(di->DIR_Name[0]!=0xE5) // Deleted entry ?
    {
     di->DIR_Attr&=0x3F;            //smash upper two bits

     if(di->DIR_Attr==ATTR_LONG_NAME)
      {
#ifdef USE_FINDFILE
#ifdef USE_FINDLONG
       if(name==NULL) // FIND_OPERATION
        {
         // Build the long name from the 13 bytes long name direntrys.
         // The long name direntrys have to be in a block of direntrys.
         // Otherwise this will not work and you get strange results.
       
         if(ffblk.newposition >= ffblk.lastposition) //found a new file ?
          {
           unsigned char offset; //offset of the direntry in long name
           unsigned char count;  //loop counter
           unsigned char i;

           offset=dib->longchars[0];
           offset&=0x1F; //force upper bits D7..5 to zero.
                         //max. value of 20 is allowed here, or even less
                         //if _MAX_NAME is defined smaller than 255.
                         //long filenames will then be cut at _MAX_NAME - 1.

           if(offset>20) offset=20; //force maximum value if too big

           count=(offset-1) * 13; // calc start adress in long name array
       
           //We can not use strncpy() because a CHAR in the long name
           //direntry has two bytes ! 2nd byte is ignored here.
       
           for(i=1; i<=9; i+=2)
            {
             by=dib->longchars[i];
             if(count<_MAX_NAME) ffblk.ff_longname[count]=by;
             count++;
            }

           for(i=14; i<=24; i+=2)
            {
             by=dib->longchars[i];
             if(count<_MAX_NAME) ffblk.ff_longname[count]=by;
             count++;
            }

           for(i=28; i<=30; i+=2)
            {
             by=dib->longchars[i];
             if(count<_MAX_NAME) ffblk.ff_longname[count]=by;
             count++;
            }

           ffblk.ff_longname[_MAX_NAME-1]=0; //End of string to avoid buffer overruns 

          }//if(ffblk.newposition >= ffblk.lastposition)
        } //if(name==NULL)
#endif //#ifdef USE_FINDLONG
#endif //#ifdef USE_FINDFILE       
      }
     else
      {
       if(name==NULL) //List only
        {
        }//if(name==NULL)
       else //searching for a filename or a directory name
        {
         if(strncmp(nam,di->DIR_Name,8)==0) match=MATCH_NAME;
         if(strncmp(ext,di->DIR_Ext,3)==0) match+=MATCH_EXT;
        }
                
       if(di->DIR_Attr & ATTR_VOLUME_ID)
        {         //nothing to do here. volume id not supported
        }
       else
        {
         // if we come here we have a file or a directory
         if(name==NULL)  //FIND_OPERATION
          {
#ifdef USE_FINDFILE
           ffblk.newposition++; //one more entry found
             
           if(ffblk.newposition > ffblk.lastposition) //found a new file ?
            {
             ffblk.lastposition=ffblk.newposition;    //save new file position
             strncpy(ffblk.ff_name,di->DIR_Name,8);   //copy filename
             strncpy(&ffblk.ff_name[9],di->DIR_Ext,3);//copy fileextension
             
             if(di->DIR_Attr & ATTR_DIRECTORY)
              {
               ffblk.ff_attr=ATTR_DIRECTORY;      //file attribute
               ffblk.ff_fsize=0; 		  //not a file, clear filesize
              }
             else
              {
               ffblk.ff_attr=ATTR_FILE;
               ffblk.ff_fsize=di->DIR_FileSize;
              }

             return FULL_MATCH; //found next entry, stop searching

            }//if(ffblk.newposition > ffblk.lastposition)
#endif//#ifdef USE_FINDFILE
          }
         else //DIR_OPERATION
          { 
           if(di->DIR_Attr & ATTR_DIRECTORY) //this is a directory
            {
             tmp=di->DIR_FstClusHI; //Highword of first cluster number
             tmp<<=16;
             tmp+=di->DIR_FstClusLO; //Lowword of first cluster number

             //we never come here if name==NULL
//             if(name==NULL) //List only
//              {
//              } 
//             else          //searching for a directoryname
              {
               if(match==FULL_MATCH)
                {
                 FirstDirCluster=tmp;

#ifdef USE_FAT32
// Special case for FAT32 and directories in ROOT directory MADE BY WIN.
// Upper directory ".." first cluster for a subdirectory is ZERO here, and NOT FAT32RootCluster !
                 if(FATtype==FAT32)
                  {
                   if(tmp < 2) FirstDirCluster=FAT32RootCluster; // force to correct cluster
                  }
#endif //#ifdef USE_FAT32

//                 FileAttr=ATTR_DIRECTORY;
                 return FULL_MATCH;
                } 
              }//if(name==NULL) 
            }//if(di->DIR_Attr & ATTR_DIRECTORY)
           else //is not a directory. this is a file
            {
             tmp=di->DIR_FstClusHI; //Highword of first cluster number
             tmp<<=16;
             tmp+=di->DIR_FstClusLO; //Lowword of first cluster number

             //we never come here if name==NULL
//             if(name==NULL) //List filenames only
//              {
//              }
//             else //searching for a filename
              {
               if(match==FULL_MATCH)
                {
                 strncpy(FileName,nam,8);
                 strncpy(FileExt,ext,3);
                 FileDirSector=sector; //keep some values in mind
                 FileDirOffset=count/32;
                 FileFirstCluster=tmp;
                 FileSize=di->DIR_FileSize;
                 FileAttr=ATTR_FILE;
                 return FULL_MATCH;
                } 
               } 

            }//if(di->DIR_Attr & ATTR_DIRECTORY)
          }
        }
      }
         
    }//if(di->DIR_Name[0]!=0xE5)
        
   count+=32;
  }while(count<BYTE_PER_SEC);

 return NO_MATCH;
}


//###########################################################
#ifdef USE_FAT32
 unsigned char ScanSubDir(unsigned long startcluster, char *name)
#else
 unsigned char ScanSubDir(unsigned int startcluster, char *name)
#endif
//###########################################################
{
 unsigned long tmpsector;
#ifdef USE_FAT32
 unsigned long tmpcluster;
#else
 unsigned int tmpcluster;
#endif

 unsigned char i,result;
 
 tmpcluster=startcluster;
 result=NO_MATCH;
 
 while(tmpcluster < endofclusterchain)
  {
   tmpsector=GetFirstSectorOfCluster(tmpcluster);
   for(i=0; i<secPerCluster; i++)
    {
     result=ScanOneDirectorySector(tmpsector + i, name);
     if(result!=NO_MATCH) break; //break sector loop
    } 

   if(result!=NO_MATCH) break; //break cluster loop
   tmpcluster=GetNextClusterNumber(tmpcluster);
  }

 return(result);
}

//###########################################################
//FAT12/16 only
unsigned char ScanRootDir(char *name)
//###########################################################
{
// unsigned long i;
 unsigned int i;
 unsigned char result;

 result=NO_MATCH;
  
 for(i=0; i<RootDirSectors; i++)
  {
   result=ScanOneDirectorySector(FirstRootSector + i, name);
   if(result!=NO_MATCH) break; //break sector loop
  }

 return(result);
}

//###########################################################
void MakeFileName(char *inname, char *outname, char *outext)
//###########################################################
{
 unsigned char by,i,j;
 char *po;

 po=outname;
 for(i=0; i<8; i++) *po++=' '; //fill filename buffers with spaces

 po=outext;
 *po++=' ';
 *po++=' ';
 *po  =' ';

 po=outname;

//29.04.2004
 if(inname[0]=='.' && inname[1]==0)
  {
   *po++='.';
   return;
  }

 if(inname[0]=='.' && inname[1]=='.') //change to upper dir
  {
   *po++='.';
   *po  ='.';
  }
 else
  {
   i=0; //get filename and make it uppercase
   do
    {
     by=inname[i];
     if(by!='.' && by!=0) *po++=toupper(by);
     i++;
    }while(i<8 && by!='.' && by!=0);

   //if i < 8 there was a dot or a \0
   //if i == 8 there could be a dot or a \0
   if(i==8 && by!='.' && by!=0) { by=inname[i]; i++; }

   if(by=='.')
    {
     j=0;
     do
      {
       by=inname[i];
       if(by!=0) outext[j]=toupper(by);
       j++;
       i++;
      }while(j<3 && by!=0);
    }
  }

// for(i=0; i<8; i++) putchar(outname[i]);
// for(i=0; i<3; i++) putchar(outext[i]);
}


