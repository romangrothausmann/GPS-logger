//###########################################################
// File: dos.c
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
// 10.10.2006 unsigned int tmp1 in Fwrite() and Fread() speeds up !
//            "% BYTE_PER_SEC" replaced with "&= (BYTE_PER_SEC -1)
//            "/ BYTE_PER_SEC" replaced with " >> 9"
//
//#########################################################################
// Last change: 18.10.2006
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: avr-gcc 3.4.5
//#########################################################################
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dos.h"

#if defined (FAT_DEBUG_RW_HITS)
 #include "serial.h" //for testing only
 #include "printf.h" //for testing only
#endif

#ifdef DOS_WRITE
#ifdef DOS_REMOVE
//###########################################################
// delete a file
unsigned char Fremove(char *name)
//###########################################################
{
#ifdef USE_FAT32
unsigned long tmp,tmp1;
#else
unsigned int tmp,tmp1;
#endif

if(FileFlag!=0) return F_ERROR; // don't delete if a file is open

FileAttr=0xFF; // test if we really find a file and not a directory
// FileAttr then must be ATTR_FILE
                
if(FindName(name)==FULL_MATCH) // Look if file exists
    {
    if(FileAttr!=ATTR_FILE) return F_ERROR; // this was not a file !

    tmp=FileFirstCluster;

    // free clusters in FAT cluster chain (make zero)
    do
        {
        tmp1=GetNextClusterNumber(tmp); // save next cluster number
        WriteClusterNumber(tmp,0);      // free cluster
        tmp=tmp1;                       // restore next cluster number
        }while(tmp<endofclusterchain);

#ifdef USE_FATBUFFER
    if(FATStatus>0)
        {
        WriteSector(FATCurrentSector,fatbuf); // write the FAT buffer
        FATStatus=0;
        } 
#endif

    FileName[0]=0xE5;   //mark file as deleted. does not affect long filename entrys !
    FileSize=0;         //make filesize 0
    FileFirstCluster=0; //delete first cluster
    FileAttr=0;         //is this necessary ? 
    UpdateFileEntry();
    }
  
return F_OK;  
}
#endif //DOS_DELETE
#endif //DOS_WRITE

#ifdef DOS_WRITE
//###########################################################
// write one byte to file
// returns 0 if nothing written
// returns 1 if byte is written
//
// Fputc() does not write to CF until a sector is completely
// filled. you can force writing with Fflush() when file should
// keep open, or close the file with Fclose().
//
//###########################################################
/*
unsigned int Fputc(unsigned char buf)
{
 return Fwrite(&buf,1); //the easy way :)
//maybe its faster to return unsigned char
}
*/

