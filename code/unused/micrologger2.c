#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <inttypes.h>
//#include <avr/eeprom.h>
//#include <math.h>
#include <avr/interrupt.h> 
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "dos.h"
#include "dos.c"
#include "fat.h"
#include "fat.c"
#include "mmc_spi.h"
#include "mmc_spi.c"
#include "dir.h"
#include "dir.c"
//#include "drivefree.h"
//#include "drivefree.c"
#include "buffer.h"
#include "buffer.c"


#define gps_on PORTD|=(1<<PD6);
#define gps_off PORTD&=~(1<<PD6);

#define green_on PORTC|=(1<<PC5);
#define green_off PORTC&=~(1<<PC5);

#define red_on PORTC|=(1<<PC4);
#define red_off PORTC&=~(1<<PC4);

#define UART_RX_BUFFER_SIZE     0x00A0      // 128 statt 160 Zeichen ?
#define NMEA_BUFFERSIZE			0x0052		// 80 Zeichen
#define DEBOUNCE                256L        // debounce clock (256Hz = 4msec)

#define MODE_INACTIVE           0x00        // Gerät inaktiv, Logging nicht aktiv
#define MODE_LOGGING            0x01        // Gerät speichert GPS-Daten
#define MODE_PRESSED           0x02        // Taste gedrueckt
//#define MODE_AC_EMPTY           0x04        // Akku fast leer
//#define MODE_TRANSMIT           0x08        // Gerät im Übertragungsmodus
//#define MODE_CLEAR              0x10        // Datensatzlöschmodus
#define MODE_Q_BAD              0x20        // GPS-Empfang schlecht
#define TRUE					1
#define FALSE					0

// constants/macros/typdefs
#define MAX             20

// Message Codes
#define NMEA_NODATA		0	// No data. Packet not available, bad, or not decoded
#define NMEA_GPGGA		1	// Global Positioning System Fix Data
#define NMEA_GPVTG		2	// Course over ground and ground speed
#define NMEA_GPGLL		3	// Geographic position - latitude/longitude
#define NMEA_GPGSV		4	// GPS satellites in view
#define NMEA_GPGSA		5	// GPS DOP and active satellites
#define NMEA_GPRMC		6	// Recommended minimum specific GPS data
#define NMEA_UNKNOWN	0xFF// Packet received but not known

void Usart_Init(void);
void nmeaProcessGPGSA(uint8_t* packet);
void nmeaProcessGPRMC(uint8_t* packet);
void nmeaProcessGPGGA(uint8_t* packet);
uint8_t nmeaProcess(cBuffer* rxBuffer);
void store_trackpoint(void);
void switch_logging(void);
void write_string(char *stri);
void write_string_buf(char *stri);
void batt_empty(void);

volatile unsigned long bytecount;
volatile uint8_t inc_push=0, name_known=0;
volatile char always_on='0';

volatile uint8_t write_sd=0;

volatile uint8_t Prescaler;
uint8_t volatile Mode, dsets=0, possets=0, gps_prec=2;
uint8_t volatile Second;                                        // count Seconds
uint16_t volatile StoreSecond;                                  // Sekundenzähler für GPS-Logging Zyklus
uint16_t volatile StoreCycle;                                   // Eingestellter Logging_Zyklus in s

const prog_char GpxHeader[] =  "<?xml version=\"1.0\"?>\n"
                    "<gpx version=\"1.0\"\n"
                    " creator=\"JG\"\n"
                    " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                    " xmlns=\"http://www.topografix.com/GPX/1/0\"\n"
                    " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0\">\n"
                    //" http://www.topografix.com/GPX/1/0/gpx.xsd\">\n"
					"<trk>\n"
					"<name>1</name>\n"
					"<desc>1</desc>\n"
					"<trkseg>\n";
const prog_char TrkEnd[]= 	"</trkseg>\n"
						  	"</trk>\n"
						  	"</gpx>\n"; 
							
