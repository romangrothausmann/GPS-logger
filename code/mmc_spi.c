//###########################################################
// File: mmc_spi.c
//
// Read-/Writeroutines for MMC MultiMedia cards and
// SD SecureDigital cards in SPI mode.
//
// This will work only for MMC cards with 512 bytes block length !
//
// 10.10.2006 Added a busy check after CMD0. Without this one of
//            my newer 512MB NoName card hangs.
//
// 16.09.2006 Changed MMCCommand(). Should wait for response !
//            Changed MMCReadSector(), MMCWriteSector(). 
//
// 08.09.2006 Removed one dummy write in MMCWriteSector()
//
// 04.09.2006 Made new MMC_IO_Init(). Removed IO pin settings from
//            MMCIdentify(). Why ? See comments above MMC_IO_Init(). 
//
// 20.06.2006 Store value for SPI speed and switch to double speed
//            for Read/WriteSector(). Restore SPI speed at end of
//            routines.
//
// 22.08.2005 Bugfix for SanDisk 256MB SD card in MMCIdentify();
//
// 29.09.2004 split SPI_WRITE() into SPI_WRITE() and SPI_WAIT()
//            speeds up program because CPU can do something else
//            while SPI hardware module is shifting in/out data 
//            see MMCReadSector() and MMCWriteSector()
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 25.10.2006
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: AVR-GCC 3.4.5
//#########################################################################
#include <avr/io.h>

#include "mydefs.h"
#include "dos.h"

#if defined (MMC_DEBUG_IDENTIFY) || defined (MMC_DEBUG_SECTORS) || defined (MMC_DEBUG_COMMANDS)
 #include "serial.h" // For debug only
 #include "printf.h" // For debug only
#endif

#ifdef MMC_CARD_SPI

//######################################################
unsigned char MMCCommand(unsigned char command, unsigned long adress)
//######################################################
{
#ifdef MMC_DEBUG_COMMANDS
 printf("Cmd %x:",(unsigned int)command);
#endif

 SPI_WRITE(DUMMY_WRITE); 		// Dummy write
 SPI_WAIT();
 SPI_WRITE(command);
 SPI_WAIT();
 SPI_WRITE((unsigned char)(adress>>24)); // MSB of adress
 SPI_WAIT();
 SPI_WRITE((unsigned char)(adress>>16));
 SPI_WAIT();
 SPI_WRITE((unsigned char)(adress>>8));
 SPI_WAIT();
 SPI_WRITE((unsigned char)adress);       // LSB of adress
 SPI_WAIT();
 SPI_WRITE(0x95); 			 // Checksum for CMD0 GO_IDLE_STATE and dummy checksum for other commands
 SPI_WAIT();                             

#ifdef MMC_DEBUG_COMMANDS
   printf(" %x",(unsigned int)SPI_REGISTER);
#endif

 //wait for response
 while(SPI_REGISTER==DUMMY_WRITE)
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();
#ifdef MMC_DEBUG_COMMANDS
   printf(" %x",(unsigned int)SPI_REGISTER);
#endif
  }

 return SPI_REGISTER;      			 
}

//######################################################
unsigned char MMCReadSector(unsigned long sector, unsigned char *buf)
//######################################################
{
 unsigned int i;
 unsigned char *p,by;
 unsigned long startadr;
 unsigned char  tmpSPSR;

 if(sector>=maxsect) return 1; //sectornumber too big

 p=buf; //using a pointer is much faster than indexing buf[i]
 
 //tmpSPCR=SPCR;
 tmpSPSR=SPSR; //save old value
 SPSR=0x01;    //give me double speed

 MMC_CS_OFF();

//calculate startadress of the sector
 //This will work only up to 4GB
 startadr=sector * BYTE_PER_SEC;

#ifdef MMC_DEBUG_SECTORS
 printf("RS %lu\n",sector);
#endif

 by=MMCCommand(MMC_READ_BLOCK,startadr);

 while(by!=START_BLOCK_TOKEN)
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();
   by=SPI_REGISTER; // wait for card response

#ifdef MMC_DEBUG_COMMANDS
// no way to come out of this :( skip this sector !?
     if(SPI_REGISTER==0x01) // ERROR !
      {
       // One of my SD cards sends this error. My cardreader has also
       // problems to read (NOT write !) the card completely. Trash. 
       printf("\nRead error 0x01 at sector %lu !\n",sector);
       MMC_CS_ON();
       SPSR=tmpSPSR;    //restore old value
       // data buffer is not valid at this point !
       return 1;
      }
#endif      
  }

