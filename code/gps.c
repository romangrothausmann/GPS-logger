//todo: replace pointers against array references for optimization!
//31: added flushing to FAT after each gga-log (VERY IMPORTANT SHOULD BATTERY RUN OUT OR BE DISCONNECTED WITHOUT STOPPING THE LOGGING!!!!!!) (iceland logs lost because this was missing:-(((()
//32: added parsing of RMC-record for date, direction and speed -> needs storing of recived data from other records, otherwise pointers after processed records will always point on last transmitted record (GGA???) (storing done already for GSV in gsv_sats)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
//#include <stdint.h>
#include <avr/io.h>
#include <inttypes.h>
#include <avr/eeprom.h>
#include <math.h>
#include <avr/interrupt.h> 
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "font6x8s.h" //defines prog_char Font [256] [6] 
#include "drehenc.h"  //defines inc_lr, inc_push, pressed

#include "mmc_spi.h"
#include "fat.h"
#include "dos.h"
#include "dir.h"
#include "find_x.h"


#define LCD_SI_PIN  PB2   //only used with manual lcd_write
#define LCD_SI_PORT PORTB //only used with manual lcd_write
#define LCD_CK_PIN  PB1   //only used with manual lcd_write
#define LCD_CK_PORT PORTB //only used with manual lcd_write
#define LCD_A0_PIN  PE4
#define LCD_A0_PORT PORTE
#define LCD_RS_PIN  PE3
#define LCD_RS_PORT PORTE
#define LCD_CS_PIN  PE2
#define LCD_CS_PORT PORTE 
#define LCD_PIXEL_BYTES 1024
#define lcd_sel() { LCD_CS_PORT&= ~(1 << LCD_CS_PIN); } //select chip
#define lcd_des() { LCD_CS_PORT|=  (1 << LCD_CS_PIN); } //deselect chip
//#define sbi(portn, bitn) asm volatile("sbi %0, %1" : : "I" (_SFR_IO_ADDR(portn)), "I" ((uint8_t)(bitn)))
//#define cbi(portn, bitn) asm volatile("cbi %0, %1" : : "I" (_SFR_IO_ADDR(portn)), "I" ((uint8_t)(bitn)))


#ifndef F_CPU
#define F_CPU 8000000L    // Systemclock,  L not UL!
#endif
 
#define BAUD 9600L        // Baudrate, L not UL!
 
// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever round
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Real baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD-1000) // Error in ppm 

#if ((BAUD_ERROR>10) || (BAUD_ERROR<-10))
  #error Systematic error: baudrate error > 1% and there for too high! Check if F_CPU is without U! 
#endif

#define U_RINGBUF_SIZE 80//128//1024

#define TIME   0
#define LAT1   1
#define LAT2   2
#define LON1   3
#define LON2   4
#define FIX    5
#define NSAT   6
#define HDOP   7
#define GEOID  8
#define WGS84  9
#define NOM   10 //GSV total number of messages
#define MNO   11 //GSV message number
#define SIV   12 //GSV number of sats in view
//#define 
#define DATE  13
#define SPEED 14
#define DIREC 15
#define GPS_DATA_MAX 16

#define GSV_MAX 469 //",nn,ee,aaa,ss"=13: x36 + 1 = 469; 31 sats now, changing

#define MAX_SATS 31 //never chage this unless the real sat # changed!
#define MAX_SAT_MAT 1//30

//Menu defines
#define INFO   0
#define GGA    1
#define SATV   2
#define GSV    3
#define CLOSEF 4
#define SENDF  5
#define UART   6
#define LCD    7
#define MENU_MAX 8

//gps_status 
#define ERROR    111
#define WAITING  1
#define NEW_MENU 2
#define DONE     3
#define BAD_CS   4
#define NEW_GGA  100  //gps protocols above 99!
#define NEW_GSV  101
#define NEW_RMC  102



typedef char** gps_t;

volatile uint8_t uart0_byte, uart0_rec, uart1_rec;
volatile uint16_t u1_ringbuf_index, u1_ringbuf_last_read;
volatile uint8_t u1_ringbuf[U_RINGBUF_SIZE];
//volatile unsigned long bytecount;


prog_char MenuStrs [8] [15] = { //max 21
    {"Info-Menu"},
    {"GP-Messages"},
    {"Satelite view"},
    {"Satelite list"},
    {"Change logging"},
    {"Transmit file"},
    {"Log to UART"},
    {"Light on/off"},
    };

//void display_gga(gps_t gps_data); 
void display_gga_rmc(gps_t gps_data); 
void zero_gps_data(char* s);

/*#######nice function but probably not optimizeable!!!#########
char setpin(char port, char pin, char state){
    switch(port){
    case 'a':
    case 'A':
    case 1:
        if (state)
            PORTA|= (1 << pin);
        else 
            PORTA&= ~(1 << pin);
        break;
    case 'b':
    case 'B':
    case 2:
        if (state)
            PORTB|= (1 << pin);
        else 
            PORTB&= ~(1 << pin);
        break;
    case 'c':
    case 'C':
    case 3:
        if (state)
            PORTC|= (1 << pin);
        else 
            PORTC&= ~(1 << pin);
        break;
    case 'd':
    case 'D':
    case 4:
        if (state)
            PORTD|= (1 << pin);
        else 
            PORTD&= ~(1 << pin);
        break;
    case 'e':
    case 'E':
    case 5:
        if (state)
            PORTE|= (1 << pin);
        else 
            PORTE&= ~(1 << pin);
        break;
    case 'f':
    case 'F':
    case 6:
        if (state)
            PORTF|= (1 << pin);
        else 
            PORTF&= ~(1 << pin);
        break;
    case 'g':
    case 'G':
    case 7:
        if (state)
            PORTG|= (1 << pin);
        else 
            PORTG&= ~(1 << pin);
        break;
    default:
        return(1);
        }
    return(0);
    }
*/
/*
uint8_t swap_nibble(uint8_t byte){
    return((byte << 4) || (byte >> 4));
    }
*/
/*
void lcd_sel(void){
//    setpin(LCD_CK_PORT, LCD_CK_PIN, 0);
    setpin(LCD_CS_PORT, LCD_CS_PIN, 0); //select chip
}

void lcd_des(void){
    setpin(LCD_CS_PORT, LCD_CS_PIN, 1); //deselect chip
    }
*/
/*
void lcd_write(uint8_t byte, uint8_t type){ 
//manual SPI, slower! DON'T forget to set DDRB, DDRE!!!
    char spcr;

    spcr= SPCR;
    SPCR= 0;

    char i;
    if (type)
        LCD_A0_PORT|= (1 << LCD_A0_PIN); //set A0
    else
        LCD_A0_PORT&= ~(1 << LCD_A0_PIN); 
    for (i= 7; i >= 0; i--){
        if (byte & (1 << i))
            LCD_SI_PORT|= (1 << LCD_SI_PIN);
        else
            LCD_SI_PORT&= ~(1 << LCD_SI_PIN); //prepare SI
        LCD_CK_PORT|= (1 << LCD_CK_PIN); //validate SI with rising edge
        LCD_CK_PORT&= ~(1  << LCD_CK_PIN);
        }
    SPCR= spcr;
    }
*/

void lcd_write(uint8_t byte, uint8_t type){
 //for internal SPI
    char spcr;

    spcr= SPCR;
    if (type) //set A0
        LCD_A0_PORT|=  (1 << LCD_A0_PIN);
    else
        LCD_A0_PORT&= ~(1 << LCD_A0_PIN);
    SPCR = (1<<SPE)|(1<<MSTR);//|(1<<SPR0); //f_cpu/4
    SPSR|= (1 << SPI2X); //SCK/2
    SPDR= byte;
    while(!(SPSR & (1<<SPIF)));
    SPCR= spcr;
    }

void lcd_command(uint8_t byte){ //only good if optimizeable!

    lcd_write(byte, 0);
    }

void lcd_data(uint8_t byte){ //only good if optimizeable!

    lcd_write(byte, 1);
    }


void lcd_init(void){
#define INIT_COM 14

    uint8_t init [] = { //change range limit INIT_COM when changing something here!!!
        0x40,
        0xA1,
        0xC0,
        0xA6,
        0xA2,
        0x2F,
        0xF8,
        0x00,
        0x27,
        0x81,
        0x16,
        0xAC,
        0x00,
        0xAF
        };
    uint8_t i;

    LCD_RS_PORT&= ~(1 << LCD_RS_PIN); //do the resetting first!!!
    _delay_ms(1);
    LCD_RS_PORT|=  (1 << LCD_RS_PIN);
    _delay_ms(1);
    lcd_sel();
    for (i= 0; i < INIT_COM; i++){ //sizeof(init)/sizeof(init[0]) //we'll do this statically for better performance
        lcd_command(init[i]);
        }
    lcd_des();
    }

