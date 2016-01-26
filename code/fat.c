//###########################################################
// File: fat.c
//
// For FAT12, FAT16 and FAT32
// Only for first Partition
// Only for drives with 512 bytes per sector (the most)
//
// Based on a White Paper from MS
// FAT: General Overview of On-Disk Format
// Version 1.03, December 6, 2000
//
// MBR MasterBootRecord
// PC intern 4
// M.Tischer
// Data Becker
//
// 11.10.2006 Replaced "% BYTE_PER_SEC" with "& (BYTE_PER_SEC-1)".
//            Typecast variables from "unsigned long" to "unsigned int" before:
//            secoffset = (unsigned int)fatoffset & (BYTE_PER_SEC-1);
//            Use "unsigned int" for indexing arrays. Does not really speed up,
//            but reduces code size ;)
//
// 25.09.2006 Initialize all global variables to zero for better compatibility
//            with other compilers.
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 19.10.2006
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

#if defined (FAT_DEBUG_SHOW_FAT_INFO) || defined (FAT_DEBUG_RW_HITS) || defined (FAT_DEBUG_CLUSTERS)
 #include "serial.h" //for testing only
 #include "printf.h" //for testing only
#endif

unsigned char iob[BYTE_PER_SEC];      //file i/o buffer

unsigned long FirstDataSector=0; 
unsigned long FirstRootSector=0; 

unsigned long FATFirstSector=0;  
unsigned char FATtype=0;         

#ifdef USE_FAT32
 unsigned long FAT32RootCluster=0;

 #ifdef FAT_DEBUG_RW_HITS
  unsigned long FATWRHits=0;
  unsigned long FATRDHits=0;
 #endif

#else
 #ifdef FAT_DEBUG_RW_HITS
  unsigned int FATWRHits=0;
  unsigned int FATRDHits=0;
 #endif
#endif

#ifdef USE_FATBUFFER
 unsigned char fatbuf[BYTE_PER_SEC];   //buffer for FAT sectors
 unsigned long FATCurrentSector=0;

 #ifdef DOS_WRITE
  unsigned char FATStatus=0; // only for FAT write buffering
 #endif
#else
// #ifdef DOS_WRITE
  unsigned char iob_status=0;
// #endif
#endif

unsigned char secPerCluster=0;

unsigned long RootDirSectors=0;

//use some global variables for file access
#ifdef USE_FAT32
 unsigned long FirstDirCluster=0;
 unsigned long FileFirstCluster=0;     //first cluster of file
 unsigned long FileCurrentCluster=0;   //actual cluster in use
 unsigned long endofclusterchain=0; //value for END_OF_CLUSTERCHAIN 
 unsigned long maxcluster=0;	 // last usable cluster+1
#else
 unsigned int FirstDirCluster=0;
 unsigned int FileFirstCluster=0;     //first cluster of file
 unsigned int FileCurrentCluster=0;   //actual cluster in use
 unsigned int endofclusterchain=0; //value for END_OF_CLUSTERCHAIN 
 unsigned int maxcluster=0;	 // last usable cluster+1
#endif

unsigned long FileCurrentSector=0;    //sector with last data read/written
unsigned long File1stClusterSector=0; //1st sector of cluster in use
unsigned long FileClusterCount=0;     //clusters read (not really)
unsigned long FileDirSector=0;        //sector with dir entry of file
unsigned char FileDirOffset=0;        //offset to file entry in FileDirSector
unsigned long FileSize=0;
unsigned long FilePosition=0;         //actual position when reading file         
unsigned char FileFlag=0;		    //read or write
unsigned char FileAttr=0;		    //file attribute
unsigned char FileName[8];
unsigned char FileExt[3];

unsigned long maxsect=0;           // last sector on drive
unsigned long BytesPerCluster=0;   //bytes per cluster

#ifdef USE_FATBUFFER
//############################################################
void UpdateFATBuffer(unsigned long newsector)
//############################################################
{
 if(newsector!=FATCurrentSector) // do we need to update the FAT buffer ?
  {
#ifdef DOS_WRITE
   if(FATStatus>0)
    {
     WriteSector(FATCurrentSector,fatbuf); // write the old FAT buffer
     FATStatus=0; // flag FAT buffer is save

     #ifdef FAT_DEBUG_RW_HITS
      FATWRHits++;
     #endif
    } 
#endif
   ReadSector(newsector,fatbuf); //read FAT sector

     #ifdef FAT_DEBUG_RW_HITS
      FATRDHits++;
     #endif
   FATCurrentSector=newsector;
  } 
}
#endif