#ifdef STANDARD_SPI_READ
 for(i=0; i< BYTE_PER_SEC; i++)
  {
   SPI_WRITE(DUMMY_WRITE); // start shift in next byte
   SPI_WAIT();      // wait for next byte
   *p++=SPI_REGISTER;       // store byte in buffer
  }
  
 SPI_WRITE(DUMMY_WRITE); // shift in crc part1
 SPI_WAIT();
 SPI_WRITE(DUMMY_WRITE); // shift in crc part2
 SPI_WAIT();
#endif // STANDARD_SPI_READ

#ifdef FAST_SPI_READ
// The following code looks very strange !
// The idea is not to stop the cpu while SPI module transfers data.
// You have 16 cpu cycles until transmission has finished !
// You can use this time to do something like storing your last data
// or get your next data out of memory, doing some loop overhead.....
// Don't wait for end of transmission until you have done something better ;)
//
 SPI_WRITE(DUMMY_WRITE); // shift in first byte
 SPI_WAIT();      // we have to wait for the first byte, but ONLY for the first byte
 by=SPI_REGISTER;         // get first byte, but store later !
 
 SPI_WRITE(DUMMY_WRITE); // start shift in next byte
                  
 for(i=0; i< (BYTE_PER_SEC-1); i++) //execute the loop while transmission is running in background
  {
   *p++=by;         // store last byte in buffer while SPI module shifts in new data
   SPI_WAIT();      // wait for next byte
   by=SPI_REGISTER;         // get next byte, but store later !
   SPI_WRITE(DUMMY_WRITE); // start shift in next byte
   // do the for() loop overhead at this point while SPI module shifts in new data
  } 

 // last SPI_WRITE(DUMMY_WRITE); is shifting in crc part1 at this point
 *p=by;         // store last byte in buffer while SPI module shifts in crc part1
 SPI_WAIT();
 SPI_WRITE(DUMMY_WRITE); // shift in crc part2
 SPI_WAIT();
#endif //FAST_SPI_READ

 MMC_CS_ON();

 SPSR=tmpSPSR;    //restore old value

 return 0;
}

