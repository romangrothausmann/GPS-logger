//#########################################################################
// File: fat.h
//
// Benutzt nur die erste Partition
// Nur für Laufwerke mit 512 Bytes pro Sektor
//
// Nach einem White Paper von MS
// FAT: General Overview of On-Disk Format
// Version 1.03, December 6, 2000
//
// 24.09.2006 typecast most constants for better compatibility with other compilers
//            like MCC18 for PIC18Fxxxx
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 19.10.2006
//#########################################################################
// Compiler: AVR-GCC 3.4.5
//#########################################################################

#ifndef __FAT_H
#define __FAT_H

// These defines are for FAT debugging only. You don't need them.
// Activating all three options takes about 1.5kB of flash.
// So be careful on devices with small flash memory !
//#define FAT_DEBUG_SHOW_FAT_INFO //activate FAT information output via printf() or puts() in GetDriveInformation()
//#define FAT_DEBUG_CLUSTERS // show cluster numbers read/write access via printf()
//#define FAT_DEBUG_RW_HITS // show FAT sectors read/write access summary when calling Fclose(), via printf()

//file operations
#define END_DIR		0
#define NO_MATCH	1
#define MATCH_NAME	2
#define MATCH_EXT	3
#define FULL_MATCH	MATCH_NAME + MATCH_EXT

#define IOB_DATA	(unsigned char)0x01  // iob[] holds data of a file sector
#define IOB_FAT		(unsigned char)0x02  // iob[] holds data of a FAT sector
#define IOB_DIR		(unsigned char)0x03  // iob[] holds data of a directory sector

#define PART1_TABLE_OFFSET (unsigned int)0x01BE //offset to first partitiontable in sector 0

//Using structures needs less memory than indexing in arrays like inbuff[]

//partitiontable structure
//most of it is not used in this program
//bootsector offset is the only thing we need
//because C/H/S values are not used. LBA !
struct PartInfo {
                 unsigned char status;      //Partition status, 0x80 = Active, 0x00 = inactive
                 unsigned char firsthead;   //First head used by partition
                 unsigned int  firstseccyl; //First sector and cylinder used by partition
                 unsigned char type;        //Partition type
                 unsigned char lasthead;    //Last head used by partition
                 unsigned int  lastseccyl;  //Last sector and cylinder used by partition
                 unsigned long bootoffset;  //Location of boot sector. !!!!!!!!!!!
                 unsigned long secofpart;   //Number of sectors for partition
};

//first sector of disc is the master boot record
//it contains four partitiontables
//only the first partition is used in this program
struct MBR {
            unsigned char dummy[PART1_TABLE_OFFSET]; //we don't need all these bytes
            struct PartInfo part1;
            struct PartInfo part2;
            struct PartInfo part3;
            struct PartInfo part4;
//all bytes below are not necessary
};

//part of FAT12/16 bootsector different to FAT32
struct RemBoot //FAT12/16 defs beginning at offset 36
 {
	unsigned char  BS_DrvNum;
	unsigned char  BS_Reserved1;
	unsigned char  BS_BootSig;
	unsigned char  BS_VolID[4];
	char           BS_VolLab[11];
	char           BS_FilSysType[8];
	unsigned char  remaining_part[450];
 };

//part of FAT32 bootsector different to FAT12/16
struct RemBoot32 //FAT32 defs beginning at offset 36
  {
	unsigned long  BPB_FATSz32; //4 bytes
	unsigned int   BPB_ExtFlags; //2 bytes
	unsigned int   BPB_FSVer; //2 bytes
	unsigned long  BPB_RootClus; //4 bytes
	unsigned int   BPB_FSInfo; //2 bytes
	unsigned int   BPB_BkBootSec; //2 bytes
	unsigned char  BPB_Reserved[12];
	unsigned char  BS_DrvNum;
	unsigned char  BS_Reserved1;
	unsigned char  BS_BootSig;
	unsigned long  BS_VolID; //4 bytes
	char           BS_VolLab[11];
	char           BS_FilSysType[8];
	unsigned char  remaining_part[422];
}; 

union endboot 
{
       struct RemBoot   rm;
       struct RemBoot32 rm32;
};

struct BootSec 
{
	unsigned char  BS_jmpBoot[3];
	char           BS_OEMName[8];
	unsigned int   BPB_BytesPerSec; //2 bytes
	unsigned char  BPB_SecPerClus;
	unsigned int   BPB_RsvdSecCnt; //2 bytes
	unsigned char  BPB_NumFATs;
	unsigned int   BPB_RootEntCnt; //2 bytes
	unsigned int   BPB_TotSec16; //2 bytes
	unsigned char  BPB_Media;
	unsigned int   BPB_FATSz16; //2 bytes
	unsigned int   BPB_SecPerTrk; //2 bytes
	unsigned int   BPB_NumHeads; //2 bytes
	unsigned long  BPB_HiddSec; //4 bytes
	unsigned long  BPB_TotSec32; //4 bytes
        union endboot  eb; //remaining part of bootsector
};


#define BYTE_PER_SEC	(unsigned int)512

#define FAT12	(unsigned char)12
#define FAT16	(unsigned char)16
#define FAT32	(unsigned char)32

//defines for special cluster values
//free cluster has value 0
//for fat32 don't use upper four bits ! ignore them
//cluster value of 0x10000000 is a FREE cluster in FAT32
 
//values for end of cluster chain
//ranges for example for FAT12 from 0xFF8 to 0xFFF
#define EOC12	(unsigned int)0xFF8
#define EOC16	(unsigned int)0xFFF8
#define EOC32	(unsigned long)0x0FFFFFF8

