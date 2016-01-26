#include "dos.h"
#include "dos.c"
#include "fat.h"
#include "fat.c"
#include "mmc_spi.h"
#include "mmc_spi.c"
#include "dir.h"
#include "dir.c"
#include "buffer.h"
#include "buffer.c"

volatile unsigned long bytecount;       // Z�hlter f�r die Schreib-Adresse der SD-Karte

int main(void){

    char result;
    char buffer= 'H';

    MMC_IO_Init();                  // initialisiert die SPI-Schnittstelle f�r die SD-Karte
//    PORTB|= (1 << PB6); //set backlight on
    if(GetDriveInformation()!=F_OK){ // Initialisiert die SD-Karte
        PORTB|= (1 << PB6); //set backlight on
        while(1);                       //    Fehler-Handling ;-)
        }
    PORTB|= (1 << PB6); //set backlight on
    result=Fopen("gps.txt",F_WRITE);        // File �ffnen
    bytecount=0;                    // Adressz�hler bei jeder neuen Datei auf 0 setzen!

    if (result==F_OK ){
        if(Fwrite(&buffer,1)!=1) 
            result=F_ERROR;  //Schreibt 1 Byte  von buffer
        bytecount++;
        }

    Fclose();  // Datei schlie�en, vorher auf keinen Fall die SD-Karte
    }
