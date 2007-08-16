#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <inttypes.h>
#include <avr/eeprom.h>
//#include <math.h>
#include <avr/interrupt.h> 
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "font6x8s.h" //defines prog_char Font [256] [6] 
#include "drehenc.h"  //defines inc_lr, inc_push, pressed

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
#define lcd_des() { LCD_CS_PORT|= (1 << LCD_CS_PIN); } //select chip



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

volatile uint8_t uart0_byte, uart0_rec, uart1_byte, uart1_rec;

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

void lcd_write(uint8_t byte, uint8_t type){ //manual SPI, slower!
    char i;
    //setpin(LCD_CK_PORT, LCD_CK_PIN, 0);
    //setpin(LCD_CS_PORT, LCD_CS_PIN, 0); //select chip, could be moved outside for performance
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
    //setpin(LCD_CS_PORT, LCD_CS_PIN, 1); //deselect chip, could be moved outside for performance
    }

/*
void lcd_write(uint8_t byte, uint8_t type){ //for SPI

    if (type) //set A0
        LCD_A0_PORT|= (1 << LCD_A0_PIN);
    else
        LCD_A0_PORT&= ~(1 << LCD_A0_PIN);
    SPCR|= (1 << SPE) | (1 << MSTR); //DORD, CPOL, CPHA, SPR1, SPR0; f/4
    SPSR|= (1 << SPI2X); //SCK/2
    SPDR= byte;
    }
*/
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
    LCD_RS_PORT|= (1 << LCD_RS_PIN);
    _delay_ms(1);
    lcd_sel();
    for (i= 0; i < INIT_COM; i++){ //sizeof(init)/sizeof(init[0]) //we'll do this statically for better performance
        lcd_command(init[i]);
        }
    lcd_des();
    }

void lcd_clear(uint8_t* m){
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
    for (i= 0; i < LCD_PIXEL_BYTES; i++) //should this be moved out?
        m[i]= 0;
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
//a char within a page, needs a write afterwards! 

    uint8_t width, i;

    if (c < 32 )
        return(0);
    width= 127 - col < FONT_WIDTH ? 127 - col : FONT_WIDTH; //check if to draw over the border, for running text;)
    for (i= 0; i < FONT_WIDTH; i++)
        lcd_set_byte(pgm_read_byte(&Font[c][i]), col + i, page, inv, trans, m); //_far needed???
    return(1);
    }

char lcd_iputchar(uint8_t c, uint8_t col, uint8_t page, uint8_t inv){
//puts char instantly on lcd

    uint8_t width, i;

    if (c < 32 )
        return(0);
    width= 127 - col < FONT_WIDTH ? 127 - col : FONT_WIDTH; //check if to draw over the border, for running text;)
    for (i= 0; i < FONT_WIDTH; i++)
        lcd_iset_byte(pgm_read_byte(&Font[c][i]), col + i, page, inv); //_far needed???
    return(1);
    }

uint8_t lcd_write_str(char* c, uint8_t col, uint8_t page, uint8_t inv, uint8_t trans, uint8_t clear, uint8_t* m){//returns displayed chars
//display a string, needs no write, use' \n'!

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
        lcd_putchar(*oc, col, page, inv, trans, m);
        col+= FONT_WIDTH;
        oc++;
        }
    if(clear)
        while (col < 127){
            lcd_set_byte(0, col, page, inv, 0, m);
            col++;
            }
    lcd_write_matrix(m);
    return((oc - c));
    }

void lcd_drawchar(char c, uint8_t x, uint8_t y, uint8_t trans, uint8_t* m){//a char at x,y

    }

void uart0_init(uint16_t baud){//51 for 9600bps
//    UBRR0H = (unsigned char)(baud >> 8);
//    UBRR0L = (unsigned char)baud;
    UBRR0H = baud >> 8;
    UBRR0L = baud & 0xFF;
    UCSR0C|= (3<<UCSZ00);//async, 8N1, see also UCSZn2! (1<<UMSEL0) sync
    UCSR0B|=  (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0); //enable recieve interrupt
    }