#ifdef DOS_WRITE
//######################################################
unsigned char MMCWriteSector(unsigned long sector, unsigned char *buf)
//######################################################
{
 unsigned int i;
 unsigned char *p, by;
 unsigned long startadr;
 unsigned char  tmpSPSR;


 if(sector>=maxsect) return 1; //sectornumber too big
 
 p=buf; //using a pointer is much faster than indexing buf[i]

 //tmpSPCR=SPCR;
 tmpSPSR=SPSR; //save old value
 SPSR=0x01;    //give me double speed

 MMC_CS_OFF();

//calculate startadress
 //This will work only up to 4GB
 startadr=sector * BYTE_PER_SEC;

#ifdef MMC_DEBUG_SECTORS
 printf("WS %lu\n",sector);
#endif

 MMCCommand(MMC_WRITE_BLOCK,startadr);

 SPI_WRITE(DUMMY_WRITE);  // Dummy write. Do we need this ?
 SPI_WAIT();

#ifdef STANDARD_SPI_WRITE
 SPI_WRITE(START_BLOCK_TOKEN); // start block token for next sector
 SPI_WAIT();
                  
 for(i=0; i<BYTE_PER_SEC; i++)
  {
   SPI_WRITE(*p++); // shift out next byte
   SPI_WAIT();    // wait for end of transmission
  }
#endif //STANDARD_SPI_WRITE

#ifdef FAST_SPI_WRITE
 SPI_WRITE(START_BLOCK_TOKEN); // start block token for next sector
                  
 for(i=0; i<BYTE_PER_SEC; i++) // execute the loop while transmission is running in background
  {
   // do the for() loop overhead at this point while SPI module shifts out new data
   by=*p++;       // get next data from memory while SPI module shifts out new data
   SPI_WAIT();    // wait for end of transmission
   SPI_WRITE(by); // start shift out next byte
  }

 SPI_WAIT();      // wait til last byte is written to MMC
#endif //FAST_SPI_WRITE

 SPI_WRITE(DUMMY_WRITE); // 16 bit crc follows data
 SPI_WAIT();
 SPI_WRITE(DUMMY_WRITE);
 SPI_WAIT();

 SPI_WRITE(DUMMY_WRITE); // read response
 SPI_WAIT();
 by=SPI_REGISTER & 0x1F;
 if(by != 0x05)   // data block accepted ?
  {
#ifdef MMC_DEBUG_COMMANDS
       printf("\nWrite error at sector %lu !\n",sector);
#endif      
   MMC_CS_ON();
   SPSR=tmpSPSR;    //restore old value
   return 1;
  }

 do
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();
// busy is not a byte value ! card sends 0 BITS if busy. So it can be any of this:
// 00000000 busy
// 00000001 busy has gone
// 00000011 busy has gone
// 00000111 busy has gone
// 00001111 busy has gone
// 00011111 busy has gone
// 00111111 busy has gone
// 01111111 busy has gone
  }while(SPI_REGISTER == 0x00); // wait til busy is gone

 MMC_CS_ON();

 SPSR=tmpSPSR;    //restore old value

 return 0;
}
#endif //DOS_WRITE

//######################################################
// Init MMC IO pins
//
// Removed this from MMCIdentify().
// Maybe some other devices are connected to SPI bus.
// All chip select pins of these devices should have
// high level before starting SPI transmissions !
// So first call MMC_IO_Init(), after that for example
// VS1001_IO_Init(), SPILCD_IO_Init() and AFTER that MMCIdentify().
void MMC_IO_Init(void)
//######################################################
{
 sbi(MMC_CS_PORT,MMC_CS_BIT); //MMC_CS set 1
 sbi(MMC_CS_DDR,MMC_CS_BIT);  //MMC_CS output

 cbi(MMC_SCK_PORT,MMC_SCK_BIT); //SCK set 0
 sbi(MMC_SCK_DDR,MMC_SCK_BIT);  //SCK output

 cbi(MMC_MISO_DDR,MMC_MISO_BIT);  //MISO input

 cbi(MMC_MOSI_PORT,MMC_MOSI_BIT); //MOSI set 0
 sbi(MMC_MOSI_DDR,MMC_MOSI_BIT);  //PB5 MOSI output
}

//######################################################
unsigned char MMCIdentify(void)
//######################################################
{
 unsigned char by;
 unsigned int i;
 unsigned int c_size, c_size_mult;
// unsigned int read_bl_len;
// unsigned long drive_size;

// Init SPI with a very slow transfer rate first !

// SPCR SPI Controlregister
// SPIE=0; //No SPI Interrupt
// SPE=1;  //SPI Enable
// DORD=0; //Send MSB first
// MSTR=1; //I am the master !
// CPOL=0; //SCK low if IDLE
// CPHA=0; //SPI Mode 0
// SPR1=1; //SPI Clock = f/128 = 125kHz @16MHz Clock
// SPR0=1; //or f/64 if SPI2X = 1 in SPSR register
 SPCR=0x53;

// SPSR SPI Statusregister
// SPI2X=1; //Double speed for SPI = 250kHz @16MHz Clock
// SPSR=0x01;
 SPSR=0x00;

 for(i=0; i<10; i++)
  {
   SPI_WRITE(DUMMY_WRITE); // give min 74 SPI clock pulses before
                           // sending commands
   SPI_WAIT();
  }                                      
 
 MMC_CS_OFF();

#ifdef MMC_DEBUG_IDENTIFY
 puts("Send CMD0\n");
#endif

//send CMD0 for RESET
 by=MMCCommand(MMC_GO_IDLE_STATE,0);

// busy is not a byte value ! card sends 0 BITS if busy. So it can be any of this:
// 00000000 busy
// 00000001 busy has gone
// 00000011 busy has gone
// 00000111 busy has gone
// 00001111 busy has gone
// 00011111 busy has gone
// 00111111 busy has gone
// 01111111 busy has gone
// One of my NoName cards sends 0x00, 0x7F at power on, but 0xFF, 0xFF, 0x01 if reset only with power already on.
// Added a busy check below and now it works !
 while(by==0x00) //wait til busy is gone
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();
   by=SPI_REGISTER;
#ifdef MMC_DEBUG_IDENTIFY
   printf(" %x",(unsigned int)SPI_REGISTER);
#endif
  } 

