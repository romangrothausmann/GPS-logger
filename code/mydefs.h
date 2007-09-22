//#########################################################################
// File: MYDEFS.H
//
//
//#########################################################################
// Last change: 29.07.2005
//#########################################################################
// Compiler: AVR-GCC 3.4.3
//#########################################################################


#ifndef __MYDEFS_H
#define __MYDEFS_H



#define T1_PRESCALER	8
#define T1_TICKS   	F_CPU / ( T1_PRESCALER * 65536 )	
#define T1_ONESECOND    T1_TICKS

#define NOP() asm volatile ("nop" ::)
//WinAVR 3.4.3 setzt die Bitsetzbefehle sbi(),cbi() ins Assembler-Listing für diese
//beiden #defines ein ! Also kein Grund Inline Assembler zu benutzen.
#define sbi(portn, bitn) ((portn)|=(1<<(bitn)))
#define cbi(portn, bitn) ((portn)&=~(1<<(bitn)))
//#define sbi(portn, bitn) asm volatile("sbi %0, %1" : : "I" (_SFR_IO_ADDR(portn)), "I" ((uint8_t)(bitn)))
//#define cbi(portn, bitn) asm volatile("cbi %0, %1" : : "I" (_SFR_IO_ADDR(portn)), "I" ((uint8_t)(bitn)))




#endif //__MYDEFS_H
