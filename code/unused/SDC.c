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

volatile unsigned long bytecount;       // Zählter für die Schreib-Adresse der SD-Karte

int main(void){

    char result;
    char buffer= 'H';

    MMC_IO_Init();                  // initialisiert die SPI-Schnittstelle für die SD-Karte
//    PORTB|= (1 << PB6); //set backlight on
    if(GetDriveInformation()!=F_OK){ // Initialisiert die SD-Karte
        PORTB|= (1 << PB6); //set backlight on
        while(1);                       //    Fehler-Handling ;-)
        }
    PORTB|= (1 << PB6); //set backlight on
    result=Fopen("gps.txt",F_WRITE);        // File öffnen
    bytecount=0;                    // Adresszähler bei jeder neuen Datei auf 0 setzen!

    if (result==F_OK ){
        if(Fwrite(&buffer,1)!=1) 
            result=F_ERROR;  //Schreibt 1 Byte  von buffer
        bytecount++;
        }

    Fclose();  // Datei schließen, vorher auf keinen Fall die SD-Karte
    }
