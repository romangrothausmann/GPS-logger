//#########################################################################
// File: media.h
//
// Select media: CompactFlash, MultiMedia Card, SecureDigital Card
//
//#########################################################################
// Last change: 29.07.2005
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: AVR-GCC 3.4.3
//#########################################################################

#ifndef __FLASH_MEDIA_H	
#define __FLASH_MEDIA_H

//define only ONE of these !
//#define COMPACTFLASH_CARD
#define MMC_CARD_SPI //SD_CARD_SPI too !

//maybe later ;)
//#define SD_CARD  //not used yet
//#define HD_DRIVE //not used yet

#endif //__FLASH_MEDIA_H
