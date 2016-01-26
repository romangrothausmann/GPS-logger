#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
//#include <stdint.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "dos.h"
#include "fat.h"
#include "mmc_spi.h"
#include "dir.h"


int main(void){

    MMC_IO_Init();
    sei();


    //PORTB|= (1 << PB6); //set backlight on
    if(GetDriveInformation() != F_OK){ // get drive parameters
        //PORTB|= (1 << PB6); //set backlight on
        //PORTB&= ~(1 << PB6); //set backlight off
        }
    PORTB&= ~(1 << PB6); //set backlight off
    //PORTB|= (1 << PB6); //set backlight on
    if(Fopen("log.txt",F_WRITE) != F_OK){
        //PORTB|= (1 << PB6); //set backlight on
        }
    PORTB|= (1 << PB6); //set backlight on
    Fclose();
    return(0);
    }