//############################################################
// get next clusternumber of fat cluster chain
#ifdef USE_FAT32
 unsigned long GetNextClusterNumber(unsigned long cluster)
#else
 unsigned int GetNextClusterNumber(unsigned int cluster)
#endif
//############################################################
{
#ifdef USE_FAT12
 unsigned int tmp, secoffset;
 unsigned char fatoffset;
#endif

 union Convert *cv;

#ifdef FAT_DEBUG_CLUSTERS
#ifdef USE_FAT32
 printf("GNCN %lu\n",cluster);
#else
 printf("GNCN %u\n",cluster);
#endif
#endif

 if(cluster<maxcluster) //we need to check this ;-)
  {

#ifdef USE_FAT12
   if(FATtype==FAT12)
    {
     //FAT12 has 1.5 Bytes per FAT entry
     tmp= ((unsigned int)cluster * 3) >>1 ; //multiply by 1.5 (rounds down)
//     secoffset = fatoffset % BYTE_PER_SEC; //we need this for later
     secoffset = (unsigned int)tmp & (BYTE_PER_SEC-1); //we need this for later
     fatoffset= (unsigned char)(tmp / BYTE_PER_SEC); //sector offset from FATFirstSector
 
#ifdef USE_FATBUFFER
     UpdateFATBuffer(FATFirstSector + fatoffset);
     if(secoffset == (BYTE_PER_SEC-1)) //if this is the case, cluster number is
                                   //on a sector boundary. read the next sector too
      {
       tmp=(unsigned int)fatbuf[BYTE_PER_SEC-1]; //keep first part of cluster number
       UpdateFATBuffer(FATFirstSector + fatoffset +1 ); //read next FAT sector
       tmp+=(unsigned int)fatbuf[0] << 8; //second part of cluster number
      }
     else
      {
       cv=(union Convert *)&fatbuf[secoffset];
       tmp=cv->ui;
      } 
#else //#ifdef USE_FATBUFFER
     ReadSector(FATFirstSector + fatoffset,iob); //read FAT sector

     #ifdef FAT_DEBUG_RW_HITS
      FATRDHits++;
     #endif
     if(secoffset == (BYTE_PER_SEC-1)) //if this is the case, cluster number is
                                   //on a sector boundary. read the next sector too
      {
       tmp=(unsigned int)iob[BYTE_PER_SEC-1]; //keep first part of cluster number
   
       ReadSector(FATFirstSector + fatoffset +1,iob ); //read next FAT sector

       #ifdef FAT_DEBUG_RW_HITS
        FATRDHits++;
       #endif

       tmp+=(unsigned int)iob[0] << 8; //second part of cluster number
      }
     else
      {
       cv=(union Convert *)&iob[secoffset];
       tmp=cv->ui;
      } 
#endif //#ifdef USE_FATBUFFER

     if(cluster & 0x01) tmp>>=4; //shift to right position
     else               tmp&=0xFFF; //delete high nibble

     return (tmp);
    }//if(FATtype==FAT12)
#endif //#ifdef USE_FAT12

#ifdef USE_FAT16
   if(FATtype==FAT16)
    {
     //two bytes per FAT entry
#ifdef USE_FATBUFFER
//     UpdateFATBuffer(FATFirstSector + ((unsigned long)cluster * 2) / BYTE_PER_SEC);
//     UpdateFATBuffer(FATFirstSector + ((unsigned int)cluster) / (BYTE_PER_SEC/2));
     UpdateFATBuffer(FATFirstSector + (unsigned char)((unsigned int)cluster >> 8));

//     cv=(union Convert *)&fatbuf[(cluster * 2) % BYTE_PER_SEC];
//     cv=(union Convert *)&fatbuf[(unsigned int)(cluster * 2) & (BYTE_PER_SEC-1)];
     cv=(union Convert *)&fatbuf[((unsigned int)cluster << 1) & (BYTE_PER_SEC-1)];
#else //#ifdef USE_FATBUFFER
//     ReadSector(FATFirstSector + ((unsigned long)cluster * 2) / BYTE_PER_SEC, iob);
//     ReadSector(FATFirstSector + ((unsigned int)cluster) / (BYTE_PER_SEC/2), iob);
     ReadSector(FATFirstSector + (unsigned char)((unsigned int)cluster >> 8), iob);

     #ifdef FAT_DEBUG_RW_HITS
      FATRDHits++;
     #endif
//     cv=(union Convert *)&iob[(cluster * 2) % BYTE_PER_SEC];
//     cv=(union Convert *)&iob[(unsigned int)((unsigned long)cluster * 2) & (BYTE_PER_SEC-1)];
//     cv=(union Convert *)&iob[((unsigned int)cluster * 2) & (BYTE_PER_SEC-1)];
     cv=(union Convert *)&iob[((unsigned int)cluster << 1) & (BYTE_PER_SEC-1)];
#endif //#ifdef USE_FATBUFFER

     return(cv->ui);
    }//if(FATtype==FAT16)
#endif //#ifdef USE_FAT16

#ifdef USE_FAT32
   if(FATtype==FAT32)
    {
     //four bytes per FAT entry
#ifdef USE_FATBUFFER
//     UpdateFATBuffer(FATFirstSector + (cluster * 4) / BYTE_PER_SEC);
//     UpdateFATBuffer(FATFirstSector + (cluster) / (BYTE_PER_SEC/4));
     UpdateFATBuffer(FATFirstSector + (cluster >> 7) );

//     cv=(union Convert *)&fatbuf[(cluster * 4) % BYTE_PER_SEC];
//     cv=(union Convert *)&fatbuf[((unsigned int)cluster * 4) & (BYTE_PER_SEC-1)];
     cv=(union Convert *)&fatbuf[((unsigned int)cluster << 2) & (BYTE_PER_SEC-1)];
#else //#ifdef USE_FATBUFFER
//     ReadSector(FATFirstSector + (cluster * 4) / BYTE_PER_SEC, iob);
     ReadSector(FATFirstSector + (cluster >> 7), iob);

     #ifdef FAT_DEBUG_RW_HITS
      FATRDHits++;
     #endif
//     cv=(union Convert *)&iob[(cluster * 4) % BYTE_PER_SEC];
     cv=(union Convert *)&iob[((unsigned int)cluster << 2) & (BYTE_PER_SEC-1)];
#endif //#ifdef USE_FATBUFFER

     return( cv->ul & 0x0FFFFFFF );
    }//if(FATtype==FAT32) 
#endif //#ifdef USE_FAT32
  }
  
 return DISK_FULL; //return impossible cluster number
}