const prog_char NewSettings[]= 	"0010.0.2.10.\r\n"
						  		"^^^^ ^ ^ ^^\r\n"
								"|    | | |\r\n"
						  		"|    | | ->timeout for GPS fix in min\r\n"
								"|    | ->GPS precision (number of datasets before log)\r\n"
								"|    ->GPS always on by '1'\r\n"
								"->log interval in s with leading '0's\r\n"
								"\r\n"
								"micrologger v2, Juergen Gruendmayer 2007\r\n"; 
    

	const prog_char lat[] = 		"<trkpt lat=\"";
	const prog_char endlat[] = 		"\" ";
	const prog_char lon[] = 		"lon=\"";
	const prog_char endlon[] = 		"\">\n";
	const prog_char ele[] = 		"<ele>";
	const prog_char endele[] = 		"</ele>\n";
	const prog_char time[] = 		"<time>";
	const prog_char endtime[] = 	"</time>\n"
									"</trkpt>\n";

struct GpsInfoType {
  volatile  char utc_time[9];
  volatile  char utc_date[11];
  volatile  char latitude[11];          // Breitengrad
  volatile  char latitude_ns[2];        // Breitengrad Nord/Süd
  volatile  char longitude[12];         // Längengrad
  volatile  char longitude_ew[2];       // Längengrad Ost/west
  volatile  char altitude[7];           // Höhe in m
  volatile  char fix[2];                // Art der Positionsbestimmung
};

cBuffer UartRxBuffer;  
volatile struct GpsInfoType GpsInfo;
volatile static char uartRxData[UART_RX_BUFFER_SIZE];
volatile uint8_t NmeaPacket[NMEA_BUFFERSIZE];
volatile unsigned char result, filename[]="gplog001.gpx";