//###########################################################
// write count bytes from buffer to file
// returns number of bytes written
//
// Fwrite() does not write to CF until a sector is completely
// filled. you can force writing with Fflush() when file should
// keep open, or close the file with Fclose().
//
//###########################################################
unsigned int Fwrite(unsigned char *buf, unsigned int count)
//###########################################################
{
 unsigned char *p, secoffset;
 unsigned long tmp;
 unsigned int buffoffset, bytecount, tmp1;

 if(FileFlag==0) return 0; //don't write if file is closed
                           //written bytes are zero

 if(count==0) return 0; //nothing to write
 
 p=buf;  //pointer to write buffer
 bytecount=0;

 do
  {
   tmp=FileSize;         //next write position
   tmp-= FileClusterCount; //calc byte position in cluster
   
   if(tmp >= BytesPerCluster)//is position in new cluster ?
    {
     WriteSector(FileCurrentSector,iob); //write last sector
     FileCurrentCluster=AllocCluster(FileCurrentCluster); //alloc new cluster
#ifdef USE_FATBUFFER
#else
     iob_status=IOB_FAT; // we have to track this for FClose() and Fflush() !
#endif
     if(FileCurrentCluster==DISK_FULL)
      {
       return bytecount; //return number of bytes written before disk full
      } 

     File1stClusterSector=GetFirstSectorOfCluster(FileCurrentCluster);//set new 1st sector of cluster
     FileCurrentSector=File1stClusterSector; //set new current sector
//       FileClusterCount++;                   //update cluster count
     FileClusterCount+=BytesPerCluster;   //update cluster count
     tmp-= BytesPerCluster;               //calc new byte position in cluster
    }

   tmp1=(unsigned int)tmp; //we lose upper bits here, but it does not matter !
   
//   secoffset=(unsigned char)(tmp1 / BYTE_PER_SEC);  //calc offset from first sector in cluster
   secoffset=(unsigned char)(tmp1 >> 9 );  //calc offset from first sector in cluster

   if(File1stClusterSector+secoffset != FileCurrentSector)
    {
     WriteSector(FileCurrentSector,iob); //write last sector used
     FileCurrentSector=File1stClusterSector+secoffset;
    }
      
//   buffoffset=(unsigned int)(tmp % BYTE_PER_SEC); //calc offset in sector
   buffoffset = tmp1 & (BYTE_PER_SEC - 1); // tmp % 512 => tmp & (512-1)

#ifdef USE_FATBUFFER
#else
  iob_status=IOB_DATA; // we have to track this for FClose() and Fflush() !
#endif

  tmp1=bytecount; //keep this in mind for calculating new FileSize
  
#ifdef FAST_FWRITE
  // At this point we have two possible situations
  // 1. all bytes can be copied to current sector buffer
  // 2. all bytes can NOT be copied to current sector buffer
  // Do an optimized loop for each situation

  if((buffoffset + (count-bytecount)) <= BYTE_PER_SEC) // all bytes can be copied to current sector buffer
   {
    do
     {
      iob[buffoffset] = *p++;  //put byte in write buffer
      buffoffset++;
      bytecount++;           //one byte written to buffer
     }while(bytecount < count);
   }
  else //all bytes can NOT be copied to current sector buffer
   {
    do
     {
      iob[buffoffset] = *p++;  //put byte in write buffer
      buffoffset++;
      bytecount++;           //one byte written to buffer
     }while(buffoffset < BYTE_PER_SEC);
   } 
#endif //#ifdef FAST_FWRITE

#ifdef SMALL_FWRITE
  //write all bytes that current sector can hold in one step
  do
   {
    iob[buffoffset] = *p++;  //put byte in write buffer
    buffoffset++;
    bytecount++;           //one byte written to buffer
   }while(bytecount<count && buffoffset<BYTE_PER_SEC);
#endif //#ifdef SMALL_FWRITE

   FileSize += (bytecount - tmp1); //update Filesize

 }while(bytecount<count);

 return bytecount;
}
#endif //DOS_WRITE

#ifdef DOS_READ
//###########################################################
// read one byte from file
// returns the byte
// NO ERROR CHECKING ! Use FileSize to see if you have
// reached the end of the file !
//###########################################################
/*
unsigned char Fgetc(void)
{
 unsigned char by;

 Fread(&by,1);

 return by;
}
*/

