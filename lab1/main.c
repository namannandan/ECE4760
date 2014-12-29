/* Hardware config
LCD :-
Control port = PORTC
RS-> PC3, RW-> PC4, EN-> PC5
Data port = PORTB

switches and led :-
sw1 is connected to PC2
SW2 is connected to INT0
led is connected to PC0
*/

//define clock frequency
#ifndef F_CPU
#define F_CPU 8000000UL
#endif

//include necessary libraries
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdbool.h>

#include "lcd.h"
#include "avr_delay.h"

//process schedule time durations
#define t1 30 //SW1 state machine update duration
#define t2 50 //application state machine update duration
#define t3 100 //lcd update duration

//SW1 states
#define NoPush 1
#define MaybePush 2
#define Pushed 3
#define MaybeNoPush 4

//main application states
#define READY 1
#define INSTRUCTIONS 2
#define WAIT 3
#define CHEAT 4
#define RUNNING 5
#define RESULTS 6

//Flags for communication between processes (there can be a total of 16 flags)
#define DISPLAY_READY 0
#define SW1_EVENT 1
#define DISPLAY_INSTRUCTIONS 2
#define WAIT_DONE 3
#define SW2_EVENT 4
#define TIME_COUNT 5
#define TOO_SLOW 6
#define DISPLAY_CHEAT 7
#define DISPLAY_TOO_SLOW 8
#define DISPLAY_RESULTS 9
#define DISPLAY_WAITING 10

//_____Global variables_____
const prog_uchar message[] PROGMEM = {"Reaction time   tester"};
const prog_uchar ready[] PROGMEM = {"READY !!!"};
const prog_uchar push_SW1[] PROGMEM = {"push SW1"};
const prog_uchar instructions[] PROGMEM = {"push SW2 when   led comes on"};
const prog_uchar reaction_time[] PROGMEM = {"R/n time :"};
const prog_uchar high_score[] PROGMEM = {"High score :"};
const prog_uchar too_slow[] PROGMEM = {"Too slow !      Try again !"};
const prog_uchar cheat[] PROGMEM = {"CHEAT !!!"};
const prog_uchar waiting[] PROGMEM = {"....."};

//timer variables used to schedule tasks
volatile uint16_t time1 = 0;
volatile uint16_t time2 = 0;
volatile uint16_t time3 = 0;

//variables used to store current state of state machines
//used for trackig SW1 state
uint8_t PushState = NoPush;
//used for tracking main application state
uint8_t app_state = READY;

//flags for inter-process communication
volatile uint16_t flags = 0;

//timer1 capture value
volatile uint16_t t1capture = 0;

//variable used to track wait duration in WAIT state of application
volatile uint16_t wait_duration = 0;

//variable to track number of times timer1 overflows
volatile uint16_t time_count = 0;

//variable used to store high_score
//make it an EEMEM variable so that it gets sotred in EEPROM
uint16_t EEMEM high_score_eeprom = 400;

//create variable is sram to store high_score
uint16_t high_score_sram = 0;

//_____Function prototypes_____
void init(void);

//tasks
void task1(void); //SW1 state machint
void task2(void); //app_state machine
void task3(void); //screen update task

//functions to modify flags
void set_flag(uint8_t val);
void clear_flag(uint8_t val);
bool is_flag_set(uint8_t val);


//_____ISR_____
//timer2 compare vector
ISR (TIMER2_COMP_vect)
{
    if (time1 > 0)
    {
        time1 = time1 - 1;
    }

    if (time2 > 0)
    {
        time2 = time2 - 1;
    }

    if (time3 > 0)
    {
        time3 = time3 - 1;
    }

    if(app_state == WAIT && !is_flag_set(WAIT_DONE))
    {
        if(wait_duration >= 2000)
        {
            set_flag(WAIT_DONE);
            wait_duration = 0;
        }

        else
        {
            wait_duration++;
        }
    }

    if (is_flag_set(TIME_COUNT) && !is_flag_set(SW2_EVENT) && !is_flag_set(TOO_SLOW))
    {
        //if response time exceeds 400ms, set TOO_SLOW flag
        if(time_count >= 400)
        {
            set_flag(TOO_SLOW);
        }

        else
        {
            time_count++;
        }
    }
}

ISR (INT0_vect)
{
    if(!is_flag_set(SW2_EVENT) && (app_state == WAIT || app_state == RUNNING))
    {
        set_flag(SW2_EVENT);
    }
}

//_____MAIN_____
int main(void)
{
    //initialization
    init();

    //reset time values
    time1 = 0;
    time2 = 0;
    time3 = 0;

    //infinite loop
    while(1)
    {
        if (time1 == 0)
        {
            task1();
        }

        if (time2 == 0)
        {
            task2();
        }

        if (time3 == 0)
        {
            task3();
        }
    }
}