#ifdef MMC_DEBUG_IDENTIFY
 puts("Send CMD1\n");
#endif

// Repeat CMD1 til result=0
 do
  {
   by=MMCCommand(MMC_INIT,0);
  }while(by!=0);

#ifdef MMC_DEBUG_IDENTIFY
 printf("CMD1 done\n");
#endif

// Read CID
// MMCCommand(MMC_READ_CID,0); // nothing really interesting here

#ifdef MMC_DEBUG_IDENTIFY
 puts("Read CSD\n");
#endif

// Read CSD Card Specific Data
 by=MMCCommand(MMC_READ_CSD,0);

// This waiting loop is for SanDisk 256MB SD
// Works with my other cards too
 while(by!=START_BLOCK_TOKEN)
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();
   by=SPI_REGISTER; // Wait for card response
  }
   
 for(i=0; i<16; i++) // CSD has 128 bits -> 16 bytes
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();
   by=SPI_REGISTER;
   iob[i]=by;
  }

 SPI_WRITE(DUMMY_WRITE); // 16 bit crc follows data
 SPI_WAIT();
 SPI_WRITE(DUMMY_WRITE);
 SPI_WAIT();

#ifdef MMC_DEBUG_IDENTIFY
 printf("Read CSD done\n");
#endif
// Here comes the hard stuff !
// Calculate disk size and number of last sector
// that can be used on your mmc/sd card

 c_size=iob[6] & 0x03; //bits 1..0
 c_size<<=10;
 c_size+=(unsigned int)iob[7]<<2;
 c_size+=iob[8]>>6;

// read_bl_len is always 512 !!??
/*
 by= iob[5] & 0x0F;
 read_bl_len=1;
 read_bl_len<<=by;
*/

 by=iob[9] & 0x03;
 by<<=1;
 by+=iob[10] >> 7;
 
 c_size_mult=1;
 c_size_mult<<=(2+by);

// drive_size=(unsigned long)(c_size+1) * (unsigned long)c_size_mult * (unsigned long)read_bl_len;
// maxsect= drive_size / BYTE_PER_SEC; // Got it !

 maxsect=(unsigned long)(c_size+1) * (unsigned long)c_size_mult;

 MMC_CS_ON();

#ifdef MMC_DEBUG_IDENTIFY
 printf("c_size %u\n",c_size);
 printf("c_size_mult %u\n",c_size_mult);
// printf("read_bl_len %u\n",read_bl_len); // always 512
 printf("DriveSize %lu kB\n",(maxsect>>1));
 printf("maxsect %lu\n",maxsect);
#endif

// Switch to high speed SPI
// SPR1=0; //SPI Clock = f/4 = 4MHz @16MHz Clock
// SPR0=0; //or f/2 if SPI2X = 1 in SPSR register
 SPCR=0x50;

// SPSR SPI Statusregister
// SPI2X=1; //Double speed for SPI = 8MHz @16MHz Clock
 SPSR=0x01;

 return 0;
}


#endif //MMC_CARD_SPI
