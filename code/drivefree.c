//###########################################################
// File: drivefree.c
//
// Get used and free memory of the drive.
//
// Counts only free and used CLUSTERS. If a file does not
// use all bytes in a cluster, this free space is NOT
// given back by this routines. This space can't be used
// by another file because the cluster is reserved.
//
// Only the free space of the unused clusters will be given back.
// So you get the MINIMUM free memory. This is the same way WIN
// gives you free or used space.
//
// Don't use this functions without a FAT buffer !
// Or you will have to wait a LOOOOOONG time.
//
// With FAT buffer on a 256MB FAT16 CF and 62650 clusters:
// 2 seconds for drivefree() or driveused()
//
// On a 32GB harddrive with FAT32 and 2 Mio clusters: ??????????????????
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 18.12.2005
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: AVR-GCC 3.4.3
//#########################################################################
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dos.h"
#include "printf.h"

#ifdef USE_DRIVEFREE

//################################################
// Give back free memory of unused clusters in kB
unsigned long drivefree(void)
//################################################
{
#ifdef USE_FAT32
 unsigned long tmpcluster, count;
#else
 unsigned int tmpcluster, count;
#endif

 count=0;
 tmpcluster=2;

// search til end of FAT
 while(tmpcluster<maxcluster)
  {
   if(GetNextClusterNumber(tmpcluster)==0) count++;
   tmpcluster++;
  }
 
 return (unsigned long)count * secPerCluster / 2;
}

//################################################
// Give back memory size of used clusters in kB
unsigned long driveused(void)
//################################################
{
 return drivesize() - drivefree();
}

//################################################
// Give back memory size of the drive in kB
unsigned long drivesize(void)
//################################################
{
 return (maxcluster-2) * secPerCluster / 2;
}

#endif //#ifdef USE_DRIVEFREE