void lcd_clear(void){
    uint16_t i;
    uint8_t j;

    lcd_sel();
    for (i= 0; i < 8; i++){
        lcd_command(0xB0 + i); //or |? //set page address
        lcd_command(0x10);     //set first column nibble
        lcd_command(0x00);     //set second column nibble
        for(j= 0; j < 128; j++)
            lcd_data(0);
        }
    lcd_command(0xB0);
    lcd_command(0x10);
    lcd_command(0x00);
    lcd_des();
    }

/*
char lcd_set_pixel(uint8_t x, uint8_t y, uint8_t on, uint8_t* matrix){
    uint8_t page, byte;

    if (x > 127 || y > 63)
        return(1);
    page= y / 8;
    if (on)
        byte= 1 << (y % 8);
    else
        byte= ~(1 << (y % 8));
    lcd_command(0xB0 + page); //or |?//set page address
    lcd_command(0x10 + ((0xF0 & x) >> 4)); //set first column nibble
    lcd_command(0x00 + (0x0F & x)); //set second column nibble
    lcd_data(byte);
    }
*/

char lcd_set_pixel(uint8_t x, uint8_t y, uint8_t inv, uint8_t* m){ 
    //only sets pixel in matrix, write matrix when all set

    uint16_t i;

    if (x > 127 || y > 63)
        return(0);
    i= y / 8 * 128 + x; //no cast necessary because y and 8 are integers
    if (inv)
        m[i]&= ~(1 << (y % 8));
    else
        m[i]|= 1 << (y % 8);
    return(1);
    }

void lcd_set_abs_pixel(uint16_t i, uint8_t inv, uint8_t* m){ 
    //only sets absolut pixel in matrix, write matrix when all set

    uint8_t x,y;

    x= i % 128;
    y= i / 128;
    lcd_set_pixel(x, y, inv, m);
    }

char lcd_set_byte(uint8_t byte, uint8_t col, uint8_t page, uint8_t inv, uint8_t trans, uint8_t* m){ 
    //only sets byte in matrix, write matrix when all set

    uint16_t i;

    if (col > 127 || page > 63)
        return(0);
    i= page * 128 + col; 
    if (inv) {
        if (trans)
            m[i]&= ~(byte);
        else 
            m[i]= ~(byte);
        }
    else {
        if (trans)
            m[i]|= byte;
        else 
            m[i]= byte;
        }
    return(1);
    }

char lcd_iset_byte(uint8_t byte, uint8_t col, uint8_t page, uint8_t inv){ 
    //sets byte instantly on lcd

    uint16_t i;

    lcd_sel();
    if (col > 127 || page > 63)
        return(0);
    i= page * 128 + col; 
    lcd_command(0xB0 + page); 
    lcd_command(0x10 + ((0xF0 & col) >> 4)); //set first column nibble
    lcd_command(0x00 + (0x0F & col)); //set second column nibble
    if (inv) 
        lcd_data(~(byte));
    else 
        lcd_data(byte);
    lcd_des();
    return(1);
    }

void lcd_clear_line(uint8_t col, uint8_t page, char inv){

    while (col < 128){
        lcd_iset_byte(0, col, page, inv);
        col++;
        }
    }

void lcd_write_matrix(uint8_t* m){
    uint16_t i;
    uint8_t page, col;

    lcd_sel();
    for (i= 0; i < LCD_PIXEL_BYTES; i++){
        page= i / 128;
        col=  i % 128;
        lcd_command(0xB0 + page); //or |?//set page address
        if (!col){ //this needs to be done because col goes up to 131
            lcd_command(0x10); //set first column nibble
            lcd_command(0x00); //set second column nibble
            }
        //lcd_command(0x10 + ((0xF0 & col) >> 4)); //set first column nibble
        //lcd_command(0x00 + (0x0F & col)); //set second column nibble
        lcd_data(m[i]);//this increases col!
        }
    lcd_des();
    }

uint8_t* lcd_fw_matrix(uint8_t* n, uint8_t* o){ //uint8_t* is a pointer of uint8_t!!! so this is fine;)
    uint16_t i;
    uint8_t page, col;

    //find first col and page with difference and so on...
    lcd_sel();
    for (i= 0; i < LCD_PIXEL_BYTES; i++){
        if(n[i] != o[i]){
            page= i / 128;
            col=  i % 128;
            lcd_command(0xB0 + page); //or |?//set page address
            lcd_command(0x10 + ((0xF0 & col) >> 4)); //set first column nibble
            lcd_command(0x00 + (0x0F & col)); //set second column nibble
            lcd_data(n[i]); //implement auto inc useage here!
            o[i]= n[i];
            }
        }    
    lcd_des();
    return(o);
    }

uint8_t* lcd_randomize_matrix(uint8_t* m){
    uint16_t i;
    for (i= 0; i < LCD_PIXEL_BYTES; i++)
        m[i]= (uint8_t) (rand() / (double) RAND_MAX * 256);
    return(m);
    }

char lcd_putchar(uint8_t c, uint8_t col, uint8_t page, uint8_t inv, uint8_t trans, uint8_t* m){
//a char within a page, needs a write afterwards! return absolute cols

    uint8_t end;

    if (c < 32 )
        return(col);
    end= 127 - col < FONT_WIDTH ? 127 - col : col + FONT_WIDTH; //check if to draw over the border, for running text;)
    for (; col < end; col++)
        lcd_set_byte(pgm_read_byte(&Font[c][FONT_WIDTH - end + col]), col, page, inv, trans, m); //_far needed???
    return(col);
    }

char lcd_iputchar(uint8_t c, uint8_t col, uint8_t page, uint8_t inv){
//puts char instantly on lcd, returns absolute col

    uint8_t end;

    if (c < 32 )//does this work with char as well???
        return(col);
    end= 127 - col < FONT_WIDTH ? 127 - col : col + FONT_WIDTH; //check if to draw over the border, for running text;)
    for (; col < end; col++)
        lcd_iset_byte(pgm_read_byte(&Font[c][FONT_WIDTH - end + col]), col, page, inv); //_far needed???
    return(col);
    }

char lcd_iputmchar(char* c, uint8_t num, uint8_t col, uint8_t page, uint8_t inv){
//puts num chars instantly on lcd, returns absolute col
    uint8_t i;

    for(i= 0; i < num; i++)
        col= lcd_iputchar(c[i], col, page, inv);
    return(col);
    }

uint8_t lcd_write_str(char* c, uint8_t col, uint8_t page, uint8_t inv, uint8_t trans, uint8_t clear, uint8_t* m){//returns absolute col!
//display a string, needs no write, use' \n'!
    uint8_t colc;
    char* oc;
    oc= c; //for 6x8 there can be 168 full chars on the display

    while (*oc) {
        if (*oc == '\n'){
            page++;
            col= 0;
            oc++;
            }
        //more controle keys need to be implemented:(

        if (col > 127 - FONT_WIDTH){//line wrap, only full letters here!
            col= 0;
            page++;
            if (*oc == ' ')//skip space if line wraped
                oc++;
            }
        if (page > 7)
            break;
        col= lcd_putchar(*oc, col, page, inv, trans, m);
        oc++;
        }
    if(clear){
        colc= col;
        while (colc < 128){
            lcd_set_byte(0, colc, page, inv, 0, m);
            colc++;
            }
        }
    lcd_write_matrix(m);
    return(col);//((oc - c));
    }

uint8_t lcd_iwrite_str(const char* c, uint8_t col, uint8_t page, uint8_t inv, uint8_t clear ){//returns absolute cols [//returns displayed chars]
//display a string, needs no write, use' \n'!
    uint8_t colc;
    const char* oc;
    oc= c; //for 6x8 there can be 168 full chars on the display

    while (*oc) {
        if (*oc == '\n'){
            page++;
            col= 0;
            oc++;
            }
        //more controle keys need to be implemented:(

        if (col > 127 - FONT_WIDTH){//line wrap, only full letters here!
            col= 0;
            page++;
            if (*oc == ' ')//skip space if line wraped
                oc++;
            }
        if (page > 7)
            break;
        col= lcd_iputchar(*oc, col, page, inv);
        oc++;
        }
    if(clear){
        colc= col;
        while (colc < 128){
            lcd_iset_byte(0, colc, page, inv);
            colc++;
            }
        }
    return(col);//(oc - c);
    }