//______Functions_______
//initialization function
void init(void)
{
    //initialize lcd
    lcd_init();

    //timer2 config for 1ms time base
    //enable interrupt on compare
    TIMSK |= (1<<OCIE2);
    //set the appropriate compare value
    OCR2 = 124;
    //set prescalar to 64 and timer 2 to CTC mode
    TCCR2 |= ((1<<CS22) | (1<<WGM21));
    //set timer counter value to 0
    TCNT2 = 0x00;

    //led
    DDRC |= (1<<PC0);

    //external hardware interrupt
    //falling edge on INT0 generates an interrupt request
    MCUCR |= (1<<ISC01);
    //enable external interrupt request on INT0
    GICR |= (1<<INT0);

    //write a message to screen
    lcd_print_string_progmem(message,16,0x80);
    lcd_print_string_progmem(&(message[16]),16,0xC0);
    delayms(2000);

    //enable global interrupts
    sei();

    return;
}

//task functions
void task1(void)
{
    time1 = t1;

    switch(PushState)
    {
        case NoPush:
        {
            if(~PINC & 0x04)
            {
                PushState = MaybePush;
            }
            else
            {
                PushState = NoPush;
            }
            break;
        }

        case MaybePush:
        {
            if(~PINC & 0x04)
            {
                PushState = Pushed;
            }
            else
            {
                PushState = NoPush;
            }
            break;
        }

        case Pushed:
        {
            if(~PINC & 0x04)
            {
                PushState = Pushed;
            }
            else
            {
                PushState = MaybeNoPush;
            }
            break;

        }

        case MaybeNoPush:
        {
            if(~PINC & 0x04)
            {
                PushState = Pushed;
            }
            else
            {
                PushState = NoPush;

                //SW1 has been pushed and released
                set_flag(SW1_EVENT);
            }
            break;

        }

    }

    return;
}

void task2(void)
{
    time2 = t2;

    switch(app_state)
    {
        case READY:
        {
            //display ready message on lcd
            if (!is_flag_set(DISPLAY_READY))
            {
                set_flag(DISPLAY_READY);
            }

            if(is_flag_set(SW1_EVENT))
            {
                //when SW1 event is detected, set app_state to INSTRUCTIONS
                app_state = INSTRUCTIONS;
                //clear SW1 event flag
                clear_flag(SW1_EVENT);
                //clear DISPLAY_READY flag
                clear_flag(DISPLAY_READY);
            }

            break;
        }

        case INSTRUCTIONS:
        {
            if (!is_flag_set(DISPLAY_INSTRUCTIONS))
            {
                set_flag(DISPLAY_INSTRUCTIONS);
            }

            if(is_flag_set(SW1_EVENT))
            {
                //when SW1 event is detected, set app_state to WAIT
                app_state = WAIT;
                //clear SW1 event flag
                clear_flag(SW1_EVENT);
                //clear DISPLAY_READY flag
                clear_flag(DISPLAY_INSTRUCTIONS);
                //st flag DISPLAY_WAITING
                set_flag(DISPLAY_WAITING);
            }

            break;
        }

        case WAIT:
        {
            //ignore SW1 events if any
            if(is_flag_set(SW1_EVENT))
            {
                clear_flag(SW1_EVENT);
            }

            //if waiting is done, clear WAIT_DONE flag
            if(is_flag_set(WAIT_DONE))
            {
                //set app state to RUNNING
                app_state = RUNNING;
                //clear WAIT_DONE flag
                clear_flag(WAIT_DONE);
                //reset time_count value
                time_count = 0;
                //turn led on
                PORTC |= (1<<PC0);
                //set flag to start counting timer1 value
                set_flag(TIME_COUNT);
            }

            //if SW2 is pressed in advance, set app_state to CHEAT and clear SW2 event
            if (is_flag_set(SW2_EVENT))
            {
                app_state = CHEAT;
                clear_flag(SW2_EVENT);
                //clear DISPLAY_WAITING flag
                clear_flag(DISPLAY_WAITING);
            }

            break;
        }

        case CHEAT:
        {
            if (!is_flag_set(DISPLAY_CHEAT))
            {
                set_flag(DISPLAY_CHEAT);
            }

            if(is_flag_set(SW1_EVENT))
            {
                //when SW1 event is detected, set app_state to WAIT
                app_state = READY;
                //clear SW1 event flag
                clear_flag(SW1_EVENT);
                //clear DISPLAY_CHEAT flag
                clear_flag(DISPLAY_CHEAT);
            }

            break;
        }

        case RUNNING:
        {
            if(is_flag_set(SW2_EVENT) || is_flag_set(TOO_SLOW))
            {
                //reset SW2_EVENT flag
                clear_flag(SW2_EVENT);
                //reset T1_COUNT flag
                clear_flag(TIME_COUNT);
                //set app state to RESULTS
                app_state = RESULTS;
                //turn off led
                PORTC &= ~(1<<PC0);
                //clear DISPLAY_WAITING flag
                clear_flag(DISPLAY_WAITING);
            }

            break;
        }

        case RESULTS:
        {
            //clear SW2_EVENT which may have been generated due to switch bounce
            clear_flag(SW2_EVENT);
            //display too slow message if TOO_SLOW flag is set
            if(is_flag_set(TOO_SLOW))
            {
                set_flag(DISPLAY_TOO_SLOW);
            }

            else
            {
                set_flag(DISPLAY_RESULTS);
            }

            if(is_flag_set(SW1_EVENT))
            {
                //set app state to ready
                app_state = READY;
                //clear SW1_EVENT flag
                clear_flag(SW1_EVENT);
                //reset TOO_SLOW flag
                clear_flag(TOO_SLOW);
                //clear DISPLAY_RESULTS flag
                clear_flag(DISPLAY_RESULTS);
                //clear DISPLAY_TOO_SLOW flag
                clear_flag(DISPLAY_TOO_SLOW);
            }

            break;
        }
    }
}