//values for bad marked clusters
#define BADC12	(unsigned int)0xFF7
#define BADC16	(unsigned int)0xFFF7
#define BADC32	(unsigned long)0x0FFFFFF7

//values for reserved clusters
//ranges for example for FAT12 from 0xFF0 to 0xFF6
#define RESC12	(unsigned int)0xFF0
#define RESC16	(unsigned int)0xFFF0
#define RESC32	(unsigned long)0x0FFFFFF0

#ifdef USE_FAT32
 #define DISK_FULL (unsigned long)0xFFFFFFFF
#else
 #define DISK_FULL (unsigned int)0xFFFF
#endif

//File/Dir Attributes
#define ATTR_FILE	(unsigned char)0x00 //not defined by MS ! I did it 
#define ATTR_READ_ONLY	(unsigned char)0x01
#define ATTR_HIDDEN	(unsigned char)0x02
#define ATTR_SYSTEM	(unsigned char)0x04
#define ATTR_VOLUME_ID	(unsigned char)0x08
#define ATTR_DIRECTORY	(unsigned char)0x10
#define ATTR_ARCHIVE	(unsigned char)0x20
#define ATTR_LONG_NAME  (unsigned char)0x0F
#define ATTR_NO_ATTR  	(unsigned char)0xFF //not defined by MS ! I did it 

//Char codes not allowed in a filename
//NOT checked yet
//0x22, 0x2A, 0x2B, 0x2C, 0x2E, 0x2F, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x5B, 0x5C, 0x5D, and 0x7C. 

struct DirEntry {
                 unsigned char DIR_Name[8];      //8 chars filename
                 unsigned char DIR_Ext[3];       //3 chars extension
                 unsigned char DIR_Attr;         //file attributes RSHA
                 unsigned char DIR_NTres;        //set to zero
                 unsigned char DIR_CrtTimeTenth; //creation time part in milliseconds
                 unsigned int  DIR_CrtTime;      //creation time
                 unsigned int  DIR_CrtDate;      //creation date
                 unsigned int  DIR_LastAccDate;  //last access date (no time for this !)
                 unsigned int  DIR_FstClusHI;  //first cluster high word                 
                 unsigned int  DIR_WrtTime;      //last write time
                 unsigned int  DIR_WrtDate;      //last write date
                 unsigned int  DIR_FstClusLO;  //first cluster low word                 
                 unsigned long DIR_FileSize;     
                };

//do a little trick for getting long name characters from a DirEntry
//DirEntryBuffer later gets the same adress as DirEntry
struct DirEntryBuffer {
                 unsigned char longchars[sizeof(struct DirEntry)];
                };

//Prototypes
extern unsigned char GetDriveInformation(void);
extern void UpdateFATBuffer(unsigned long newsector);

#ifdef USE_FAT32
 extern unsigned long GetFirstSectorOfCluster(unsigned long n);
 extern unsigned long GetNextClusterNumber(unsigned long cluster);
 extern unsigned char WriteClusterNumber(unsigned long cluster, unsigned long number);
 extern unsigned long AllocCluster(unsigned long currentcluster);
 extern unsigned long FindFreeCluster(unsigned long currentcluster);
#else
 extern unsigned long GetFirstSectorOfCluster(unsigned int n);
 extern unsigned int GetNextClusterNumber(unsigned int cluster);
 extern unsigned char WriteClusterNumber(unsigned int cluster, unsigned int number);
 extern unsigned int AllocCluster(unsigned int currentcluster);
 extern unsigned int FindFreeCluster(unsigned int currentcluster);
#endif

#ifdef USE_FAT32
 extern unsigned long endofclusterchain;
 extern unsigned long maxcluster;        // last usable cluster+1
 extern unsigned long FAT32RootCluster;
#else
 extern unsigned int endofclusterchain;
 extern unsigned int maxcluster;        // last usable cluster+1
#endif

extern unsigned long maxsect;           // last sector on drive
extern unsigned char secPerCluster;
extern unsigned long BytesPerCluster;

extern unsigned char fatbuf[];   //buffer for FAT sectors
extern unsigned char iob[];      //file i/o buffer

//extern unsigned long FATHits;	// count FAT write cycles. you don't really need this ;)
extern unsigned long FATFirstSector;
extern unsigned long FATCurrentSector;
extern unsigned char FATtype;
extern unsigned char FATStatus; // only for FAT write buffering
extern unsigned char iob_status;// only without FAT write buffering 

#ifdef USE_FAT32
  #ifdef FAT_DEBUG_RW_HITS
   extern unsigned long FATWRHits;
   extern unsigned long FATRDHits;
  #endif

 extern unsigned long FirstDirCluster;
 extern unsigned long FileFirstCluster;
 extern unsigned long FileCurrentCluster;
#else
  #ifdef FAT_DEBUG_RW_HITS
   extern unsigned int FATWRHits;
   extern unsigned int FATRDHits;
  #endif

 extern unsigned int FirstDirCluster;
 extern unsigned int FileFirstCluster;
 extern unsigned int FileCurrentCluster;
#endif

extern unsigned long FileCurrentSector;
extern unsigned long File1stClusterSector;
extern unsigned long FileClusterCount;
extern unsigned long FileDirSector;
extern unsigned char FileDirOffset;
extern unsigned long FileSize;
extern unsigned long FilePosition;
extern unsigned char FileFlag;
extern unsigned char FileAttr;
extern unsigned char FileName[];
extern unsigned char FileExt[];

extern unsigned long FirstRootSector;
extern unsigned long FirstDataSector; 
extern unsigned long RootDirSectors;

#endif //FAT_H