uint8_t lcd_iwrite_strp(const char* s, uint8_t col, uint8_t page, uint8_t inv, uint8_t clear ){//returns absolute cols [//returns displayed chars]
//display a string, needs no write, use' \n'!
    uint8_t colc;
    const char* oc;
    char c;

    oc= s; //for 6x8 there can be 168 full chars on the display
    while((c= pgm_read_byte(oc))){
        if (c == '\n'){
            page++;
            col= 0;
            oc++;
            }
        //more controle keys need to be implemented:(

        if (col > 127 - FONT_WIDTH){//line wrap, only full letters here!
            col= 0;
            page++;
            if (c == ' ')//skip space if line wraped
                oc++;
            }
        if (page > 7)
            break;
        col= lcd_iputchar(c, col, page, inv);
        oc++;
        }
    if(clear){
        colc= col;
        while (colc < 128){
            lcd_iset_byte(0, colc, page, inv);
            colc++;
            }
        }
    return(col);//(oc - c);
    }
/*
void lcd_drawchar(char c, uint8_t x, uint8_t y, uint8_t trans, uint8_t* m){//a char at x,y

    }
*/
void lcd_4way_sym(uint8_t cx, uint8_t cy, uint8_t x, uint8_t y, char inv, uint8_t* m){
    if (x == 0) {
        lcd_set_pixel(cx, cy + y, inv, m);
        lcd_set_pixel(cx, cy - y, inv, m);
        } 
    else if (y == 0) {
        lcd_set_pixel(cx + x, cy, inv, m);
        lcd_set_pixel(cx - x, cy, inv, m);
        } 
    else {
        lcd_set_pixel(cx + x, cy + y, inv, m);
        lcd_set_pixel(cx - x, cy + y, inv, m);
        lcd_set_pixel(cx + x, cy - y, inv, m);
        lcd_set_pixel(cx - x, cy - y, inv, m);
        }
    }

void lcd_8way_sym(uint8_t cx, uint8_t cy, uint8_t x, uint8_t y, char inv, uint8_t* m){
    if (x == 0) {
        lcd_set_pixel(cx, cy + y, inv, m);
        lcd_set_pixel(cx, cy - y, inv, m);
        lcd_set_pixel(cx + y, cy, inv, m);
        lcd_set_pixel(cx - y, cy, inv, m);
        } 
    else if (x == y) {
        lcd_set_pixel(cx + x, cy + y, inv, m);
        lcd_set_pixel(cx - x, cy + y, inv, m);
        lcd_set_pixel(cx + x, cy - y, inv, m);
        lcd_set_pixel(cx - x, cy - y, inv, m);
        } 
    else if (x < y) {
        lcd_set_pixel(cx + x, cy + y, inv, m);
        lcd_set_pixel(cx - x, cy + y, inv, m);
        lcd_set_pixel(cx + x, cy - y, inv, m);
        lcd_set_pixel(cx - x, cy - y, inv, m);
        lcd_set_pixel(cx + y, cy + x, inv, m);
        lcd_set_pixel(cx - y, cy + x, inv, m);
        lcd_set_pixel(cx + y, cy - x, inv, m);
        lcd_set_pixel(cx - y, cy - x, inv, m);
        }
    }

void lcd_circle(uint8_t r, uint8_t cx, uint8_t cy, char inv,  uint8_t* m){
    uint8_t x, y; //so r < 256
    int p; //has to hold -2r up to sqrt(2)/2*r

    x= 0;
    y= r;
    p= (5 - r*4)/4;

    lcd_4way_sym(cx, cy, x, y, inv, m);
    while (x < y) {
        x++;
        if (p < 0) {
            p+= 2*x+1;
            } 
        else {
            y--;
            p+= 2*(x-y)+1;
            }
        lcd_4way_sym(cx, cy, x, y, inv, m);
        }
    }

void lcd_elips(uint8_t a, uint8_t b, uint8_t mx, uint8_t my, char inv,  uint8_t* m){
    int x,  mx1,mx2,  my1,my2;
    long aq,bq, dx,dy, r,rx,ry;

    lcd_set_pixel(mx + a, my, inv, m);
    lcd_set_pixel(mx - a, my, inv, m);

    mx1= mx - a;
    my1= my;
    mx2= mx + a;
    my2= my;

    aq= a * a; //       {calc sqr}
    bq= b * b;
    dx= aq << 1;//               {dx= 2 * a * a}
    dy= bq << 1;//               {dy= 2 * b * b}
    r = a * bq; //                {r = a * b * b}
    rx= r << 1;//                {rx= 2 * a * b * b}
    ry= 0;//                      {because y = 0}
    x= a;

    while (x > 0){
        if (r > 0){
            // { y + 1 }
            my1++;
            my2--;
            ry+= dx;
            r-= ry;
            }
        if (r <= 0){
            // { x - 1 }
            x--;
            mx1++;
            mx2--;
            rx-= dy;
            r+= rx; 
            }
        lcd_set_pixel(mx1, my1, inv, m);
        lcd_set_pixel(mx1, my2, inv, m);
        lcd_set_pixel(mx2, my1, inv, m);
        lcd_set_pixel(mx2, my2, inv, m);
        }
    }
/*
void lcd_elips(uint8_t a, uint8_t b, uint8_t cx, uint8_t cy, char inv,  uint8_t* m){
    float x, y; //so r < 256
    int p; //has to hold -2r up to sqrt(2)/2*r

    x= 0;
    y= b;
    p= (5 - b * 4) / 4;

    lcd_4way_sym(cx, cy, x, y, inv, m);
    while (x < y) {
        x++;
        if (p < 0) {
            p+= (2 * x + 1) / a;
            } 
        else {
            y--;
            p+= 2 * x / a + 1. / a - 2 * y / b + 1. / b;
            }
        lcd_4way_sym(cx, cy, x, y, inv, m);
        }
    while (y < x) {
        y--;
        if (p < 0) {
            p+= (2 * x + 1) / a;
            } 
        else {
            y--;
            p+= 2 * x / a + 1. / a - 2 * y / b + 1. / b;
            }
        lcd_4way_sym(cx, cy, x, y, inv, m);
        }

    }
*/

/* gatopeich's Line Algorithm with integer operations */
/* http://gatopeichs.pbwiki.com/gatopeich%27s%20Graphic%20Algorithms*/

void lcd_line(int x1, int y1, int x2, int y2, char inv,  uint8_t* m){
    int dx = x2-x1;
    if(dx<0) dx = -dx;
    dx++;
    int dy = y2-y1;
    if(dy<0) dy = -dy;
    dy++;

	if( dx >= dy ) /* Base case: dx >= dy */
    {
        if(x1>x2) { /* we want to traverse left-right */
            int t=x1; x1=x2; x2=t;
            t=y1; y1=y2; y2=t;
        }
        int a = dx;
        a -= dy/2; /* better symmetry */
        int y_inc = (y2 >= y1) ? 1 : -1;
        do {
            do {
                lcd_set_pixel(x1, y1, inv, m);
                a -= dy;
            } while ( a > 0 );
            a += dx;
            if(y1==y2) break;
            y1 += y_inc;
        } while(1);
    }
    else /* Transposed case: dy > dx */
    {
        if(y1>y2) { /* we want to traverse up-down */
            int t=x1; x1=x2; x2=t;
            t=y1; y1=y2; y2=t;
        }
        int a = dy;
        a -= dx/2; /* better symmetry */
        int x_inc = (x2 >= x1) ? 1 : -1;
        do {
            do {
                lcd_set_pixel(x1, y1, inv, m);
                a -= dx;
            } while ( a > 0 );
            a += dy;
            if(x1==x2) break;
            x1 += x_inc;
        } while(1);
    }
}


void clear_matrix(uint8_t* m){
    uint16_t i;
    
    for (i= 0; i < LCD_PIXEL_BYTES; i++) //clear matrix
        m[i]= 0;
    }

void uart0_init(uint16_t baud){//51 for 9600bps
//    UBRR0H = (unsigned char)(baud >> 8);
//    UBRR0L = (unsigned char)baud;
    UBRR0H = baud >> 8;
    UBRR0L = baud & 0xFF;
    UCSR0C|= (3<<UCSZ00);//async, 8N1, see also UCSZn2! (1<<UMSEL0) sync
    UCSR0B|= (1<<TXEN0); //enable recieve interrupt
    }

void uart1_init(uint16_t baud){//51 for 9600bps
//    UBRR0H = (unsigned char)(baud >> 8);
//    UBRR0L = (unsigned char)baud;
    UBRR1H = baud >> 8;
    UBRR1L = baud & 0xFF;
    UCSR1C|= (3<<UCSZ10);//async, 8N1, see also UCSZn2! (1<<UMSEL0) sync
    UCSR1B|= (1<<RXCIE1)|(1<<RXEN1)|(1<<TXEN1); //enable recieve interrupt

    u1_ringbuf_last_read= 0;
//    u1_ringbuf_finished= 0;
    u1_ringbuf_index= 0; //start on 1! 0 means round done! NO! index can change before buf got processed!
    uart1_rec= 0; //this counts how many chars are unread of the buffer!
    }

