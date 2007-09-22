//#########################################################################
// File: mmc_spi.h
//
// MMC MultiMediaCard and SD SecureDigital definitions for SPI protocol
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 28.10.2006
//#########################################################################
// Compiler: AVR-GCC 3.4.5
//#########################################################################

#ifndef __MMC_CARD_SPI_H
#define __MMC_CARD_SPI_H

#define STANDARD_SPI_READ  // No optimizations   
//#define FAST_SPI_READ      // Optimizations

#define STANDARD_SPI_WRITE  // No optimizations   
//#define FAST_SPI_WRITE      // Optimizations

//#define MMC_DEBUG_IDENTIFY //activate debug output via printf()
//#define MMC_DEBUG_SECTORS //activate debug output via printf()
//#define MMC_DEBUG_COMMANDS //activate debug output via printf()

#define SPI_REGISTER	SPDR  //makes it easier to switch to another hardware
#define SPI_WRITE(a) 	{ SPI_REGISTER=(a); }
#define SPI_WAIT() 	{ while(! ( SPSR & (1<<SPIF) ) ); }

#if defined (__AVR_ATmega32__) || defined (__AVR_ATmega323__) || defined (__AVR_ATmega161__)

 #define MMC_CS_BIT	4	//Pin number for MMC_CS
 #define MMC_CS_PORT 	PORTB   //Port where MMC_CS is located
 #define MMC_CS_DDR 	DDRB    //Port direction register where MMC_CS is located

 #define MMC_SCK_BIT	7
 #define MMC_SCK_PORT   PORTB
 #define MMC_SCK_DDR    DDRB
 
 #define MMC_MISO_BIT	6
 #define MMC_MISO_PORT  PORTB
 #define MMC_MISO_DDR   DDRB

 #define MMC_MOSI_BIT	5
 #define MMC_MOSI_PORT  PORTB
 #define MMC_MOSI_DDR   DDRB

#elif defined (__AVR_ATmega128__) || defined (__AVR_ATmega64__)

 #define MMC_CS_BIT	0	//Pin number for MMC_CS
 #define MMC_CS_PORT 	PORTB   //Port where MMC_CS is located
 #define MMC_CS_DDR 	DDRB    //Port direction register where MMC_CS is located

 #define MMC_SCK_BIT	1
 #define MMC_SCK_PORT   PORTB
 #define MMC_SCK_DDR    DDRB

 #define MMC_MISO_BIT	3
 #define MMC_MISO_PORT  PORTB
 #define MMC_MISO_DDR   DDRB

 #define MMC_MOSI_BIT	2
 #define MMC_MOSI_PORT  PORTB
 #define MMC_MOSI_DDR   DDRB

#elif defined (__AVR_ATmega168__) || defined (__AVR_ATmega8__) || defined (__AVR_ATmega88__)
 #define MMC_CS_BIT	2	//Pin number for MMC_CS
 #define MMC_CS_PORT 	PORTB   //Port where MMC_CS is located
 #define MMC_CS_DDR 	DDRB    //Port direction register where MMC_CS is located

 #define MMC_SCK_BIT	5
 #define MMC_SCK_PORT   PORTB
 #define MMC_SCK_DDR    DDRB

 #define MMC_MISO_BIT	4
 #define MMC_MISO_PORT  PORTB
 #define MMC_MISO_DDR   DDRB

 #define MMC_MOSI_BIT	3
 #define MMC_MOSI_PORT  PORTB
 #define MMC_MOSI_DDR   DDRB

#else
#  error "processor type not defined in mmc_spi.h"
#endif

#define MMC_CS_ON() 	sbi(MMC_CS_PORT,MMC_CS_BIT);
#define MMC_CS_OFF() 	cbi(MMC_CS_PORT,MMC_CS_BIT);

// MMC/SD commands
#define MMC_RESET		(unsigned char)(0x40 + 0)
#define MMC_GO_IDLE_STATE	(unsigned char)(0x40 + 0)
#define MMC_INIT		(unsigned char)(0x40 + 1)
#define MMC_READ_CSD		(unsigned char)(0x40 + 9)
#define MMC_READ_CID		(unsigned char)(0x40 + 10)
#define MMC_STOP_TRANSMISSION	(unsigned char)(0x40 + 12)
#define MMC_SEND_STATUS		(unsigned char)(0x40 + 13)
#define MMC_SET_BLOCKLEN	(unsigned char)(0x40 + 16)
#define MMC_READ_BLOCK		(unsigned char)(0x40 + 17)
#define MMC_READ_MULTI_BLOCK	(unsigned char)(0x40 + 18)
#define MMC_WRITE_BLOCK		(unsigned char)(0x40 + 24)
#define MMC_WRITE_MULTI_BLOCK	(unsigned char)(0x40 + 25)

#define DUMMY_WRITE		(unsigned char)(0xFF)
#define START_BLOCK_TOKEN	(unsigned char)(0xFE)

//prototypes
extern unsigned char MMCReadSector(unsigned long sector, unsigned char *buf);
extern unsigned char MMCWriteSector(unsigned long sector, unsigned char *buf);
extern unsigned char MMCIdentify(void);
extern void MMC_IO_Init(void);

#define ReadSector(a,b) 	MMCReadSector((a),(b))
#define WriteSector(a,b) 	MMCWriteSector((a),(b))
#define IdentifyMedia()		MMCIdentify()

// security checks !
#if defined (STANDARD_SPI_WRITE) && defined (FAST_SPI_WRITE)
 #error "Define STANDARD_SPI_WRITE or FAST_SPI_WRITE only in mmc_spi.h, NOT both !"
#endif

#if !defined (STANDARD_SPI_WRITE) && !defined (FAST_SPI_WRITE)
 #error "Define at least STANDARD_SPI_WRITE or FAST_SPI_WRITE in mmc_spi.h !"
#endif

#if defined (STANDARD_SPI_READ) && defined (FAST_SPI_READ)
 #error "Define STANDARD_SPI_READ or FAST_SPI_READ only in mmc_spi.h, NOT both !"
#endif

#if !defined (STANDARD_SPI_READ) && !defined (FAST_SPI_READ)
 #error "Define at least STANDARD_SPI_READ or FAST_SPI_READ in mmc_spi.h !"
#endif

#endif