int main(void){

//beginn initialisierungen ********************
	DDRC=0b00110000;	//leds als output
	DDRD=0b01000000;	//gps_pwr als output
	PORTC|=(1<<PC6);	//pull-up fuer inc_push

	TCCR0 = (1<<CS02);					//timer0 mit clk/256
	TIMSK = (1<<TOIE0) | (1<<OCIE1A);
	TCCR1B = (1<<CS10);             	//timer1 (16bit) mit clk
    OCR1A = 0;
    TCNT1 = -1;

	Usart_Init();
	MMC_IO_Init();

	Second = 0;   
    StoreSecond = 0;
	StoreCycle = 10;

	bufferInit(&UartRxBuffer, uartRxData, UART_RX_BUFFER_SIZE);

sei();
//ende initialisierungen ********************
uint8_t i;
uint16_t batt, fixtimeout=10;
unsigned char buffer1, buffer2[3], setchar[20];
_delay_ms(100);

				ADCSRA = (1<<ADEN) | (1<<ADPS1) | (1<<ADPS0);    // Frequenzvorteiler 
                               // setzen auf 8 (1) und ADC aktivieren (1)

				ADMUX = 3;                      // Kanal waehlen
  				ADMUX |= (1<<REFS0); // Vcc als Referenzspannung nutzen 
								  		ADCSRA |= (1<<ADSC);              // eine ADC-Wandlung 
				  		while ( ADCSRA & (1<<ADSC) ) {
     					;     // auf Abschluss der Konvertierung warten 
  						}
						batt=ADCW;
						batt=0;
				  for (i=0; i<5; i++){
				  		ADCSRA |= (1<<ADSC);              // eine ADC-Wandlung 
				  		while ( ADCSRA & (1<<ADSC) ) {
     					;     // auf Abschluss der Konvertierung warten 
  						}
						batt+=ADCW;
						}
				ADCSRA &= ~(1<<ADEN);             // ADC deaktivieren (2)  
				batt/=5;
				if (batt<310) batt_empty(); 



			 if(GetDriveInformation()!=F_OK){ // get drive parameters
 				red_on;
   				while(1);
 			 }

          
          if(Fopen("settings.txt",F_READ)==F_OK)
           {
            Fread(setchar,11);
            Fclose();
			name_known=1;
           }
		  else{
		   		bytecount=0;
				result=Fopen("settings.txt",F_WRITE);
				i=0;
		 		while (buffer1=pgm_read_byte(&NewSettings[i])){
					if (result==F_OK ){
	   	  				if(Fwrite(&buffer1,1)!=1) result=F_ERROR;
	   	 				bytecount++;
						i++;
					}
				}
				Fclose();
				bytecount=0;
		   } 
		if (name_known) always_on = setchar[5];
		if (name_known) gps_prec = atoi(setchar[7]);
		if (name_known){
			buffer2[0]=setchar[9];
			buffer2[1]=setchar[10];
			buffer2[2]='\0';
			fixtimeout=atoi(buffer2);
		}

		setchar[4]='\0';
		if (name_known) StoreCycle=atoi(setchar);
		name_known=0;
		setchar[0]='\0';



gps_on;
StoreSecond=StoreCycle;


while(1){
	uint8_t timeout=0;

				//	while(nmeaProcess(&UartRxBuffer));
        		//	nmeaProcess(&UartRxBuffer);
                        
           			 if (GpsInfo.fix[0] < '2')                              // 2d-fix not available
             			   Mode |= MODE_Q_BAD;
           			 else
               			 Mode &= ~(MODE_Q_BAD); 	


	switch_logging();		

								              
            if( Second >= 2){
 								//hier Battcheck !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

				ADCSRA = (1<<ADEN) | (1<<ADPS1) | (1<<ADPS0);    // Frequenzvorteiler 
                               // setzen auf 8 (1) und ADC aktivieren (1)

				ADMUX = 3;                      // Kanal waehlen
  				ADMUX |= (1<<REFS0); // Vcc als Referenzspannung nutzen 
								  		ADCSRA |= (1<<ADSC);              // eine ADC-Wandlung 
				  		while ( ADCSRA & (1<<ADSC) ) {
     					;     // auf Abschluss der Konvertierung warten 
  						}
						batt=ADCW;
						batt=0;
				  for (i=0; i<3; i++){
				  		ADCSRA |= (1<<ADSC);              // eine ADC-Wandlung 
				  		while ( ADCSRA & (1<<ADSC) ) {
     					;     // auf Abschluss der Konvertierung warten 
  						}
						batt+=ADCW;
						}
				ADCSRA &= ~(1<<ADEN);             // ADC deaktivieren (2)  
				batt/=3;
				if (batt<305) batt_empty(); 
                Second = 0;
            }



             if (StoreSecond >= StoreCycle) {  
					StoreSecond=0;
					if (Mode & MODE_LOGGING){
						dsets=possets=0;
						gps_on;
						//green_on;
						timeout=0;
						while (timeout==0){
						switch_logging();
						//	while(nmeaProcess(&UartRxBuffer));
							if (GpsInfo.fix[0] >= '2'){                              // 2d-fix not available
   		          			   Mode &= ~MODE_Q_BAD;
							   dsets++;
							}
       	    			 	else{
       	        			   Mode |= (MODE_Q_BAD);
							}
							if (StoreSecond>(fixtimeout*60)) timeout=2;
							if (dsets>gps_prec){
								if (possets>gps_prec){
									if(!(Mode &(MODE_Q_BAD)))
									timeout=1;
								}
							}
						}
						if (timeout==1)
						store_trackpoint();
						if (always_on=='0') gps_off;
					//	gps_off;
						//green_off;
						bufferFlush(&UartRxBuffer);
						GpsInfo.fix[0]='0';
						StoreSecond = 0;
					}
            }
    
}

return 0;


}// ENDE Hauptprogramm



void Usart_Init(void)
{
/* Set baud rate */
  
  uint16_t ubrr = 103; // 4800 bps alt: 51; //9600 bps

    UBRRH = (uint8_t) (ubrr>>8);
    UBRRL = (uint8_t) (ubrr);

	UCSRB = (1 << RXEN) | (1<<RXCIE);
	//UCSRC |= (1<<USBS)| (3<<UCSZ0);  //Asynchron 8N1 (1<<USBS)
	UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0); //8 Bit,1 Stop, no parity
	    // Flush Receive-Buffer (entfernen evtl. vorhandener ungültiger Werte)
    do
    {
        uint8_t dummy;
        (void) (dummy = UDR);
    }
    while (UCSRA & (1 << RXC));
	}