//###########################################################
//first datacluster is cluster 2 !
#ifdef USE_FAT32
 unsigned long GetFirstSectorOfCluster(unsigned long n)
#else
 unsigned long GetFirstSectorOfCluster(unsigned int n)
#endif
//###########################################################
{
 return (((unsigned long)(n - 2) * secPerCluster) + FirstDataSector);
}

#ifdef DOS_WRITE
//###########################################################
#ifdef USE_FAT32
 unsigned long AllocCluster(unsigned long currentcluster)
#else
 unsigned int AllocCluster(unsigned int currentcluster)
#endif
//###########################################################
{
#ifdef USE_FAT32
 unsigned long cluster;
#else
 unsigned int cluster;
#endif

// do this if you want to search from beginning of FAT
// cluster=FindFreeCluster(0); //get next free cluster number
 cluster=FindFreeCluster(currentcluster); // get next free cluster number
 if(cluster!=DISK_FULL && cluster<=maxcluster) // disk full ?
  {
    // insert new cluster number into chain
    // currentcluster=0 means: this is a new cluster chain
    if(currentcluster>0) WriteClusterNumber(currentcluster,cluster);

   // mark end of cluster chain
#ifdef USE_FAT12
   if(FATtype==FAT12) WriteClusterNumber(cluster,0xFFF); 
#endif
#ifdef USE_FAT16
   if(FATtype==FAT16) WriteClusterNumber(cluster,0xFFFF); 
#endif
#ifdef USE_FAT32
   if(FATtype==FAT32) WriteClusterNumber(cluster,0x0FFFFFFF); 
#endif
  }

 return cluster;
}
#endif //DOS_WRITE