uint8_t uart0_Rx(void){
    while (!(UCSR0A & (1<<RXC0)))   // warten bis Zeichen verfuegbar
        ;
    return UDR0;                   // Zeichen aus UDR an Aufrufer zurueckgeben
    }

uint8_t uart1_Rx(void){
    while (!(UCSR1A & (1<<RXC1)))   // warten bis Zeichen verfuegbar
        ;
    return UDR1;                   // Zeichen aus UDR an Aufrufer zurueckgeben
    }

char uart1_getchar(void){

    uart1_rec--; //check before uart1_getchar() if uart1_rec > 0 !!!
    //if (u1_ringbuf_last_read > U_RINGBUF_SIZE - 1)
    //    u1_ringbuf_last_read= 0;
    (u1_ringbuf_last_read > U_RINGBUF_SIZE - 2) ? u1_ringbuf_last_read= 0 : u1_ringbuf_last_read++;
    return(u1_ringbuf[u1_ringbuf_last_read]);
    //return(u1_ringbuf_finished || u1_ringbuf_last_read < u1_ringbuf_index ? u1_ringbuf[u1_ringbuf_last_read++] : 0);
    }

void uart0_write_str(char* s){
    while(*s){
        while (!(UCSR0A & (1<<UDRE0))) {}
        UDR0=*s;
        s++;
        }
    while (!(UCSR0A & (1<<UDRE0))) {}
    UDR0='\r';
    while (!(UCSR0A & (1<<UDRE0))) {}
    UDR0='\n';
    while(!(UCSR0A & (1<<TXC0))) {}        
    }

char gps_process_gsv(char* const o, gps_t gps_data, char* const gsv_sats){//, uint8_t sat_matrix [][MAX_SAT_MAT][2]) {
//returns total number of processed sats 
    char* s, * t;
    uint8_t siv, sat_num;
    const char * nums;
    char elo;
    uint8_t num, snr, x, y, sat= 0;
    int azi; //never ever again change this to unsigned or do a cast!!!

#define MAX_R 31
#define CX 37
#define CY 31

    s= o; //keep original pointer unchanged!
    s= gps_data[NOM]= strchr(s, ',') + 1;
    s= gps_data[MNO]= strchr(s, ',') + 1;
    s= gps_data[SIV]= strchr(s, ',') + 1;
    s= strchr(s, ','); //set beginning of specific data with ','!
    //*strchr(s, '*')= 0; //terminate
    t= strrchr(s, '*'); //ends with ",*" if no SNR sent!
    *t= 0;

    if(*gps_data[MNO] == '1')
        *gsv_sats= 0;
    //check for space?
    strcat(gsv_sats, s);
    //uart0_write_str(gsv_sats);
    if(*gps_data[NOM] != *gps_data[MNO]){
        return(0);
        }
        
    siv= atoi(gps_data[SIV]);

    s= gsv_sats;
    sat_num= 0;
    while((s= strchr(s, ','))){
        sat_num++;
        s++;
        }
    if(sat_num / 4 != siv){ //this should be removed later!
        lcd_iwrite_str("sat_num != siv", 0, 6, 1, 0);
        return(0);
        }

    s= gsv_sats;
    s= strchr(s, ',') + 1;
    while(*s){
        sat++;
        //s= strchr(s, ',') + 1;
        num= atoi(s) - 1;
        nums= s;
        s= strchr(s, ',') + 1;
        elo= atoi(s);
        s= strchr(s, ',') + 1;
        azi= atoi(s);
        s= strchr(s, ',') + 1;
        snr= atoi(s);
        s= strchr(s, ',') + 1;
        x= ((elo - 90) * MAX_R / 90. * sin(-azi / 180. * M_PI)) * 1.2 + CX;
        y=  (elo - 90) * MAX_R / 90. * cos(-azi / 180. * M_PI);
        y+= CY; //separately because of rounding error
/*
        if(sat_matrix[num][sat_matrix[num][0][0]][0] != x || sat_matrix[num][sat_matrix[num][0][0]][1] != y){
            sat_matrix[num][0][0]++;//0 has the current position!
            if(sat_matrix[num][0][0] >= MAX_SAT_MAT)
                sat_matrix[num][0][0]= 1; //0 has the current position!
            sat_matrix[num][sat_matrix[num][0][0]][0]= x;
            sat_matrix[num][sat_matrix[num][0][0]][1]= y;
            //lcd_write_str("Trace!", 90, 4, 0, 1, 0, n);
            }
*/
        }
    return(siv); 
    }

void gps_display_gsv(char* sivs, char* const gsv_sats, uint8_t tsat) {
    uint8_t col= 0, i, j, siv;
    char s[4];
    char* o;//const

    col= lcd_iputmchar(sivs, 2, col, 0, 0);
//    col= lcd_iwrite_str(itoa(sat_num, s, 10),  col, 0, 0, 0);
    col= lcd_iwrite_str(": S# EL AZI SNR", col, 0, 0, 0);


    o= gsv_sats;
    siv= atoi(sivs);
    tsat+= 256 / siv / 2 * siv; //set tsat to mid 256 so 0->255 break is far away
    for(j= 0; j < tsat % siv; j++)
        for(i= 0; i < 4; i++)
            o= strchr(o, ',') + 1;
    j= tsat % siv;

    for(i= 0; i < 7; i++, j++){
        col= 0;
        if(j >= siv)
            j= 0;
        if(j < 10) //right align
            col= lcd_iputchar(' ', col, i + 1 , 0);
        col= lcd_iwrite_str(itoa(j, s, 10), col, i + 1, 0, 0);
        col= lcd_iwrite_str(": ", col, i + 1 , 0, 0); //better 2x iputchar???
        o= strchr(o, ',') + 1;
        if(!*o)//from the beginning
            o= gsv_sats + 1;
        col= lcd_iputmchar(o, 2, col, i + 1, 0);
        col= lcd_iputchar(' ', col, i + 1 , 0); //this could go in a for loop
        o= strchr(o, ',') + 1;
        col= lcd_iputmchar(o, 2, col, i + 1, 0);
        col= lcd_iputchar(' ', col, i + 1 , 0);
        o= strchr(o, ',') + 1;
        col= lcd_iputmchar(o, 3, col, i + 1, 0);
        col= lcd_iputchar(' ', col, i + 1 , 0);
        o= strchr(o, ',') + 1;
        if(*o == ',' || !*o)//SNR not always sent!
            lcd_clear_line(col, i + 1, 0);
        else
            col= lcd_iputmchar(o, 2, col, i + 1, 0);
        }
    }

void gps_process_gga(char* const o, gps_t gps_data){
    /* $GPGGA,HHMMSS.ss,BBBB.BBBB,b,LLLLL.LLLL,l,Q,NN,D.D,H.H,h,G.G,g,A.A,RRRR*PP */

    char* s;

    s= o; //keep original pointer unchanged!
    //s= gps_data[TIME]= strchr(s, ',') + 1;
    s= strchr(s, ',') + 1;//skip HHMMSS
/*     s= gps_data[LAT1]= strchr(s, ',') + 1; */
/*     s= gps_data[LAT2]= strchr(s, ',') + 1; */
/*     s= gps_data[LON1]= strchr(s, ',') + 1; */
/*     s= gps_data[LON2]= strchr(s, ',') + 1; */
    s= strchr(s, ',') + 1;//skip LAT
    s= strchr(s, ',') + 1;//skip N|S
    s= strchr(s, ',') + 1;//skip LON
    s= strchr(s, ',') + 1;//skip E|W
    s= gps_data[FIX]= strchr(s, ',') + 1; //Q: GPS-Qualität: 0 für ungültig, 1 für GPS fix, 2 für DGPS fix, 6 für geschätzt (nur bei NMEA-0183 ab Version 2.3)
    s= gps_data[NSAT]= strchr(s, ',') + 1;
    s= gps_data[HDOP]= strchr(s, ',') + 1;//D.D: horizontale Abweichung (dilution of precision)
    s= gps_data[GEOID]= strchr(s, ',') + 1;//H.H: Höhe der Antenne über Geoid oder MSL (mean sea level)
    s++; //skip //h: Einheit der Antennenhöhe (Meter)
    s= gps_data[WGS84]= strchr(s, ',') + 1;//G.G: geoidal separation
    }

