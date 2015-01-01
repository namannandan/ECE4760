fndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED


//define cpu clock frequency
#ifndef F_CPU
#define F_CPU 8000000UL
#endif


//_____Libraries_____
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdlib.h>


//_____Custom libraries_____
#include "lcd.h"
#include "avr_delay.h"


//_____Constants_____
//scheduler constants
//interval for debouncing buttons (30ms)
#define BUTTON_TIMEOUT 30
//interval for handling button events (100ms)
#define BUTTON_EVENT_HANDLER_TIMEOUT 100
//interval for updating measured value (200ms)
#define MEASUREMENT_TIMEOUT 200
//interval for autoranging (300ms)
#define AUTORANGING_TIMEOUT 300
//interval for updating lcd (500ms)
#define LCD_TIMEOUT 500

//application states (frequency, voltage or resistance measurement)
#define FREQUENCY 0
#define VOLTAGE 1
#define RESISTANCE 2

//resistors used to form voltage divider (used for resistance measurement)
//R_0 (1 Kohm)
#define R_0 0
#define R_0_CONFIG DDRD
#define R_0_PORT PORTD
#define R_0_LOC PD5
//R_1 (10 Kohm)
#define R_1 1
#define R_1_CONFIG DDRC
#define R_1_PORT PORTC
#define R_1_LOC PC1

//button configuration (used to get input from user)
//button 0 (used to chang application state i.e the quantity being measured : frequency, voltage or resistance)
#define BUTTON_0_PORT PINC
#define BUTTON_0_LOC PC2
//button 1 (used to toggle autoranging on or off)
#define BUTTON_1_PORT PIND
#define BUTTON_1_LOC PD2
//button 2 (used to change measurement range manually when autoranging is disabled)
#define BUTTON_2_PORT PIND
#define BUTTON_2_LOC PD3

//button debounce state machine
//states
#define MAY_BE_PUSH 0
#define PUSH 1
#define MAY_BE_NO_PUSH 2
#define NO_PUSH 3

//flags for inter task communication
//lcd flags
//flag to request for update of lcd
#define UPDATE_LCD 0
//application state change
#define APP_STATE_CHANGE 1
//measured value change
#define MEASURED_VALUE_CHANGE 2
//autoranging
#define AUTORANGING 3
//frequency measurement
//frequency autoranging update (this flag will be set based on timer 1 overflow and input capture ISRs)
//or it may be set when manually changing the prescaler
#define INCREASE_PRESCALER 4
//display selected range on lcd
#define RANGE_DISPLAY_UPDATE 5
//button events
//button 0 event
#define BUTTON_0_EVENT 6
//button 1 event
#define BUTTON_1_EVENT 7
//button 2 event
#define BUTTON_2_EVENT 8


//_____Global variables_____
//strings to be stored in flash memory
const prog_uchar initial_message[] PROGMEM = {"AUTORANGING     MULTIMETER"};
const prog_uchar frequency_string[] PROGMEM = {"Frequency(Hz):-"};
const prog_uchar prescaler_string[] PROGMEM = {"PSc"};
const prog_uchar auto_range_string[] PROGMEM = {"A"};
const prog_uchar manual_range_string[] PROGMEM = {"M"};
const prog_uchar voltage_string[] PROGMEM = {"VOLTAGE(V):-"};
const prog_uchar vref_string[] PROGMEM = {"VRef"};
const prog_uchar resistance_string[] PROGMEM = {"RESISTANCE(Kohm)"};
const prog_uchar rref_string[] PROGMEM = {"Rref"};
const prog_uchar r_0_string[] PROGMEM = {"1K"};
const prog_uchar r_1_string[] PROGMEM = {"10K"};

//application
//set default application state to frequency measurement
uint8_t app_state = FREQUENCY;

//frequency measurement
volatile uint32_t frequency = 0;
volatile uint32_t old_frequency = 0;
volatile uint16_t tick_val = 0;
//autoranging values
const uint16_t prescaler_values[5] = {1, 8, 64, 256, 1024};
//lower limit of frequency measurement for the above prescaler values
const uint16_t frequency_lower[5] = {2000, 1000, 500, 100, 0};
//initial default prescaler is 1
uint8_t prescaler_index = 0;
uint16_t prescaler = 1;

//voltage measurement
volatile uint16_t adch = 0;
volatile uint16_t adcl = 0;
volatile float voltage = 0;
volatile float new_voltage = 0;
//default value of VREF is 5.0V
volatile float vref = 5.0;

//resistance measurement
float resistance = 0;
//reference resistor selected initially is R_0 (1 Kohm)
uint8_t ref_resistance = R_0;
//reference resistance value is by default set to 1Kohm
uint16_t ref_resistance_val = 1000;

//scheduler related variables
volatile uint16_t button_time_count = BUTTON_TIMEOUT;
volatile uint16_t button_event_handler_time_count = BUTTON_EVENT_HANDLER_TIMEOUT;
volatile uint16_t measurement_time_count = MEASUREMENT_TIMEOUT;
volatile uint16_t autoranging_time_count = AUTORANGING_TIMEOUT;
volatile uint16_t lcd_time_count = LCD_TIMEOUT;

//flags (used for inter task communication)
volatile uint16_t flags = 0;

//push buttons
//declare initial state of all three buttons
uint8_t button_0_push_state = NO_PUSH;
uint8_t button_1_push_state = NO_PUSH;
uint8_t button_2_push_state = NO_PUSH;


//_____Function prototypes_____
//initialization task
void init(void);

//tasks
//task used to detect button events
void button_task(void);
//task used to handle button events
void button_event_handler_task(void);
//task used to perform frequency, voltage or resistance measuement
void measurement_task(void);
//task used to perform autoranging
void autoranging_task(void);
//task used to handle lcd
void lcd_task(void);

//inter task communication
void set_flag(uint8_t val);
void clear_flag(uint8_t val);
bool is_flag_set(uint8_t val);
void toggle_flag(uint8_t val);


#endif // MAIN_H_INCLUDED