//###########################################################
// read count bytes into buffer
// returns number of bytes read
unsigned int Fread(unsigned char *buf, unsigned int count)
//###########################################################
{
 unsigned char *p, secoffset;
 unsigned long tmp;
 unsigned int buffoffset, bytecount, tmp1;
 unsigned char end_of_file_is_near;

 if(FileFlag==0) return 0; //don't read if file is closed
                           //read bytes are zero

 if(count==0 || FileSize==0) return 0; //nothing to read
 
 p=buf;  //pointer to read buffer
 bytecount=0;

 do
  {
   tmp=FilePosition;
   if(tmp<FileSize) //end of file reached ?
    {
//     tmp-= FileClusterCount * BytesPerCluster; //calc byte position in cluster
     tmp-= FileClusterCount; //calc byte position in cluster
//maybe its faster to count cluster and sectorbytes

     if(tmp >= BytesPerCluster)//is position in current cluster ?
      {
       FileCurrentCluster=GetNextClusterNumber(FileCurrentCluster); //if not get next cluster
       File1stClusterSector=GetFirstSectorOfCluster(FileCurrentCluster);//set new 1st sector of cluster
       ReadSector(File1stClusterSector,iob); //read new sector
#ifdef USE_FATBUFFER
#else
       iob_status=IOB_DATA;
#endif

       FileCurrentSector=File1stClusterSector; //set new current sector
//       FileClusterCount++;                   //update cluster count
       FileClusterCount+=BytesPerCluster;   //update cluster count
       tmp-= BytesPerCluster;               //calc new byte position in cluster
      }

     tmp1=(unsigned int)tmp; //we lose upper bits here, but it does not matter !

//     secoffset=(unsigned char)(tmp1 / BYTE_PER_SEC);  //calc sector offset from first sector in cluster
     secoffset=(unsigned char)(tmp1 >> 9);  //calc sector offset from first sector in cluster

     if(File1stClusterSector+secoffset != FileCurrentSector) //new sector ?
      {
       FileCurrentSector=File1stClusterSector+secoffset;
       ReadSector(FileCurrentSector,iob); //read new sector
#ifdef USE_FATBUFFER
#else
       iob_status=IOB_DATA;
#endif
      }
      
//   buffoffset=(unsigned int)(tmp % BYTE_PER_SEC); //calc offset in sector
     buffoffset = tmp1 & (BYTE_PER_SEC - 1); // tmp % 512 => tmp & (512-1)

     if((FileSize-FilePosition) > BYTE_PER_SEC)
      {
       end_of_file_is_near = 0;
       tmp1 = bytecount;
      } 
     else end_of_file_is_near=1;
     
#ifdef FAST_FREAD
  // At this point we have two possible situations
  // 1. all bytes can be copied from current sector buffer
  // 2. all bytes can NOT be copied from current sector buffer
  // Do an optimized loop for each situation

    if((buffoffset + (count-bytecount)) <= BYTE_PER_SEC) // all bytes can be copied from current sector buffer
     {

      do
       {
        *p++ = iob[buffoffset];
        buffoffset++;
        bytecount++;     //one byte read into buffer

        if(end_of_file_is_near)
         {
          FilePosition++;  //update file position
          if(FilePosition==FileSize) break; //end of file reached, break the loop
         }

//       }while(bytecount<count && FilePosition<FileSize);
       }while(bytecount<count);

     }
    else // all bytes can NOT be copied from current sector buffer
     {

      do
       {
        *p++ = iob[buffoffset];
        buffoffset++;
        bytecount++;     //one byte read into buffer

        if(end_of_file_is_near)
         {
          FilePosition++;  //update file position
          if(FilePosition==FileSize) break; //end of file reached, break the loop
         }

//       }while(buffoffset<BYTE_PER_SEC && FilePosition<FileSize);
       }while(buffoffset<BYTE_PER_SEC);

     } //if((buffoffset + (count-bytecount)) <= BYTE_PER_SEC)

    if(end_of_file_is_near==0) FilePosition += (bytecount - tmp1);  //update file position

#endif //#ifdef FAST_FREAD
      
#ifdef SMALL_FREAD
     //get all bytes buffer can hold in one step
     do
      {
       *p++=iob[buffoffset];
       buffoffset++;
       bytecount++;     //one byte read into buffer

       if(end_of_file_is_near)
        {
         FilePosition++;  //update file position
         if(FilePosition==FileSize) break; //end of file reached break the loop
        }

//      }while(bytecount<count && buffoffset<BYTE_PER_SEC && FilePosition<FileSize);
      }while(bytecount<count && buffoffset<BYTE_PER_SEC);

     if(end_of_file_is_near==0) FilePosition += (bytecount - tmp1);  //update file position

#endif //#ifdef SMALL_FREAD

    } //if(tmp<FileSize)
   else return bytecount; //return bytes read til end of file

  } while(bytecount<count);
  
 return bytecount;
}
#endif //DOS_READ