void task3(void)
{
    time3 = t3;

    static uint8_t new_lcd_state = 0;
    static uint8_t old_lcd_state = 0;

    if(is_flag_set(DISPLAY_READY))
    {
        new_lcd_state = 1;
        if(new_lcd_state != old_lcd_state)
        {
            lcd_reset();
            lcd_print_string_progmem(ready,sizeof(ready)/sizeof(prog_uchar),0x80);
        }
    }

    else if(is_flag_set(DISPLAY_INSTRUCTIONS))
    {
        new_lcd_state = 2;
        if(new_lcd_state != old_lcd_state)
        {
            lcd_reset();
            lcd_print_string_progmem(instructions,sizeof(instructions)/sizeof(prog_uchar),0x80);
            lcd_print_string_progmem(&(instructions[16]),sizeof(instructions)/sizeof(prog_uchar),0xC0);
        }
    }

    else if(is_flag_set(DISPLAY_RESULTS))
    {
        new_lcd_state = 3;
        if(new_lcd_state != old_lcd_state)
        {
            lcd_reset();
            lcd_print_string_progmem(reaction_time,sizeof(reaction_time)/sizeof(prog_uchar),0x80);
            //display reaction time value at location 0x8B
            lcd_print_num(time_count,3,0x8B);

            lcd_print_string_progmem(high_score,sizeof(high_score)/sizeof(prog_uchar),0xC0);
            //read the value of high score in EEPROM to a variable in sram
            high_score_sram = eeprom_read_word(&high_score_eeprom);
            if(time_count < high_score_sram)
            {
                high_score_sram = time_count;
                eeprom_update_word(&high_score_eeprom, high_score_sram);
            }
            //display high score at location 0xCD on lcd
            lcd_print_num(high_score_sram,3,0xCD);
        }
    }

    else if(is_flag_set(DISPLAY_CHEAT))
    {
        new_lcd_state = 4;
        if(new_lcd_state != old_lcd_state)
        {
            lcd_reset();
            lcd_print_string_progmem(cheat,sizeof(cheat)/sizeof(prog_uchar),0x80);
        }
    }

    else if(is_flag_set(DISPLAY_TOO_SLOW))
    {
        new_lcd_state = 5;
        if(new_lcd_state != old_lcd_state)
        {
            lcd_reset();
            lcd_print_string_progmem(too_slow,sizeof(too_slow)/sizeof(prog_uchar),0x80);
            lcd_print_string_progmem(&(too_slow[16]),sizeof(too_slow)/sizeof(prog_uchar),0xC0);
        }
    }

    else if(is_flag_set(DISPLAY_WAITING))
    {
        new_lcd_state = 6;
        if(new_lcd_state != old_lcd_state)
        {
            lcd_reset();
            lcd_print_string_progmem(waiting,sizeof(waiting)/sizeof(prog_uchar),0x80);
        }
    }

    else;

    old_lcd_state = new_lcd_state;
}

//functions to set and reset flags
void set_flag(uint8_t val)
{
    flags |= (1<<val);
}

void clear_flag(uint8_t val)
{
    flags &= ~(1<<val);
}

bool is_flag_set(uint8_t val)
{
    if (flags & (1<<val))
    {
        return (true);
    }

    else
    {
        return (false);
    }
}