ISR(TIMER0_OVF_vect){
 if (PINC & (1<<PC6)) Mode &= ~MODE_PRESSED; //pressed=0;

	if (!(Mode & MODE_PRESSED)){  //pressed==0){
		if (inc_push==0){
			if (!(PINC & (1<<PC6))){
				Mode |= MODE_PRESSED; //pressed=1;
				inc_push=1;
			}
		}
 }
}


ISR(TIMER1_COMPA_vect){
 uint8_t tcnt1h = TCNT1H;
    OCR1A += F_CPU / DEBOUNCE;                               // new compare value
    if( ++Prescaler == (uint8_t)DEBOUNCE ) {
        Prescaler = 0;
        Second++;                                           // exact one Second over
        StoreSecond++;
        #if F_CPU % DEBOUNCE                                 // handle remainder
            OCR1A += F_CPU % DEBOUNCE;                       // compare once per Second
        #endif       
    }
	TCNT1H = tcnt1h;                                        // restore for delay() !
 	if (TCNT0<5){
 		if (!(Mode & MODE_Q_BAD)) green_on;
	}
	else green_off;
	if (TCNT0<5){
 		if (Mode & MODE_LOGGING) red_on;
	}
	else red_off;

}

ISR(USART_RXC_vect){
    char zeichen;
   	 	zeichen = UDR;
   	 	bufferAddToEnd(&UartRxBuffer, zeichen);
		if (Mode & MODE_LOGGING){
				if (result==F_OK ){
	   	  			if(Fwrite(&zeichen,1)!=1) result=F_ERROR;
	   	 			bytecount++;
				}
		} 
}






uint8_t nmeaProcess(cBuffer* rxBuffer) {
	uint8_t foundpacket = NMEA_NODATA;
	uint8_t startFlag = FALSE;
	//uint8_t data;
	uint16_t i,j;

	// process the receive buffer
	// go through buffer looking for packets
	while(rxBuffer->datalength)
	{
		// look for a start of NMEA packet
		if(bufferGetAtIndex(rxBuffer,0) == '$')
		{
			// found start
			startFlag = TRUE;
			// when start is found, we leave it intact in the receive buffer
			// in case the full NMEA string is not completely received.  The
			// start will be detected in the next nmeaProcess iteration.

			// done looking for start
			break;
		}
		else
			bufferGetFromFront(rxBuffer);
	}
	
	// if we detected a start, look for end of packet
	if(startFlag)
	{   
		for(i=1; i<(rxBuffer->datalength)-1; i++)
		{
			// check for end of NMEA packet <CR><LF>
			if((bufferGetAtIndex(rxBuffer,i) == '\r') && (bufferGetAtIndex(rxBuffer,i+1) == '\n'))
			{
				// have a packet end
				// dump initial '$'
				bufferGetFromFront(rxBuffer);
				// copy packet to NmeaPacket
				for(j=0; j<(i-1); j++)
				{
					// although NMEA strings should be 80 characters or less,
					// receive buffer errors can generate erroneous packets.
					// Protect against packet buffer overflow
					if(j<(NMEA_BUFFERSIZE-1))
						NmeaPacket[j] = bufferGetFromFront(rxBuffer);
					else
						bufferGetFromFront(rxBuffer);
				}
				// null terminate it
				NmeaPacket[j] = 0;
				// dump <CR><LF> from rxBuffer
				bufferGetFromFront(rxBuffer);
				bufferGetFromFront(rxBuffer);

				// found a packet
				// done with this processing session
				foundpacket = NMEA_UNKNOWN;
				break;
			}
		}
	}
    
	if(foundpacket)
	{   
		// check message type and process appropriately
		if(!strncmp(NmeaPacket, "GPGGA", 5))
		{
			// process packet of this type
			nmeaProcessGPGGA(NmeaPacket);
			// report packet type
			foundpacket = NMEA_GPGGA;
		}
		else if(!strncmp(NmeaPacket, "GPRMC", 5))
		{
			// process packet of this type
			nmeaProcessGPRMC(NmeaPacket);
			// report packet type
			foundpacket = NMEA_GPRMC;
		}
		else if(!strncmp(NmeaPacket, "GPGSA", 5))
		{
			// process packet of this type
			nmeaProcessGPGSA(NmeaPacket);
			// report packet type
			foundpacket = NMEA_GPGSA;
		}        
       
	}
	else if(rxBuffer->datalength >= rxBuffer->size)
	{
		// if we found no packet, and the buffer is full
		// we're logjammed, flush entire buffer
		bufferFlush(rxBuffer);
	}
	return foundpacket;
}