#if defined (DOS_READ) || defined (DOS_WRITE)
//###########################################################
//open a file for reading OR writing, NOT both !
unsigned char Fopen(char *name, unsigned char flag)
//###########################################################
{
#ifdef DOS_WRITE
 unsigned long tmp;
#endif //DOS_WRITE

 FileClusterCount=0;

 if(FileFlag!=0) return F_ERROR; //a file is already open !

 #ifdef FAT_DEBUG_RW_HITS
  FATWRHits=0;
  FATRDHits=0;
 #endif
 
#ifdef DOS_READ
 if(flag==F_READ)
  {
   if(FindName(name)==FULL_MATCH) //file MUST exist for reading
    {
     FilePosition=0; //actual read position
     FileCurrentCluster=FileFirstCluster;
     FileFlag=flag; //needed for fclose

     File1stClusterSector=GetFirstSectorOfCluster(FileFirstCluster);
     FileCurrentSector=File1stClusterSector;
     ReadSector(FileCurrentSector,iob); //read first sector of file
#ifdef USE_FATBUFFER
#else
       iob_status=IOB_DATA;
#endif

     return F_OK;      //give back something above zero
    }
  }
#endif //DOS_READ

#ifdef DOS_WRITE
 if(flag==F_WRITE)
  {
   //if file exists, open it and spool to end of file to append data
   if(FindName(name)==FULL_MATCH)
    {
     tmp=FileFirstCluster;
     FileCurrentCluster=FileFirstCluster; //we need this if file is smaller as ONE cluster
     
     while(tmp<endofclusterchain) //go to end of cluster chain
      {
       tmp=GetNextClusterNumber(tmp);
       if(tmp<endofclusterchain)
        {
         FileCurrentCluster=tmp;
         FileClusterCount+=BytesPerCluster;
        } 
      }

     tmp= FileSize - FileClusterCount;
     File1stClusterSector=GetFirstSectorOfCluster(FileCurrentCluster); 
     FileCurrentSector=File1stClusterSector;
     FileCurrentSector += tmp / BYTE_PER_SEC;

     ReadSector(FileCurrentSector,iob); //read first sector of file
#ifdef USE_FATBUFFER
#else
       iob_status=IOB_DATA;
#endif
    }
   else //make a new file
    { 
     FileAttr=ATTR_FILE; //needed for MakeNewFileEntry() ! 18.10.2006
     MakeFileName(name,FileName,FileExt); //Split into name and extension
     if(MakeNewFileEntry()) //file does not exist, try to make new file in currentdir
      {
       FileCurrentCluster=FileFirstCluster;
       File1stClusterSector=GetFirstSectorOfCluster(FileFirstCluster); 
       FileCurrentSector=File1stClusterSector;

       ReadSector(FileCurrentSector,iob); //read first sector of file
#ifdef USE_FATBUFFER
#else
       iob_status=IOB_DATA;
#endif
      }
     else
      {
       FileFlag=0; //needed for fclose
       return F_ERROR; //new file could not be made
      }
    }

   FileFlag=flag; //needed for fclose
   return F_OK; //file is open for writing
  }//if(flag==F_WRITE)
#endif //DOS_WRITE
  
 return F_ERROR; //something went wrong  
}

//###########################################################
// close the file
// no error codes
void Fclose(void)
//###########################################################
{
#ifdef DOS_READ
 if(FileFlag==F_READ)
  {
  }
#endif //DOS_READ

#ifdef DOS_WRITE
 if(FileFlag==F_WRITE)
  {
   Fflush();  //write last sector used to CF and update filesize, filetime

   #ifdef FAT_DEBUG_RW_HITS
    #ifdef USE_FAT32
     printf("FAT WR Hits %lu ",FATWRHits);
    #else
     printf("FAT WR Hits %u ",FATWRHits);
    #endif
   #endif //#ifdef FAT_DEBUG_RW_HITS
  }


#ifdef USE_FATBUFFER
#else
 iob_status=0;
#endif

#endif //DOS_WRITE

 #ifdef FAT_DEBUG_RW_HITS
  #ifdef USE_FAT32
   printf("FAT RD Hits %lu\n",FATRDHits);
  #else
   printf("FAT RD Hits %u\n",FATRDHits);
  #endif
 #endif //#ifdef FAT_DEBUG_RW_HITS

 FileFlag=0; //a second fclose should do nothing
             //reading and writing disabled

}
#endif //#if defined (DOS_READ) || defined (DOS_WRITE)

#ifdef DOS_WRITE
//###########################################################
// force writing last data written into sectorbuffer to be
// stored into CF without Fclose(). direntry will also be
// updated.
void Fflush(void)
//###########################################################
{
 if(FileFlag==0) return; //don't write if file is closed

#ifdef USE_FATBUFFER
 if(FATStatus>0)
  {
   WriteSector(FATCurrentSector,fatbuf); // write the FAT buffer

   #ifdef FAT_DEBUG_RW_HITS
    FATWRHits++;
   #endif

   FATStatus=0;
  } 

 WriteSector(FileCurrentSector,iob); //write last sector used
#else
 //if last access to iob was a FAT access than we should not write FileCurrentSector here
 if(iob_status==IOB_DATA)
  {
   WriteSector(FileCurrentSector,iob); //write last sector used
  }
#endif

 //update file entry filesize and date/time
 UpdateFileEntry();
 ReadSector(FileCurrentSector,iob); //iob was changed in UpdateFileEntry()

#ifdef USE_FATBUFFER
#else
 iob_status=IOB_DATA;
#endif
}
#endif //DOS_WRITE

//############################################################
// search for a filename or directoryname in current directory
unsigned char FindName(char *name)
//############################################################
{
 unsigned char result;

 if(FileFlag>0) return NO_MATCH; //don't search if a file is open
 
 if(FirstDirCluster<2)
  {
   result=ScanRootDir(name);
  }
 else
  {
   result=ScanSubDir(FirstDirCluster,name);
  }

 return result;
}