void gps_process_rmc(char* const o, gps_t gps_data){
    /* $GPRMC,162614.00,A,5230.5900,N,01322.3900,E,10.0,90.0,131006,1.2,E,A*13 */
    /* $GPRMC,HHMMSS.ss,A,BBBB.BBBB,b,LLLLL.LLLL,l,GG.G,RR.R,DDMMYY,M.M,m,F*PP */

    char* s;

    s= o; //keep original pointer unchanged!
    s= gps_data[TIME]= strchr(s, ',') + 1;
    //s= strchr(s, ',') + 1;//skip HHMMSS.ss
    //s= strchr(s, ',') + 1;//skip A
    s++;//skip A
    s= gps_data[LAT1]= strchr(s, ',') + 1;
    s= gps_data[LAT2]= strchr(s, ',') + 1;
    s= gps_data[LON1]= strchr(s, ',') + 1;
    s= gps_data[LON2]= strchr(s, ',') + 1;
/*     s= strchr(s, ',') + 1;//skip LAT */
/*     s= strchr(s, ',') + 1;//skip N|S */
/*     s= strchr(s, ',') + 1;//skip LON */
/*     s= strchr(s, ',') + 1;//skip E|W */
    s= gps_data[SPEED]= strchr(s, ',') + 1;
    s= gps_data[DIREC]= strchr(s, ',') + 1;
    s= gps_data[DATE]= strchr(s, ',') + 1;
    //disgard rest of record
    }

char gps_det_type(char* s, char* t){
    char i;

    for (i= 0; i < 5; i++)
        if (*s++ != *t++)
            return(0);
    return(1);
    }

void display_time(const char* time, uint8_t col, uint8_t page){
    char* s;

    s= time;
    col= lcd_iwrite_str(s, col, page, 0, 0);
/*     col= lcd_iputchar(*s++, col, page, 0); */
/*     col= lcd_iputchar(*s++, col, page, 0); */
/*     col= lcd_iputchar(':', col, page, 0); */
/*     col= lcd_iputchar(*s++, col, page, 0); */
/*     col= lcd_iputchar(*s++, col, page, 0); */
/*     col= lcd_iputchar(':', col, page, 0); */
/*     col= lcd_iputchar(*s++, col, page, 0); */
/*     col= lcd_iputchar(*s++, col, page, 0); */
    //skip fraction of s
    }

void display_date(const char* date, uint8_t col, uint8_t page){
    char* s;

    s= date;
    col= lcd_iwrite_str(s, col, page, 0, 0);
    //col= lcd_iputchar(s[4], col, page, 0);
    //col= lcd_iputchar(s[5], col, page, 0);
    //col= lcd_iputchar('.', col, page, 0);
    //col= lcd_iputchar(s[2], col, page, 0);
    //col= lcd_iputchar(s[3], col, page, 0);
    //col= lcd_iputchar('.', col, page, 0);
    //col= lcd_iputchar(s[0], col, page, 0);
    //col= lcd_iputchar(s[1], col, page, 0);
    //skip fraction of s
    }

void display_speed(const char* s, uint8_t col, const uint8_t page){

    lcd_iwrite_str(s, col, page, 0, 0);
    }

void display_direction(const char* s, uint8_t col, const uint8_t page){

    lcd_iwrite_str(s, col, page, 0, 0);
    }

void display_lat(const char* lat1, const char* lat2, uint8_t col, const uint8_t page){
    const char* s;
    char sec_s[9]; //to contain "%6.3f" + \0
    float sec; 

    s= lat1;
    col= lcd_iputchar('0', col, page, 0);//pad with 0
    col= lcd_iputchar(*s++, col, page, 0);
    col= lcd_iputchar(*s++, col, page, 0);
    col= lcd_iputchar('°', col, page, 0);
    col= lcd_iputchar(*s++, col, page, 0);
    col= lcd_iputchar(*s++, col, page, 0);
    col= lcd_iputchar('\'', col, page, 0);
    s++;//skip decimal point
    memcpy(sec_s, s, 5); //always 4 digits reported after decimal point
    sec_s[5]= 0;
//    strncpy(sec_s, s, 5);//does this set sec_s[5]= 0 ???
    sec= atol(sec_s) * 60 / 100000.0;
    dtostrf(sec, 5, 3, sec_s);
    ////check if padding with 0 is necessary
    if (sec < 9.9995) //because dtostrf rounds! pre up to 9.999499!  < 10)
        col= lcd_iputchar('0', col, page, 0); //to avoid sprintf
//    sprintf(sec_s, "%6.3f", sec);
//    ltoa((long)sec, sec_str, 10);
    col= lcd_iwrite_str(sec_s, col, page, 0, 0);
    col= lcd_iputchar('"', col, page, 0);
    lcd_iputchar(*lat2, col, page, 0);
    }

void display_lon(const char* lon1, const char* lon2, uint8_t col, const uint8_t page){
    const char* s;
    char sec_s[9];
    float sec; 

    s= lon1;
    col= lcd_iputchar(*s++, col, page, 0);
    col= lcd_iputchar(*s++, col, page, 0);
    col= lcd_iputchar(*s++, col, page, 0);
    col= lcd_iputchar('°', col, page, 0);
    col= lcd_iputchar(*s++, col, page, 0);
    col= lcd_iputchar(*s++, col, page, 0);
    col= lcd_iputchar('\'', col, page, 0);
    s++;//skip decimal point
    memcpy(sec_s, s, 5);
    sec_s[5]= 0;
//    strncpy(sec_s, s, 5);//does this set sec_s[5]= 0 ???
    //lcd_iwrite_str(sec_s, 0, 3, 0, 0);
    sec= atol(sec_s) * 60 / 100000.0;
    dtostrf(sec, 5, 3, sec_s);
    if (sec < 9.9995) //because dtostrf rounds!  < 10)
        col= lcd_iputchar('0', col, page, 0); //to avoid sprintf
//    sprintf(sec_s, "%6.3f", sec);
//    ltoa((long)sec, sec_s, 10);
    col= lcd_iwrite_str(sec_s, col, page, 0, 0);
    col= lcd_iputchar('"', col, page, 0);
    lcd_iputchar(*lon2, col, page, 0);
    }

void display_nsat(const char* s, uint8_t col, const uint8_t page){

    lcd_iwrite_str(s, col, page, 0, 1);
    }

void display_geoid(const char* s, const uint8_t col, const uint8_t page){
    
    lcd_iwrite_str(s, col, page, 0, 1);
    }

/* void display_gga(gps_t gps_data){ */
/*     if(*gps_data[TIME]) */
/*         display_time(gps_data[TIME], 0, 0); */
/*     if(*gps_data[LAT1] && *gps_data[LAT2]) */
/*         display_lat(gps_data[LAT1], gps_data[LAT2], 0, 1); */
/*     if(*gps_data[LON1] && *gps_data[LON2]) */
/*         display_lon(gps_data[LON1], gps_data[LON2], 0, 2); */
/*     if(*gps_data[NSAT]) */
/*         display_nsat(gps_data[NSAT], 10 * FONT_WIDTH, 0); */
/*     if(*gps_data[GEOID]) */
/*         display_geoid(gps_data[GEOID], 13 * FONT_WIDTH, 0); */
/*        } */

void display_gga_rmc(gps_t gps_data){
    if(*gps_data[TIME])
        display_time(gps_data[TIME], 0, 0);
    if(*gps_data[DATE])
        display_date(gps_data[DATE], 15 * FONT_WIDTH, 0);
    //if(*gps_data[NSAT])
    //    display_nsat(gps_data[NSAT], 18 * FONT_WIDTH, 0);
    //if(*gps_data[GEOID])
    //    display_geoid(gps_data[GEOID], 21 * FONT_WIDTH, 0);
    if(*gps_data[LAT1] && *gps_data[LAT2])
        display_lat(gps_data[LAT1], gps_data[LAT2], 0, 1);
    if(*gps_data[LON1] && *gps_data[LON2])
        display_lon(gps_data[LON1], gps_data[LON2], 0, 2);
    if(*gps_data[SPEED])
        display_speed(gps_data[SPEED], 0, 3);
    if(*gps_data[DIREC])
        display_direction(gps_data[DIREC], 4 * FONT_WIDTH, 3);
    if(*gps_data[GEOID])
        display_geoid(gps_data[GEOID], 0, 4);
       }

void write_str(char* s, char c){
    uint8_t len, count;

    len= strlen(s);
    if((count= Fwrite((uint8_t*)s,len)) != len)
        lcd_iwrite_str("Write error!", 0, 4, 0, 0);
    //else
//    bytecount+= count;
    if(Fwrite((uint8_t*)&c,1) != 1)
        lcd_iwrite_str("Write error of char!", 1 * FONT_WIDTH, 5, 0, 0);
    //else
//    bytecount++;
    }