//************************************************************************************************************
/*
    $GPGGA,170834,4124.8963,N,08151.6838,W,1,05,1.5,280.2,M,-34.0,M,,,*75 
    
1       Time                                        170834          17:08:34 UTC 
2, 3    Latitude                                    4124.8963,  N   41d 24.8963' N or 41d 24' 54" N 
4, 5    Longitude                                   08151.6838, W   81d 51.6838' W or 81d 51' 41" W 
6       Fix Quality:
        - 0 = Invalid
        - 1 = GPS fix
        - 2 = DGPS fix                              1       Data is from a GPS fix 
7       Number of Satellites                        05      5 Satellites are in view 
8       Horizontal Dilution of Precision (HDOP)     1.5     Relative accuracy of horizontal position 
9       Altitude                                    280.2, M 280.2 meters above mean sea level 
10      Height of geoid above WGS84 ellipsoid       -34.0, M -34.0 meters 
11      Time since last DGPS update                 blank   No last update 
12      DGPS reference station id                   blank   No station id 

*/
void nmeaProcessGPGGA(uint8_t* packet) {
    uint16_t i,j,k, len;
	double grad, minutes, dezimal_lat, dezimal_lon;
	char temp[20];    
    char *endptr;
    char field;
    char buf[MAX];

	// start parsing just after "GPGGA,"
	i = 6;
	for (field = 1; field < 13; field++) {        
        endptr = strchr(&packet[i], ',');
        len = (int)endptr - (int)&packet[i];
        
        strncpy(buf, &packet[i], len);
        buf[len] = '\0';
        switch (field) {
            case 2:
				if(len<4)  strcpy(GpsInfo.latitude, buf);
				else{
				temp[0]=buf[0];
				temp[1]=buf[1];
				temp[2]='\0';
				grad = atof(temp);
				temp[0]=buf[2];
				temp[1]=buf[3];
				temp[2]='.';
				temp[3]=buf[5];
				temp[4]=buf[6];
				temp[5]=buf[7];
				temp[6]='\0';
				minutes = atof(temp);
				dezimal_lat = grad + minutes / 60; 
				strncpy(buf, &packet[i+len+1], 1);
        		buf[1] = '\0';
				if (buf[0] == 'S')
        		dezimal_lat = -dezimal_lat;
			//	sprintf(GpsInfo.latitude, "%.6f", dezimal_lat); 
				dtostrf( dezimal_lat, 9, 6, GpsInfo.latitude ); 
				possets++;
				}               
                break;
            case 3:
                strcpy(GpsInfo.latitude_ns, buf);
                break;
            case 4:
				if(len<4)  strcpy(GpsInfo.longitude, buf);
				else{
				temp[0]=buf[0];
				temp[1]=buf[1];
				temp[2]=buf[2];
				temp[3]='\0';
				grad = atof(temp);
				temp[0]=buf[3];
				temp[1]=buf[4];
				temp[2]='.';
				temp[3]=buf[6];
				temp[4]=buf[7];
				temp[5]=buf[8];
				temp[6]='\0';
				minutes = atof(temp);
				dezimal_lon = grad + minutes / 60; 
				strncpy(buf, &packet[i+len+1], 1);
        		buf[1] = '\0';
				if (buf[0] == 'W')
        		dezimal_lon = -dezimal_lon;
				//sprintf(GpsInfo.longitude, "%.6f", dezimal_lon);
				dtostrf( dezimal_lon, 9, 6, GpsInfo.longitude );
				} 
                break;
            case 5:
                strcpy(GpsInfo.longitude_ew, buf);
                break;
            case 9:
                strcpy(GpsInfo.altitude, buf);             
        }
        buf[0] = '\0';
    	while(packet[i++] != ',');				// next field
    } 
	//if (dsets<10) dsets++;
}