#ifdef DOS_WRITE
//###########################################################
// go through the FAT to find a free cluster
#ifdef USE_FAT32
 unsigned long FindFreeCluster(unsigned long currentcluster)
#else
 unsigned int FindFreeCluster(unsigned int currentcluster)
#endif
//###########################################################
{
#ifdef USE_FAT32
 unsigned long cluster;
#else
 unsigned int cluster;
#endif

 cluster=currentcluster+1; // its a good idea to look here first
                           // maybe we do not need to search the whole FAT
                           // and can speed up free cluster search
                           // if you do not want this call FindFreeCluster(0)
// search til end of FAT
 while(cluster<maxcluster)
  {
   if(GetNextClusterNumber(cluster)==0) break;
   cluster++;
  }

// if we have not found a free cluster til end of FAT
// lets start a new search at beginning of FAT
 if(cluster>=maxcluster)
  {
   cluster=2; // first possible free cluster
   while(cluster<=currentcluster) // search til we come to where we have started
    {
     if(GetNextClusterNumber(cluster)==0) break;
     cluster++;
    }

   if(cluster>=currentcluster) return DISK_FULL; // no free cluster found
  }
  
 if(cluster>=maxcluster) return DISK_FULL;
    
 return cluster;
}
#endif //DOS_WRITE

#ifdef DOS_WRITE
//############################################################
// write a cluster number into fat cluster chain
#ifdef USE_FAT32
 unsigned char WriteClusterNumber(unsigned long cluster, unsigned long number)
#else
 unsigned char WriteClusterNumber(unsigned int cluster, unsigned int number)
