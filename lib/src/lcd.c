#include <avr/io.h>
#include "avr_delay.h"
#include <stdio.h>

#include "lcd.h"

//PB1 = RS, PB2 = RW, PB3 = EN, PORTD = data
#define CONTROL_PORT PORTC
#define CONTROL_PORT_CONFIG DDRC
#define DATA_PORT PORTB
#define DATA_PORT_CONFIG DDRB
#define RS PC3
#define RW PC4
#define EN PC5
#define BUSY PB7
#define BUSY_INPUT PINB

#define ENABLE_DURATION 1000 //in micro-seconds

//lcd functions
void lcd_ready()
{
    DATA_PORT &= ~(1<<BUSY); //set PD7 to 0, if it was set to 1 earlier
    // if PD7 was set to 1 earlier and it is not set to 0 before configuring it as an input,
    // its value will always be read as 1
    DATA_PORT &= ~(1<<BUSY); //set PD7 as input
    CONTROL_PORT &= ~(1<<RS); //RS = 0
    CONTROL_PORT |= (1<<RW); //RW = 1

    //perform busy wait
    while((BUSY_INPUT & (1<<BUSY)) == (1<<BUSY))
    {
        CONTROL_PORT &= ~(1<<EN); //EN = 0
        delayus(ENABLE_DURATION);
        CONTROL_PORT |= (1<<EN); //EN = 1
    }

    DATA_PORT_CONFIG |= (1<<BUSY); //make PD7 output again
    CONTROL_PORT &= ~(1<<EN); //EN = 0

    return;
}

void lcd_cmd(unsigned char cmd)
{
    lcd_ready();
    DATA_PORT = cmd; //send cmd to data port
    CONTROL_PORT &= ~((1<<RS) | (1<<RW)); //RS = 0, RW = 0
    CONTROL_PORT |= (1<<EN); //EN = 1
    delayus(ENABLE_DURATION);
    CONTROL_PORT &= ~(1<<EN); //EN = 0

    return;
}

void lcd_data(unsigned char data)
{
    lcd_ready();
    DATA_PORT = data; //send data to data port
    CONTROL_PORT &= ~(1<<RW); //RW = 0
    CONTROL_PORT |= (1<<RS); //RS = 1
    CONTROL_PORT |= (1<<EN); //EN = 1
    delayus(ENABLE_DURATION);
    CONTROL_PORT &= ~(1<<EN); //EN =0

    return;
}

void lcd_init(void)
{
    CONTROL_PORT_CONFIG |= ((1<<RS) | (1<<RW) | (1<<EN));
    PORTB &= ~((1<<RS) | (1<<RW) | (1<<EN));
    DATA_PORT_CONFIG |= 0xFF;
    DATA_PORT = 0x00;

    lcd_cmd(0x38); //2 lines 5x7 matrix for each character
    lcd_cmd(0x01); //clear display
    lcd_cmd(0x02); //take cursor to initial location
    lcd_cmd(0x0E); //display on, cursor blinking

    return;
}

void lcd_reset(void)
{
    lcd_cmd(0x01); //clear display
    lcd_cmd(0x02); //take cursor to initial location

    return;
}

//a simple routine to demostrate the lcd (NOTE :- consists of significant delays !!!!!)
void lcd_demo(void)
{
    lcd_data('H');
    lcd_data('e');
    lcd_data('l');
    lcd_data('l');
    lcd_data('o');
    delayms(1000);
    lcd_cmd(0x01); //clear display
    lcd_cmd(0x02); //return cursor to home
    lcd_cmd(0xC5); //go to beginning of 2nd line
    lcd_data('w');
    lcd_data('o');
    lcd_data('r');
    lcd_data('l');
    lcd_data('d');
    delayms(1000);
    lcd_cmd(0x01);
    lcd_cmd(0x02);

    return;
}

void lcd_print_string(const char* ptr, uint8_t num_chars, char loc)
{
    uint8_t count = 0;

    lcd_cmd(loc); //set cursor to specified initial location

    for (count =0; count<num_chars; count++)
    {
        if (*(ptr+count) != '\0')
        {
            lcd_data(*(ptr+count));

            loc++;
            if(loc >= 0x90 && loc < 0xC0)
            {
                lcd_cmd(0xC0);
                loc = 0xC0;
            }
        }

        else
        {
            break;
        }
    }

    return;
}

//this function is used to print constant strings stored in flash memory
void lcd_print_string_progmem(const prog_uchar* ptr, uint8_t num_chars, char loc)
{
    uint8_t count = 0;
    prog_uchar data;

    lcd_cmd(loc); //set cursor to specified initial location

    for (count =0; count<num_chars; count++)
    {
        data = pgm_read_byte_near(ptr+count);

        if(data != '\0')
        {
            lcd_data(data);
        }

        else
        {
            break;
        }

    }

    return;
}

void lcd_print_num(uint16_t val, uint8_t num_digits, char loc)
{
    //Max value of 'val' is 65536 and max value of 'num_digits' is 5
    /*uint8_t count = 0;
    uint16_t mask = 1; //used to print the number digit by digit
    uint8_t flag = 0; //this variable is used to track the first non zero digit in the input number
    char data = '0'; //ascii value of the digit

    for (count =0; count<num_digits-1; count++)
    {
        mask = mask*10;
    }

    lcd_cmd(loc); //set cursor to specified initial location

    for (count =0; count<num_digits; count++)
    {
        data = 0x30 + (val/mask);
        if (data > 0x30)
        {
            flag = 1;
        }

        if (flag == 1 || count == num_digits-1) //this check is done to prevent the printing of leading zeros
        {
           lcd_data(data);
        }

        else
        {
            lcd_data(' '); //print spaces insted of lesding zeros
        }

        val = val%mask;
        mask = mask/10;
    }*/

    char array[num_digits + 1];
    sprintf(array, "%d", val);
    lcd_print_string(array, num_digits, loc);

    return;
}

void lcd_clear_segment(uint8_t num_segments, char loc)
{
    uint8_t count = 0;

    lcd_cmd(loc); //take cursor to the correct location

    for(count = 0; count < num_segments; count++)
    {
        lcd_data(' ');
    }

    return;
}