void log_gga(gps_t gps_data){
    char tmp[15], * s;
    uint8_t fdeg;
    float deg;

    if(*gps_data[LAT1]){
        strncpy(tmp, gps_data[LAT1], 2);
        tmp[2]= 0;
        //lcd_iwrite_str(tmp, 0, 4, 0, 0);
        fdeg= atoi(tmp);
        strncpy(tmp, gps_data[LAT1] + 2, 7);
        tmp[7]= 0;
        //lcd_iwrite_str(tmp, 0, 5, 0, 0);
        deg= atof(tmp); //minutes
        deg= fdeg + deg / 60;
        if(*gps_data[LAT2] == 'S')
            deg= -deg;
        dtostrf(deg, 4, 9, tmp);
        //lcd_iwrite_str(tmp, 0, 4, 0, 0);
        write_str(tmp, ',');//one sign, two before . and nine digits afterwards
        }
    else
        write_str(NULL, ',');
    if(*gps_data[LON1]){
        strncpy(tmp, gps_data[LON1], 3);
        tmp[3]= 0;
        //lcd_iwrite_str(tmp, 0, 4, 0, 0);
        fdeg= atoi(tmp);
        strncpy(tmp, gps_data[LON1] + 3, 7);
        tmp[7]= 0;
        //lcd_iwrite_str(tmp, 0, 5, 0, 0);
        deg= atof(tmp); //minutes
        deg= fdeg + deg / 60;
        if(*gps_data[LON2] == 'W')
            deg= -deg;
        dtostrf(deg, 5, 9, tmp);
        //lcd_iwrite_str(tmp, 0, 5, 0, 0);
        write_str(tmp, ',');//one sign, three before . and nine digits afterwards
        }
    else
        write_str(NULL, ',');
    if(*gps_data[GEOID]){
        //lcd_iwrite_str(gps_data[GEOID], 0, 4, 0, 0);
        write_str(gps_data[GEOID], ',');
        }
    else
        write_str(NULL, ',');
    if(*gps_data[TIME]){
        s= gps_data[TIME];
        tmp[0]= *s++; 
        tmp[1]= *s++;
        tmp[2]= ':';
        tmp[3]= *s++;
        tmp[4]= *s++;
        tmp[5]= ':';
        tmp[6]= *s++;
        tmp[7]= *s++;
        tmp[8]= 0;
        write_str(tmp, ',');
        }
    else
        write_str(NULL, ',');
    if(*gps_data[DATE]){
        s= gps_data[DATE];
        ////DD.MM.YY
        /* tmp[0]= s[4]; */
        /* tmp[1]= s[5]; */
        /* tmp[2]= '.'; */
        /* tmp[3]= s[2]; */
        /* tmp[4]= s[3]; */
        /* tmp[5]= '.'; */
        /* tmp[6]= s[0]; */
        /* tmp[7]= s[1]; */
        /* tmp[8]= 0; */
        /* write_str(tmp, ' '); */
        ////YYMMDD
        write_str(s, ',');
        }
    else
        write_str(NULL, ',');
    if(*gps_data[SPEED]){
        write_str(gps_data[SPEED], ',');
        }
    else
        write_str(NULL, ',');
    if(*gps_data[DIREC]){
        write_str(gps_data[DIREC], ',');
        }
    else
        write_str(NULL, ',');
    if(*gps_data[NSAT]){
        //lcd_iwrite_str(gps_data[NSAT], 0, 4, 0, 0);
        write_str(gps_data[NSAT], '\n');
        }
    else
        write_str(NULL, '\n');
    }


uint8_t gps_process(char* const s, gps_t gps_data, char* gsv_sats, uint8_t* const siv){//, uint8_t sat_matrix [][MAX_SAT_MAT][2]){
    char* t, * h;
    uint8_t cs= 0;
    char sum[3];

    h= s;
    *siv= 0;
    if((t= strchr(s, '*')))
        while(h < t)
            cs= cs ^ *h++;
    if(strtol(++t, (char **)NULL, 16) != cs){
        lcd_iwrite_str("BAD_CS!", 0, 5, 1, 1);
        lcd_iwrite_str(t, 48, 5, 1, 1);
        lcd_iwrite_str(itoa(cs,sum,16), 66, 5, 1, 1);
        return(BAD_CS);
        }
    if(gps_det_type(s, "GPGGA")){ //(strstr(s, "GPGGA")) //(!strncmp(s, "GPGGA", 5))
        gps_process_gga(s, gps_data); 
//        lcd_iwrite_str(s, 0, 4, 1, 1); //why is GPGGA still there after gps_det_type()???
        zero_gps_data(s); //this truncates s!!!!
        return(NEW_GGA);
        }
    if(gps_det_type(s, "GPGSV")){ //list of visible satalites
        if((*siv= gps_process_gsv(s, gps_data, gsv_sats)))//, sat_matrix)))
            return(NEW_GSV);
        //else
        //    return(GSV_UNF);
        }
    if(gps_det_type(s, "GPRMC")){ //Recommended Minimum Sentence C, date, speed, direction
        gps_process_rmc(s, gps_data);
        zero_gps_data(s); //this truncates s!!!!
        return(NEW_RMC);
        }
    /* else if(!strncmp(s, "GPGSA", 5)) */
    /*     gps_process_gsa(s); */

    return(ERROR);
    }

void init_gps_data(gps_t gps_data){
    uint8_t i;

    for (i= 0; i < GPS_DATA_MAX; i++)
        gps_data[i]= NULL;
    }
/*
void zero_gps_data(gps_t gps_data){
    uint8_t i;

    for (i= 0; i < GPS_DATA_MAX; i++)
        if (*(gps_data[i]) == ',')
            gps_data[i]= 0;
    }
*/
void zero_gps_data(char* s){
    
    while(*s){
        *s= *s == ',' ? 0 : *s; //replace ',' , use if(*gps_data[LAT1]) to check!!!
        s++;
        }
    }

//void gps_display_sats(const char* gsv_sats, uint8_t sat_matrix [][MAX_SAT_MAT][2],  uint8_t sel, uint8_t* n){
void gps_display_sats(const char* gsv_sats, uint8_t sel, uint8_t* n){    const char* s, * nums;
    char i, o[6], elo;
    uint8_t num, snr, x, y, sat= 0;
    int azi; //never ever again change this to unsigned or do a cast!!!
/*
  27.5/64
  .42968750000000000000
  47.5/128
  .37109375000000000000
  27.5/last
  74.10526315789473684210
  .42968750000000000000/.37109375000000000000
  1.15789473684210526315
*/

#define MAX_R 31
#define CX 37
#define CY 31

    s= gsv_sats;//++; //skip ','
    clear_matrix(n);
    lcd_set_pixel(CX, CY, 0, n);
    for(i= 1; i <= 90/30; i++)
        //lcd_circle(MAX_R * i / 3 , CX, CY, 0, n);
        lcd_elips(MAX_R * 1.2 * i / 3, MAX_R  * i / 3, CX, CY, 0, n);
//    for(i= 0; i < 360, i+= 30)
//        lcd_line(32, 32, i, 32, m);

    s= strchr(s, ',') + 1;
    while(*s){
        sat++;
        //s= strchr(s, ',') + 1;
        num= atoi(s) - 1;
        nums= s;
        s= strchr(s, ',') + 1;
        elo= atoi(s);
        s= strchr(s, ',') + 1;
        azi= atoi(s);
        s= strchr(s, ',') + 1;
        snr= atoi(s);
        s= strchr(s, ',') + 1;
        x= ((elo - 90) * MAX_R / 90. * sin(-azi / 180. * M_PI)) * 1.2 + CX;
        y=  (elo - 90) * MAX_R / 90. * cos(-azi / 180. * M_PI);
        y+= CY; //separately because of rounding error
/*
        if(sat_matrix[num][sat_matrix[num][0][0]][0] != x || sat_matrix[num][sat_matrix[num][0][0]][1] != y){
            sat_matrix[num][0][0]++;//0 has the current position!
            if(sat_matrix[num][0][0] >= MAX_SAT_MAT)
                sat_matrix[num][0][0]= 1; //0 has the current position!
            sat_matrix[num][sat_matrix[num][0][0]][0]= x;
            sat_matrix[num][sat_matrix[num][0][0]][1]= y;
            lcd_write_str("Trace!", 90, 4, 0, 1, 0, n);
            }
*/
        lcd_set_pixel(x, y, 0, n);
/*
  for(i= 1; i < MAX_SAT_MAT; i++){ //0 has the current position!
  if(sat_matrix[num][i][0])
  lcd_set_pixel(sat_matrix[num][i][0], sat_matrix[num][i][1], 0, n);
  }
  x= sat_matrix[num][sat_matrix[num][0][0]][0];
  y= sat_matrix[num][sat_matrix[num][0][0]][1];
*/
        /*
          lcd_set_pixel(x - 1, y, 0, n);
          lcd_set_pixel(x - 1, y + 1, 0, n);
          lcd_set_pixel(x, y + 1, 0, n);
        */        
        lcd_set_pixel(x + 1, y, 0, n);
        lcd_set_pixel(x - 1, y, 0, n);
        lcd_set_pixel(x, y + 1, 0, n);
        lcd_set_pixel(x, y - 1, 0, n);
/*
  lcd_set_pixel(x + 1, y + 1, 1, n);
  lcd_set_pixel(x - 1, y - 1, 1, n);
  lcd_set_pixel(x - 1, y + 1, 1, n);
  lcd_set_pixel(x + 1, y - 1, 1, n);
*/
        if (!sel || sat == sel){
            for(i= 5; i >= 0; i--)
                lcd_set_pixel(x + 7 - i, y, !((num + 1) & (1 << i)), n);
            for(i= 0; i < snr / 2; i++)
                lcd_set_pixel(x + i + 3, y + 1, 0, n);
            //lcd_write_str(nums, 90, 6, 0, 1, 0, n);
            }
        }
/*
  for(k= 30; k <=270; k+= 30){
  for(j= 0; j <= 90; j+= 15){
  num= 21;
  elo= j;
  azi= k;
  //azi= 360;
  snr= 20;
  x= ((elo - 90) * MAX_R / 90. * sin(-azi / 180. * M_PI) + CX) * 1.2;
  y=  (elo - 90) * MAX_R / 90. * cos(-azi / 180. * M_PI);
  y+= CY;
  //lcd_iwrite_str(dtostrf(y, 5, 4, o), 80, j / 15, 1, 1);
  lcd_set_pixel(x, y, 0, n);
  lcd_set_pixel(x + 1, y, 0, n);
  lcd_set_pixel(x - 1, y, 0, n);
  lcd_set_pixel(x, y + 1, 0, n);
  lcd_set_pixel(x, y - 1, 0, n);
  for(i= 4; i >= 0; i--)
  lcd_set_pixel(x + 6 - i, y, !(num & (1 << i)), n);
  for(i= 0; i < snr / 2; i++)
  lcd_set_pixel(x + i + 2, y + 1, 0, n);
  }
  }
*/
    //lcd_fw_matrix(n, n);
    lcd_write_matrix(n);
    lcd_iwrite_str(itoa(sel, o, 10), 80, 7, 1, 1);
    }