#endif
//############################################################
{
#ifdef USE_FAT12
 unsigned int tmp, secoffset;
 unsigned char fatoffset;
 unsigned char lo,hi;
#endif

 unsigned long sector;
 unsigned char *p;
 
#ifdef FAT_DEBUG_CLUSTERS
#ifdef USE_FAT32
 printf("WCN %lu\n",cluster);
#else
 printf("WCN %u\n",cluster);
#endif
#endif

 if(cluster<maxcluster) //we need to check this ;-)
  {

#ifdef USE_FAT12
   if(FATtype==FAT12)
    {
     //FAT12 has 1.5 Bytes per FAT entry
     tmp= ((unsigned int)cluster * 3) >>1 ; //multiply by 1.5 (rounds down)
//     secoffset = fatoffset % BYTE_PER_SEC; //we need this for later
     secoffset = (unsigned int)tmp & (BYTE_PER_SEC-1); //we need this for later
     fatoffset= (unsigned char)(tmp / BYTE_PER_SEC); //sector offset from FATFirstSector
     sector=FATFirstSector + fatoffset;

     tmp=(unsigned int)number;
     if(cluster & 0x01) tmp<<=4; //shift to right position
     lo=(unsigned char)tmp;
     hi=(unsigned char)(tmp>>8);
     
#ifdef USE_FATBUFFER
     UpdateFATBuffer(sector); //read FAT sector
#else //#ifdef USE_FATBUFFER
     ReadSector(sector,iob); //read FAT sector

     #ifdef FAT_DEBUG_RW_HITS
      FATRDHits++;
     #endif
#endif //#ifdef USE_FATBUFFER
     if(secoffset == (BYTE_PER_SEC-1)) //if this is the case, cluster number is
                                   //on a sector boundary. read the next sector too
      {
#ifdef USE_FATBUFFER
       p=&fatbuf[BYTE_PER_SEC-1]; //keep first part of cluster number
#else //#ifdef USE_FATBUFFER
       p=&iob[BYTE_PER_SEC-1]; //keep first part of cluster number
#endif //#ifdef USE_FATBUFFER

       if(cluster & 0x01)
        {
         *p&=0x0F;
         *p|=lo;
        }
       else *p=lo;
        
#ifdef USE_FATBUFFER
       FATStatus=1; // we have made an entry, so write before next FAT sector read
       UpdateFATBuffer(sector+1); //read FAT sector
       p=&fatbuf[0]; //second part of cluster number
#else //#ifdef USE_FATBUFFER
       WriteSector(sector,iob);

       #ifdef FAT_DEBUG_RW_HITS
        FATWRHits++;
       #endif

       ReadSector(sector+1,iob ); //read next FAT sector

       #ifdef FAT_DEBUG_RW_HITS
        FATRDHits++;
       #endif

       p=&iob[0]; //second part of cluster number
#endif //#ifdef USE_FATBUFFER

       if(cluster & 0x01) *p=hi;
       else
        {
         *p&=0xF0;
         *p|=hi;
        }

#ifdef USE_FATBUFFER
       FATStatus=1; // we have made an entry, so write before next FAT sector read
#else //#ifdef USE_FATBUFFER
       WriteSector(sector+1,iob);

       #ifdef FAT_DEBUG_RW_HITS
        FATWRHits++;
       #endif
#endif //#ifdef USE_FATBUFFER
      }
     else
      {
#ifdef USE_FATBUFFER
       p=&fatbuf[secoffset];
#else //#ifdef USE_FATBUFFER
       p=&iob[secoffset];
#endif //#ifdef USE_FATBUFFER

       if(cluster & 0x01)
        {
         *p&=0x0F;
         *p++|=lo;
         *p=hi;
        } 
       else
        {
         *p++=lo;
         *p&=0xF0;
         *p|=hi;
        } 

#ifdef USE_FATBUFFER
       FATStatus=1; // we have made an entry, so write before next FAT sector read
#else //#ifdef USE_FATBUFFER
       WriteSector(sector,iob);

       #ifdef FAT_DEBUG_RW_HITS
        FATWRHits++;
       #endif
#endif //#ifdef USE_FATBUFFER
      } 

    }//if(FATtype==FAT12)
#endif

#ifdef USE_FAT16
   if(FATtype==FAT16)
    {
     //two bytes per FAT entry
//     sector=FATFirstSector + ((unsigned long)cluster * 2) / BYTE_PER_SEC;
//     sector=FATFirstSector + ((unsigned int)cluster) / (BYTE_PER_SEC/2);
     sector=FATFirstSector + (unsigned char)((unsigned int)cluster >> 8);

#ifdef USE_FATBUFFER
     UpdateFATBuffer(sector); //read FAT sector

//     p=&fatbuf[(cluster * 2) % BYTE_PER_SEC];
     p=&fatbuf[((unsigned int)cluster << 1) & (BYTE_PER_SEC-1)];
#else //#ifdef USE_FATBUFFER
     ReadSector(sector, iob);

     #ifdef FAT_DEBUG_RW_HITS
      FATRDHits++;
     #endif
//     p=&iob[(cluster * 2) % BYTE_PER_SEC];
     p=&iob[((unsigned int)cluster << 1) & (BYTE_PER_SEC-1)];
#endif //#ifdef USE_FATBUFFER

     *p++=(unsigned char)(number);
     *p  =(unsigned char)(number >> 8);

#ifdef USE_FATBUFFER
     FATStatus=1; // we have made an entry, so write before next FAT sector read
#else //#ifdef USE_FATBUFFER
     WriteSector(sector, iob);

     #ifdef FAT_DEBUG_RW_HITS
      FATWRHits++;
     #endif
#endif //#ifdef USE_FATBUFFER

    }// if(FATtype==FAT16)
#endif //#ifdef USE_FAT16

#ifdef USE_FAT32
   if(FATtype==FAT32)
    {
     //four bytes per FAT entry
//     sector=FATFirstSector + (cluster * 4) / BYTE_PER_SEC;
//     sector=FATFirstSector + (cluster) / (BYTE_PER_SEC/4);
     sector=FATFirstSector + (cluster >> 7);

#ifdef USE_FATBUFFER
     UpdateFATBuffer(sector); //read FAT sector

//     p=&fatbuf[(cluster * 4) % BYTE_PER_SEC];
//     p=&fatbuf[(unsigned int)(cluster * 4) & (BYTE_PER_SEC-1)];
     p=&fatbuf[((unsigned int)cluster << 2) & (BYTE_PER_SEC-1)];
#else //#ifdef USE_FATBUFFER
     ReadSector(sector, iob);

     #ifdef FAT_DEBUG_RW_HITS
      FATRDHits++;
     #endif

//     p=&iob[(cluster * 4) % BYTE_PER_SEC];
//     p=&iob[(unsigned int)(cluster * 4) & (BYTE_PER_SEC-1)];
     p=&iob[((unsigned int)cluster << 2) & (BYTE_PER_SEC-1)];
#endif //#ifdef USE_FATBUFFER

     number&=0x0FFFFFFF;
     
     *p++=(unsigned char)( number);
     *p++=(unsigned char)(number >> 8);
     *p++=(unsigned char)(number >> 16);
     *p  =(unsigned char)(number >> 24);

#ifdef USE_FATBUFFER
     FATStatus=1; // we have made an entry, so write before next FAT sector read
#else //#ifdef USE_FATBUFFER
     WriteSector(sector, iob);

     #ifdef FAT_DEBUG_RW_HITS
      FATWRHits++;
     #endif
#endif //#ifdef USE_FATBUFFER

    }// if(FATtype==FAT32) 
#endif //#ifdef USE_FAT32
  } // if(cluster<maxcluster) //we need to check this ;-)

 return 0;
}
#endif //DOS_WRITE

