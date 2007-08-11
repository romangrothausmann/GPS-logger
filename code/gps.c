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


#define LCD_SI_PIN  PB2
#define LCD_SI_PORT 'B' //PORTB
#define LCD_CK_PIN  PB1
#define LCD_CK_PORT 'B' //PORTB
#define LCD_A0_PIN  PE4
#define LCD_A0_PORT 'E' //PORTE
#define LCD_RS_PIN  PE3
#define LCD_RS_PORT 'E' //PORTE
#define LCD_CS_PIN  PE2
#define LCD_CS_PORT 'E' //PORTE 

#define LCD_PIXEL_BYTES 1024


typedef uint8_t lcd_t[LCD_PIXEL_BYTES];


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


void lcd_write(uint8_t byte, uint8_t type){
    char i;
    setpin(LCD_CK_PORT, LCD_CK_PIN, 0);
    setpin(LCD_A0_PORT, LCD_A0_PIN, type); //set A0
    setpin(LCD_CS_PORT, LCD_CS_PIN, 0); //select chip, could be moved outside for performance
    for (i= 7; i >= 0; i--){
//    _delay_ms(1);
        setpin(LCD_SI_PORT, LCD_SI_PIN, (byte & (1 << i))); //prepare SI
//    _delay_ms(1);
        setpin(LCD_CK_PORT, LCD_CK_PIN, 1); //validate SI with rising edge
//    _delay_ms(1);
        setpin(LCD_CK_PORT, LCD_CK_PIN, 0);
        }

    setpin(LCD_CS_PORT, LCD_CS_PIN, 1); //deselect chip, could be moved outside for performance
    }

void lcd_command(uint8_t byte){
    lcd_write(byte, 0);
    }

void lcd_data(uint8_t byte){
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

    setpin(LCD_RS_PORT, LCD_RS_PIN, 0); //do the resetting first!!!
    _delay_ms(1);
    setpin(LCD_RS_PORT, LCD_RS_PIN, 1);
    _delay_ms(1);
	
    for (i= 0; i < INIT_COM; i++){ //sizeof(init)/sizeof(init[0]) //we'll do this statically for better performance
        lcd_command(init[i]);
        }
    }

void lcd_clear(lcd_t m){
    uint16_t i;
    uint8_t j;

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
    for (i= 0; i < LCD_PIXEL_BYTES; i++)
        m[i]= 0;
    }

/*
char lcd_set_pixel(uint8_t x, uint8_t y, uint8_t on, lcd_t matrix){
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

char lcd_set_pixel(uint8_t x, uint8_t y, uint8_t on, lcd_t m){ 
    //only sets pixel in matrix use lcd_write_matrix() when all set

    uint16_t i;

    if (x > 127 || y > 63)
        return(1);
    i= y / 8 * 128 + x; //no cast necessary because y and 8 are integers
    if (on)
        m[i]|= 1 << (y % 8);
    else
        m[i]&= ~(1 << (y % 8));
    return(0);
    }

void lcd_set_abs_pixel(uint16_t i, uint8_t on, lcd_t m){ 
    //only sets absolut pixel in matrix use lcd_write_matrix() when all set

    uint8_t x,y;

    x= i % 128;
    y= i / 128;
    lcd_set_pixel(x, y, on, m);
    }

void lcd_write_matrix(lcd_t m){
    uint16_t i;
    uint8_t page, col;

    for (i= 0; i < LCD_PIXEL_BYTES; i++){
        page= i / 128;
        col=  i % 128;
        lcd_command(0xB0 + page); //or |?//set page address
        lcd_command(0x10 + ((0xF0 & col) >> 4)); //set first column nibble
        lcd_command(0x00 + (0x0F & col)); //set second column nibble
        lcd_data(m[i]);
        }
    }

uint8_t* lcd_fw_matrix(lcd_t n, lcd_t o){ //lcd_t is a pointer of uint8_t!!! so this is fine;)
    uint16_t i;
    uint8_t page, col;

    //find first col and page with difference and so on...
    for (i= 0; i < LCD_PIXEL_BYTES; i++){
        if(n[i] != o[i]){
            page= i / 128;
            col=  i % 128;
            lcd_command(0xB0 + page); //or |?//set page address
            lcd_command(0x10 + ((0xF0 & col) >> 4)); //set first column nibble
            lcd_command(0x00 + (0x0F & col)); //set second column nibble
            lcd_data(n[i]);
            o[i]= n[i];
            }
        }    
    return(o);
    }

void lcd_randomize_matrix(lcd_t m){
    uint16_t i;
    for (i= 0; i < LCD_PIXEL_BYTES; i++)
        m[i]= rand()/RAND_MAX * 256;
    }

int main(void){
    uint8_t x, y, j;
    uint16_t i;
    lcd_t m, old;
    uint8_t* o;

    o= old;
    DDRB= 0x46;  //0100 0110
    DDRE= 0x1C;  //0001 1100

    lcd_init(); // must be executed at the very beginning!
    lcd_clear(m);





    setpin('B', PB6, 1);//PORTB|= (1 << PB6); //set what to on?
    //  for(;;)
    //  lcd_command(0xA5);
    //lcd_data(0xFF);
    //lcd_data(0xFF);
    //lcd_command(0x49);

    lcd_set_pixel(0, 10, 1, m);
    x= 0;
    y= 0;
    j= 10;
    lcd_randomize_matrix(m);
    lcd_write_matrix(m);
    _delay_ms(127);
    for(;;){
/*
  if (x > 127){
  x= 0;
  y++;
  }
  if (y > 63)
  y= 0;
  lcd_set_pixel(x, y, 1, m);
*/
        
        for(i= 0; i < LCD_PIXEL_BYTES * 8; i++) {
            lcd_set_abs_pixel(i,1,m);
            lcd_set_abs_pixel(i-j,0,m);
            o= lcd_fw_matrix(m,o);
            //lcd_write_matrix(m);
            //_delay_ms(12);
            }
        
        lcd_randomize_matrix(m);
        lcd_write_matrix(m);
        _delay_ms(127);
        }
    
    return(0);
    }