int main(void){
    uint8_t j= 0, lcd_on= 0, gps_status= 1, gps_status_old= 0, siv, closed;
    uint8_t menu_cnt= 0, menu_sel= 1; //menu stuff
    uint16_t last_lr= 0, send_bytes;
    uint8_t  new[LCD_PIXEL_BYTES];//, old[LCD_PIXEL_BYTES];
    uint8_t * n, read_buf;//, * o;
    uint8_t fnum, ftot, fseek, i;
    char fname[13];
    char gps_s[81], gps_line[81], s[5], c, logit;
    char* gps_d[GPS_DATA_MAX];
    char gsv_sats[GSV_MAX];//for concatenating all gsv messages, 469 Bytes

    //uint8_t sat_matrix[MAX_SATS][MAX_SAT_MAT][2];
    //char*** gsv_sats;

//    gps_t gps_data= gps_d;
    //gsv_sats= gsv_sat;
    //gps_s= gps_str; //bakup of pointer, save is save;)
/*
  for(i= 0; i < MAX_SATS; i++){
  for(j= 1; j < MAX_SAT_MAT; j++){
  sat_matrix[i][j][0]= 0;
  sat_matrix[i][j][1]= 0;
  }
  sat_matrix[i][0][0]= 1;
  //sat_matrix[i][j][1]= 0;
  }
*/
    n= new;
    //o= old;

    //DDRB= 0x47; //6;  //0100 0110 //lcd on, SI, CK, SS
    DDRB= 0x47; //6;  //0100 0111 //lcd on, MOSI, CK, SS
//never again forget to set SS as an output even if it is not the SS of the lcd
//otherwise if driven low SPI-master will become a SPI-slave!!!
    DDRE= 0x1C;  //0001 1100 //A0, RST, CS
    
    DDRD= 0x80;  //1000 0000 //gps on

    PORTD|=  (7 << PD4); //swith on pull-up 

    MMC_IO_Init();

    lcd_init(); // must be executed at the very beginning!
    lcd_clear();
    clear_matrix(n);

    uart0_init(UBRR_VAL);//don't forget to do some recieving if IR is on!
    uart1_init(UBRR_VAL);

    dreh_init(); //uses timer2; inc_push, inc_lr, pressed
    inc_lr= 5 * MENU_MAX; // for 0-break

    init_gps_data(gps_d);
 
    lcd_des();
    //MMC_IO_Init();

    sei();

    //PORTB|= (1 << PB6); //set backlight on
    PORTD|= (1 << PD7);//switch on gps
    lcd_randomize_matrix(n);
    lcd_write_matrix(n);
    lcd_write_str("GPS-Logger von RHG\nV32_130914", 0, 1, 0, 0, 0, n);
//    lcd_fw_matrix(n, old);
    clear_matrix(n);

    if(GetDriveInformation() != F_OK){ // get drive parameters
        lcd_iwrite_str("MMC not responding!", 0, 4, 1, 1);
        }
    else
        lcd_iwrite_str("MMC responding!", 0, 4, 1, 1);

/*
  Fremove("test.txt");//FindName("test.txt");
  if(Fopen("test.txt",F_WRITE) != F_OK){
  lcd_iwrite_str("Can't open file for w!", 0, 0, 1, 1);
  logit= 0;
//        bytecount= 0;
}
else{
char c;
lcd_iwrite_str("Opened file for w!", 0, 1, 1, 1);
c= Fwrite("Hello World on MMC!\n", 20);
//        bytecount= c;
c= Fwrite("Hello World on MMC again!\n", 26);
Fclose();
lcd_iwrite_str(itoa(c, s, 10), 0, 2, 1, 1);
}
*/
    fnum= 0;
    if(Findfirst()!=0){
        do{
            if(ffblk.ff_attr==ATTR_FILE){
                fnum++;
                //ffblk.ff_name[] enthält den DOS 8.3 Dateinamen
                //ffblk.ff_longname[] enthält den langen Dateinamen
                }
/*
  if(ffblk.ff_attr==ATTR_DIRECTORY){
  }
*/
            }while(Findnext()!=0);
        }
    //Fremove("log.txt");
    logit= 1;
    closed= 0;
    itoa(fnum, fname, 10);
    if(Fopen(fname,F_WRITE) != F_OK){
        lcd_iwrite_str("Can't open file for w!", 0, 5, 1, 1);
        logit= 0;
        closed= 1;
//        bytecount= 0;
        }
    else
        lcd_iwrite_str(fname, lcd_iwrite_str("Opened file ", 0, 5, 1, 1), 5, 1, 1);  

    for(;;){
        gps_status= DONE;
        while(uart1_rec){
            c= uart1_getchar();//this decreases uart1_rec!
            if (c == '\r'){ //end \r\n
                gps_s[j]= 0;
                strcpy(gps_line, gps_s);
                //gps_com= 1;
                j= 0;    
                //uart0_write_str(gps_s);
                if(*gps_s == '$'){
                    gps_status= gps_process(gps_s + 1, gps_d, gsv_sats, &siv);//, sat_matrix);
                    }
                break;
                }
            else if (c > 0x20){ // (c != '\n'){
                gps_s[j]=c;
                j++;
                }

            if(j > 80){
                lcd_iwrite_str("ERROR! j > 80!", 0, 7, 1, 0);
                j= 0;
                }
            }
      
        if (inc_push){
            inc_push= 0;
            menu_sel= !menu_sel; //change menu or other input switch
//            if (menu_sel)
//                inc_lr= menu_cnt + 5 * MENU_MAX; //start at last value
            lcd_clear();
            gps_status= NEW_MENU;
            /*
              gps_process("GPGGA,230906.00,5125.54685,N,00709.16364,E,1,04,8.01,127.3,M,47.5,M,,*5F", gps_d);//"GPGGA,201835.00,,,,,0,00,99.99,,,,,,*6B", gps_d);
              zero_gps_data(gps_s);
              display_gga(gps_d);
              col= lcd_iwrite_str("UTC: ", 3, 6, 0, 0);
              lcd_iwrite_str(itoa(col, s, 10), col, 7, 0, 1);
            */
            ///////Here the on/off Menus!
            switch (menu_cnt){
            case LCD:
                menu_sel= 1;
                if (lcd_on){
                    PORTB&= ~(1 << PB6);//set backlight off
                    lcd_iwrite_str("Backlight off", 0, 0, 1, 1);
                    }
                else {
                    PORTB|= (1 << PB6);//set backlight on                
                    lcd_iwrite_str("Backlight on", 0, 0, 1, 1);
                    }
                lcd_on= !lcd_on;
                break;
            case CLOSEF:
                menu_sel= 1;
                if (closed){
                    if(Fopen(fname,F_WRITE) != F_OK){
                        lcd_iwrite_str("Can't open file for w!", 0, 4, 1, 1);
                        logit= 0;
//        bytecount= 0;
                        }
                    else{
                        logit= 1;
                        lcd_iwrite_str("Started loggin!", 0, 0, 1, 1);
                        }
                    }
                else {
                    Fclose();
                    logit= 0;
                    lcd_iwrite_str("Stopped logging!", 0, 0, 1, 1);
                    }
                closed= !closed;

                break;
                }
            if (menu_sel){
                inc_lr= menu_cnt + 5 * MENU_MAX; //start at last value
                lcd_iwrite_str(itoa(menu_cnt, s, 10), 0, 6, 0, 1);
                lcd_iwrite_strp(MenuStrs[menu_cnt], 0, 7, 0, 1);
                }
            }
/****This is checked each loop and there for to often
*****It's now in turn and push checks
if (menu_sel){
lcd_iwrite_str(itoa(menu_cnt, s, 10), 0, 6, 0, 1);
lcd_iwrite_strp(MenuStrs[menu_cnt], 0, 7, 0, 1);
}
*/
        if (inc_lr != last_lr){//better use a ch_lr if number stays the same?
            if (menu_sel){
                //cli();
                menu_cnt= (uint8_t) inc_lr % MENU_MAX;
                lcd_iwrite_str(itoa(menu_cnt, s, 10), 0, 6, 0, 1);
                lcd_iwrite_strp(MenuStrs[menu_cnt], 0, 7, 0, 1);
                //sei();
                }
            last_lr= inc_lr;
            }
        if(gps_status == NEW_GGA && logit){
            log_gga(gps_d);
            Fflush(); ///VERY IMPORTANT SHOULD BATTERY RUN OUT OR BE DISCONNECTED!!!!!
            }
        if(!menu_sel)
            if(gps_status != DONE) {
                switch(menu_cnt){
                case INFO:
                    switch(gps_status){
                    case NEW_GGA:
                        //lcd_clear();//yields flickering
                        if((gps_status_old != gps_status)){
                            gps_status_old= gps_status;
                            lcd_clear();//yields flickering
                            }
                        //display_gga(gps_d);
                        display_gga_rmc(gps_d);
                        break;
                    case NEW_MENU:
                        //case WAITING:
                        if((gps_status_old != gps_status)){
                            gps_status_old= gps_status;
                            lcd_clear();//yields flickering
                            }
                        lcd_iwrite_str("Info-menue\nwaiting...", 0, 0, 1, 0);
                        break; 
//                case ERROR: //uncomment when all other done
//                    lcd_iwrite_str("Info-menue\nError processing gps data!...", 0, 0, 1, 1);
//                    break;
                        }
                    //    }
                    break;
                case GGA:
                    //if(gps_status_old!= gps_status){
                    //    gps_status_old= gps_status;
                    switch(gps_status){
                    //case NEW_GGA:
                    case NEW_RMC:
                        //lcd_clear();//yields flickering
                        if((gps_status_old != gps_status)){
                            gps_status_old= gps_status;
                            lcd_clear();//yields flickering
                            }
                        lcd_iwrite_str(gps_line, 0, 0, 0, 1);
                        break;
                    case NEW_MENU:
                        if((gps_status_old != gps_status)){
                            gps_status_old= gps_status;
                            lcd_clear();//yields flickering
                            }
                        lcd_iwrite_str("GGA-menue\nwaiting...", 0, 6, 1, 0);
                        break;
                        }
                    break;
                case SATV:
                    switch(gps_status){
                    case NEW_GSV:
                        if((gps_status_old != gps_status)){
                            gps_status_old= gps_status;
                            lcd_clear();//onyl once!
                            }
                        //gps_display_sats(gsv_sats, sat_matrix, siv ? (inc_lr + 5 * (siv + 1)) % (siv + 1) : 0,  n);
                        gps_display_sats(gsv_sats, siv ? (inc_lr + 5 * (siv + 1)) % (siv + 1) : 0,  n);
                        lcd_iputmchar(itoa(siv, s, 10), 2, 80, 5, 1);
                        break;
                    case NEW_MENU:
                        lcd_clear();//yields flickering
                        lcd_iwrite_str("Satelite view\nwaiting...", 0, 6, 1, 0);
                        inc_lr= 0;
                        break;
                        }
                    break;
                case GSV:
                    switch(gps_status){
                    case NEW_GSV:
                        if((gps_status_old != gps_status)){
                            gps_status_old= gps_status;
                            lcd_clear();//onyl once!
                            }
                        gps_display_gsv(gps_d[SIV], gsv_sats, (uint8_t)inc_lr);
                        break;
                    case NEW_MENU:
                        lcd_clear();//yields flickering
                        lcd_iwrite_str("GSV-menue\nwaiting...", 0, 6, 1, 0);
                        inc_lr= 0;
                        break;
                        }
                    break;
/*
  case LCD:
  if(gps_status == NEW_MENU){
  lcd_iwrite_str("Backlight on/off", 0, 6, 1, 1);
  lcd_on= !lcd_on;
  if (lcd_on)
  PORTB&= ~(1 << PB6);//set backlight off
  else
  PORTB|= (1 << PB6);//set backlight on
  }
  break;
*/
                case UART:
                    //if(gps_status == BAD_CS)
                    //    lcd_iwrite_str("BAD_CS!", 0, 6, 1, 1);
                    if(gps_status >= BAD_CS){// NEW_GGA){
                        lcd_iwrite_str("Writing to UART0", 0, 6, 1, 1);
                        if(gps_status == BAD_CS)
                            uart0_write_str("E:"); 
                        uart0_write_str(gps_line);
                        }
                    break;
/*
  case CLOSEF:
  if(gps_status == NEW_MENU){

  Fclose();
  logit= 0;
  lcd_iwrite_str("Closed file, stopped logging", 0, 6, 1, 1);
  }
  break;
*/
                case SENDF:
                    if(gps_status == NEW_MENU){
                        ftot= 0;
                        if(Findfirst()){
                            do{
                                if(ffblk.ff_attr==ATTR_FILE)
                                    ftot++;
                                }while(Findnext());
                            }
                        else{
                             lcd_iwrite_str("No file found!", 0, 5, 1, 1);
                             break;
                            }
                        if(Findfirst()){
                            fseek= inc_lr % ftot;
                            for(i= 0; i < fseek; i++)
                                Findnext();
                            lcd_iwrite_str(ffblk.ff_name, lcd_iwrite_str("Current file: ", 0, 2, 1, 1), 2, 1, 1); 
                            }
                        Fclose();
                        logit= 0;
                        closed= 1;

                        lcd_iwrite_str("Stopped logging", 0, 0, 1, 1);
                        if(Fopen(ffblk.ff_name,F_READ) != F_OK)
                            lcd_iwrite_str("Can't open file for r!", 0, 5, 1, 1);
                        else {
                            lcd_iwrite_str("Writing file to UART0", 0, 1, 1, 1);
                            send_bytes= 0;
                            while(Fread(&read_buf, 1))
                                {
                                while (!(UCSR0A & (1<<UDRE0))) {}
                                UDR0=read_buf;
                                send_bytes++;
                                lcd_iset_byte(1,0,send_bytes / 10,0);
                                }
                            lcd_iwrite_str(" Bytes written!", lcd_iwrite_str(itoa(send_bytes, s, 10), 0, 2, 1, 1), 2, 1, 1);                          
                            }
                        Fclose();
                        }
                    else
                        lcd_iwrite_str("Writing file done!", 0, 6, 1, 1);
                    break;
                default:
                    lcd_iwrite_str("Menu not known!", 0, 7, 1, 1);
                    }
                } 
/*
  if(pressed)
  PORTB|= (1 << PB6);//set backlight on
  else
  PORTB&= ~(1 << PB6);//set backlight off
*/
        }
    
    return(0);
    }

/*
ISR(USART0_RX_vect){
    uart0_byte= UDR0;
    uart0_rec= 1;
    while (!(UCSR1A & (1<<UDRE1))) {} //send to gps
    UDR1=uart0_byte;
    }
*/
ISR(USART1_RX_vect){
    cli();
    (u1_ringbuf_index > U_RINGBUF_SIZE - 2) ? u1_ringbuf_index= 0 : u1_ringbuf_index++;
    u1_ringbuf[u1_ringbuf_index]= UDR1;
//    uart1_byte= UDR1;
    uart1_rec++;
    sei();
    }