//###########################################################
unsigned char GetDriveInformation(void)
//###########################################################
{
 unsigned char by;
 unsigned long DataSec,TotSec;
 unsigned long bootSecOffset;
 unsigned long FATSz; // FATSize
 unsigned char loop;

#ifdef USE_FAT32
  unsigned long CountofClusters;
#else
  unsigned int CountofClusters;
#endif

 unsigned int  RootEntrys;

 struct MBR *mbr;
 struct BootSec *boot;
  
 by=IdentifyMedia(); //LaufwerksInformationen holen
 if(by==0)
  {
   FATtype=0; //Unknown FAT type
   bootSecOffset=0; //erstmal

   by=ReadSector(0,iob); //Lese den MBR. Erster Sektor auf der Platte
                       //enthält max. 4 Partitionstabellen mit jeweils 16Bytes
                       //Die erste fängt bei 0x01BE an, und nur die nehme ich !

   //Erstmal checken ob wir nicht schon einen Bootsektor gelesen haben.
   boot=(struct BootSec *)iob;

   loop=0;
   do
    {
     // Jetzt checke ich doch den FAT-String im Bootsektor um den Typ der FAT
     // zu bestimmen. Einen besseren Weg sehe ich im Moment nicht.
     if(   boot->eb.rm.BS_FilSysType[0]=='F'
        && boot->eb.rm.BS_FilSysType[1]=='A'
        && boot->eb.rm.BS_FilSysType[2]=='T'
        && boot->eb.rm.BS_FilSysType[3]=='1'  )
      {
       //Wenn ich hier ankomme habe ich entweder FAT12 oder FAT16
#ifdef USE_FAT12
       if(boot->eb.rm.BS_FilSysType[4]=='2') FATtype=FAT12;
#endif
#ifdef USE_FAT16
       if(boot->eb.rm.BS_FilSysType[4]=='6') FATtype=FAT16;
#endif
      }
     else
      {
#ifdef USE_FAT32
       if(   boot->eb.rm32.BS_FilSysType[0]=='F'
          && boot->eb.rm32.BS_FilSysType[1]=='A'
          && boot->eb.rm32.BS_FilSysType[2]=='T'
          && boot->eb.rm32.BS_FilSysType[3]=='3'
          && boot->eb.rm32.BS_FilSysType[4]=='2'  )
        {
         FATtype=FAT32;
        }
       else //war kein Bootsektor, also feststellen wo der liegt
#endif
        {
         mbr=(struct MBR *)iob; //Pointer auf die Partitionstabelle
         bootSecOffset=mbr->part1.bootoffset; //Nur den brauche ich

         by=ReadSector(bootSecOffset,iob);      //read bootsector
         boot=(struct BootSec *)iob;
        } 
      }

     loop++;
    }while(loop<2 && FATtype==0); //Bis zu zwei Versuche den Bootsektor zu lesen

   if(FATtype==0) return F_ERROR; // FAT-Typ nicht erkannt

   secPerCluster=boot->BPB_SecPerClus; //Sectors per Cluster
   RootEntrys=boot->BPB_RootEntCnt;    //32 Byte Root Directory Entrys

//Number of sectors for FAT
   if(boot->BPB_FATSz16 != 0) FATSz = boot->BPB_FATSz16;
   else FATSz = boot->eb.rm32.BPB_FATSz32; //Für FAT32

   RootDirSectors = ((RootEntrys * 32) + (BYTE_PER_SEC - 1)) / BYTE_PER_SEC;
   
   FATFirstSector= bootSecOffset + boot->BPB_RsvdSecCnt;
   FirstRootSector = FATFirstSector + (boot->BPB_NumFATs * FATSz);
   FirstDataSector = FirstRootSector + RootDirSectors;

   if(boot->BPB_TotSec16 != 0) TotSec = boot->BPB_TotSec16;
   else TotSec = boot->BPB_TotSec32;

//Number of data sectors
   DataSec = TotSec - (boot->BPB_RsvdSecCnt + (boot->BPB_NumFATs * FATSz) + RootDirSectors);

//Number of valid clusters
   CountofClusters = DataSec / secPerCluster;
   maxcluster=CountofClusters+2;

//Note also that the CountofClusters value is exactly that: the count of data clusters
//starting at cluster 2. The maximum valid cluster number for the volume is
//CountofClusters + 1, and the "count of clusters including the two reserved clusters"
// is CountofClusters + 2.

   FirstDirCluster=0; // for FAT12 and FAT16

#ifdef USE_FAT12
   if(FATtype==FAT12)
    {
     endofclusterchain=EOC12;
    } 
#endif
#ifdef USE_FAT16
   if(FATtype==FAT16)
    {
     endofclusterchain=EOC16;
    } 
#endif
#ifdef USE_FAT32
   if(FATtype==FAT32)
    {
     endofclusterchain=EOC32;
     FAT32RootCluster=boot->eb.rm32.BPB_RootClus;
     FirstDirCluster=FAT32RootCluster;
     FirstRootSector=GetFirstSectorOfCluster(FAT32RootCluster);
    }
#endif

  }
 else
  {
   return F_ERROR; // CF gives no answer
  } 

 FileFirstCluster=0;
 FileSize=0;
 FileFlag=0;
 BytesPerCluster=BYTE_PER_SEC * secPerCluster; //bytes per cluster

// for debugging only
#ifdef FAT_DEBUG_SHOW_FAT_INFO
 if(FATtype==FAT12) puts("FAT12\n");
 if(FATtype==FAT16) puts("FAT16\n");
 if(FATtype==FAT32) puts("FAT32\n");

 printf("bootSecOffset %lu\n",bootSecOffset);
 printf("Reserved Sectors %u\n",boot->BPB_RsvdSecCnt);
 printf("FAT Sectors %lu\n",FATSz);
 printf("Num. of FAT's %u\n",(unsigned int)boot->BPB_NumFATs);
 printf("secPerCluster %u\n",(unsigned int)secPerCluster);
 printf("BytesPerCluster %lu\n",BytesPerCluster);
 printf("FATFirstSector %lu\n",FATFirstSector);
 printf("FirstRootSector %lu\n",FirstRootSector);
 printf("RootDirSectors %lu\n",RootDirSectors);
 printf("FirstDataSector %lu\n",FirstDataSector);
 printf("maxsect %lu\n",maxsect);
#ifdef USE_FAT32
 printf("FirstDirCluster %lu\n",FirstDirCluster);
 printf("maxcluster %lu\n",maxcluster);
#else
 printf("FirstDirCluster %u\n",FirstDirCluster);
 printf("maxcluster %u\n",maxcluster);
#endif

#endif //#ifdef FAT_DEBUG_SHOW_FAT_INFO

#ifdef USE_FATBUFFER
 FATCurrentSector=FATFirstSector;
 ReadSector(FATCurrentSector,fatbuf); //read first FAT sector

 #ifdef DOS_WRITE
  FATStatus=0; // nothing to write til here
 #endif

#endif
 return F_OK;
}