//************************************************************************************************************
/*
    for NMEA 0183 version 3.00 active the Mode indicator field is added
    $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a,m*hh
Field #
1    = UTC time of fix
2    = Data status (A=Valid position, V=navigation receiver warning)
3    = Latitude of fix
4    = N or S of longitude
5    = Longitude of fix
6    = E or W of longitude
7    = Speed over ground in knots
8    = Track made good in degrees True
9    = UTC date of fix
10   = Magnetic variation degrees (Easterly var. subtracts from true course)
11   = E or W of magnetic variation
12   = Mode indicator, (A=Autonomous, D=Differential, E=Estimated, N=Data not valid)
13   = Checksum
*/
void nmeaProcessGPRMC(uint8_t* packet) {
    uint16_t i, len;
    char *endptr;
    char field;
    char buf[MAX];
	// start parsing just after "GPRMC,"
	i = 6;
	for (field = 1; field < 12; field++) {        
        endptr = strchr(&packet[i], ',');
        len = (int)endptr - (int)&packet[i];        // Hier Fehlfunktion, wenn & fehlt
        strncpy(buf, &packet[i], len);
        buf[len] = '\0';
        switch (field) {
            case 1:     // UTC-Uhrzeit
				if (len<4) strcpy(GpsInfo.utc_time, buf);
				else{
            	GpsInfo.utc_time[0]=buf[0];
            	GpsInfo.utc_time[1]=buf[1];
            	GpsInfo.utc_time[2]=':';
            	GpsInfo.utc_time[3]=buf[2];
            	GpsInfo.utc_time[4]=buf[3];
            	GpsInfo.utc_time[5]=':';
            	GpsInfo.utc_time[6]=buf[4];
            	GpsInfo.utc_time[7]=buf[5];
            	GpsInfo.utc_time[8]='\0';
				}
                break;
            case 9:     // UTC-Datum
				if (len<4) strcpy(GpsInfo.utc_date, buf);
				else{
            	GpsInfo.utc_date[0]='2';
            	GpsInfo.utc_date[1]='0';
            	GpsInfo.utc_date[2]=buf[4];
            	GpsInfo.utc_date[3]=buf[5];
            	GpsInfo.utc_date[4]='-';
            	GpsInfo.utc_date[5]=buf[2];
            	GpsInfo.utc_date[6]=buf[3];
            	GpsInfo.utc_date[7]='-';
            	GpsInfo.utc_date[8]=buf[0];
            	GpsInfo.utc_date[9]=buf[1];
            	GpsInfo.utc_date[10]='\0';
				}
                break;               
        }
        buf[0] = '\0';
    	while(packet[i++] != ',');				// next field
    }
    //if (dsets<10)	dsets++;   
}

//************************************************************************************************************
/*
$GPGSA,A,3,,,,15,17,18,23,,,,,,4.7,4.4,1.5*3F
       ^ ^ ^                   ^   ^   ^
       | | |                   |   |   |
       | | |                   |   |   VDOP (vertikale Genauigkeit)
       | | |                   |   |
       | | |                   |   HDOP (horizontale Genauigkeit)
       | | |                   |
       | | |                   PDOP (Genauigkeit)
       | | |
       | | PRN-Nummern von maximal 12 Satelliten
       | |
       | Art der Positionsbestimmung (3 = 3D-fix)
       |                             (2 = 2D-fix)
       |                             (1 = kein Fix)
       |
       Auto-Auswahl 2D oder 3D Bestimmung
*/
void nmeaProcessGPGSA(uint8_t* packet) {
    uint16_t i, len;
    char *endptr;
    char field;
    char buf[MAX];
	// start parsing just after "GPGSA,"
	i = 6;
	for (field = 1; field < 17; field++) {        
        endptr = strchr(&packet[i], ',');
        len = (int)endptr - (int)&packet[i];        // Hier Fehlfunktion, wenn & fehlt
        strncpy(buf, &packet[i], len);
        buf[len] = '\0';
        switch (field) {
            case 2:     // Fix
                strcpy(GpsInfo.fix, buf);
                break;            
        }
        buf[0] = '\0';
    	while(packet[i++] != ',');				// next field
    }
	//if (dsets<10) dsets++;   
}