void uart1_init(uint16_t baud){//51 for 9600bps
//    UBRR0H = (unsigned char)(baud >> 8);
//    UBRR0L = (unsigned char)baud;
    UBRR1H = baud >> 8;
    UBRR1L = baud & 0xFF;
    UCSR1C|= (3<<UCSZ10);//async, 8N1, see also UCSZn2! (1<<UMSEL0) sync
    UCSR1B|= (1<<RXCIE1)|(1<<RXEN1)|(1<<TXEN1); //enable recieve interrupt
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

int main(void){
    uint8_t j= 0;
    uint16_t i;
    uint8_t  new[LCD_PIXEL_BYTES], old[LCD_PIXEL_BYTES];
    uint8_t *n, *o;
    char gps_s[80], s[5];

    n= new;
    o= old;

    DDRB= 0x46;  //0100 0011 //lcd on, SI, CK
    DDRE= 0x1C;  //0001 1100 //A0, RST, CS
    
    DDRD= 0x80;  //1000 0000 //gps on

    DDRD &= ~(7 << PD4);//set to input
    PORTD|= (7 << PD4); //swith on pull-up 

    lcd_init(); // must be executed at the very beginning!
    lcd_clear(n);

    uart0_init(UBRR_VAL);
    uart1_init(UBRR_VAL);

    dreh_init(); //uses timer2; inc_push, inc_lr, pressed

    sei();

//    while (!(UCSR0A & (1<<UDRE0))) {}
//    UDR0='H';
    //setpin('B', PB6, 1);//PORTB|= (1 << PB6); //set backlight on
    PORTD|= (1 << PD7);//switch on gps
    _delay_ms(128);
    lcd_randomize_matrix(n);
    lcd_write_str("Hello World!\nGo, go!\nJuppey, this actually works great! Incredible this is, wow!", 0, 1, 0, 0, 0, n);
    lcd_write_matrix(n);
    _delay_ms(128);
    for(;;){

        //interrupt gps
        /*
        if(uart1_rec){
            while (!(UCSR0A & (1<<UDRE0))) {}
            cli();
            UDR0=uart1_byte;
            while(!(UCSR0A & (1<<TXC0))) {}
            sei();
            if (uart1_byte == '\r'){ //end
                gps_s[j]= 0;
                //gps_com= 1;
                j= 0;    
                for (i= 0; i < LCD_PIXEL_BYTES; i++) 
                    n[i]= 0;
                cli();
                lcd_write_str(gps_s, 0, 0, 0, 0, n);
                sei();
                }
            else{
                gps_s[j]=uart1_byte;
                j++;
                }

            if(j > 80){
                lcd_write_str("ERROR! j > 80!", 0, 7, 1, 0, n);
                j= 0;
                }

            uart1_rec= 0;
            }
        */
        if (inc_push){
            lcd_write_str("Encoder pressed! ", 0, 0, 0, 0, 1, n);
            lcd_write_str(itoa(inc_lr, s, 10), 0, 6, 0, 0, 1, n);
            inc_push= 0;
            }
        if(pressed)
            PORTB|= (1 << PB6);//set backlight on
        else
            PORTB&= ~(1 << PB6);//set backlight off
//        lcd_iputchar('R', 0, 6, 0);
//        lcd_iputchar(uart0_Rx(), 0, 6, 0);
/*
  if(uart0_rec){
  lcd_iputchar(uart0_byte, 10, 6, 0);
  uart0_rec= 0;
  }
*/
        //lcd_write_str(s, 0, 0, 0, 0, n);


/*
//polling gps
s[0]=uart1_Rx();
s[1]= 0;
lcd_write_str(s, 0, 0, 0, 0, n);    
*/
/*
  while (!(UCSR0A & (1<<UDRE0))) {}
  UDR0=gps_s[0];
*/
        }
    
    return(0);
    }


ISR(USART0_RX_vect){
    uart0_byte= UDR0;
    uart0_rec= 1;
    }

ISR(USART1_RX_vect){
    uart1_byte= UDR1;
    uart1_rec= 1;
    }