void switch_logging(void){
uint8_t i, filenumber;
unsigned char filenamebuffer[4], buffer;

	if (inc_push==1){
		inc_push=0;
		if (!(Mode & MODE_LOGGING)){
			gps_on;
			StoreSecond=StoreCycle;
			bytecount=0;
		//	 if(GetDriveInformation()!=F_OK){ // get drive parameters
 		//		//Fehlermeldung
   		//		while(1);
 		//	 }

			for (i=0; i<3; i++) filenamebuffer[i]=filename[i+5]; filenamebuffer[3]='\0';
			filenumber=atoi(filenamebuffer);
           while(FindName(filename)==FULL_MATCH)
            {
			filenumber++;
			//sprintf(filenamebuffer, "%03u", filenumber);
			//dtostrf( filenumber, 3, 0, filenamebuffer );
			itoa (filenumber, filenamebuffer, 10);
			if (filenumber<10){
				filenamebuffer[3]='\0';
				filenamebuffer[2]=filenamebuffer[0];
				filenamebuffer[0]=filenamebuffer[1]='0';
			}
			else{
				if (filenumber<100){
					filenamebuffer[3]='\0';
					filenamebuffer[2]=filenamebuffer[1];
					filenamebuffer[1]=filenamebuffer[0];
					filenamebuffer[0]='0';
				}
			}
       		for (i=0; i<3; i++) filename[i+5]= filenamebuffer[i];
            }
			name_known=1;


 			result=Fopen(filename,F_WRITE);
			i=0;
		 	while (buffer=pgm_read_byte(&GpxHeader[i])){
				if (result==F_OK ){
	   	  			if(Fwrite(&buffer,1)!=1) result=F_ERROR;
	   	 			bytecount++;
					i++;
				}
			}
		//	bl_on;
			
			Mode |= MODE_LOGGING;
		}
		else{
			if (always_on=='0') gps_off;
			i=0;
			while (buffer=pgm_read_byte(&TrkEnd[i])){
				if (result==F_OK ){
	   	  			if(Fwrite(&buffer,1)!=1) result=F_ERROR;
	   	 			bytecount++;
					i++;
				}
			}
			Mode &= ~MODE_LOGGING;
			Fclose();
		}
	}
}





void store_trackpoint(void){
	uint8_t i;
	char buffer;



	write_string_buf(lat);
	write_string(GpsInfo.latitude);
	write_string_buf(endlat);
	write_string_buf(lon);
	write_string(GpsInfo.longitude);
	write_string_buf(endlon);
	write_string_buf(ele);
	write_string(GpsInfo.altitude);
	write_string_buf(endele);
	write_string_buf(time);
	write_string(GpsInfo.utc_date);
				buffer='T';
				if (result==F_OK ){
	   	  			if(Fwrite(&buffer,1)!=1) result=F_ERROR;
	   	 			bytecount++;
				}

	write_string(GpsInfo.utc_time);
				buffer='Z';
				if (result==F_OK ){
	   	  			if(Fwrite(&buffer,1)!=1) result=F_ERROR;
	   	 			bytecount++;
				}

	write_string_buf(endtime);


}

void write_string(char *stri){
uint8_t i;
char buffer;
			i=0;
			while (buffer=stri[i]){
				if (result==F_OK ){
	   	  			if(Fwrite(&buffer,1)!=1) result=F_ERROR;
	   	 			bytecount++;
					i++;
				}
			}
}

void write_string_buf(char *stri){
uint8_t i;
char buffer;
			i=0;
			while (buffer=pgm_read_byte(&stri[i])){
				if (result==F_OK ){
	   	  			if(Fwrite(&buffer,1)!=1) result=F_ERROR;
	   	 			bytecount++;
					i++;
				}
			}
}

void batt_empty(void){
gps_off;
cli();
Fclose();
red_on;
while(1);
}
