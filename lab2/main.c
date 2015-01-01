/* Hardware config
LCD :-
Control port = PORTC
RS-> PC3, RW-> PC4, EN-> PC5
Data port = PORTB

Buttons and led :-
button_0 is connected to PC2
botton_1 is connected to PD2
button_2 is connected to PD3
R_0 (1Kohm) is connected to PD5
R_1 (1Kohm) is connected to PC1

Frequency measurement probe is connected to AIN1
Voltage and resistance measurement probe is connected to PC0
*/


//include "main.h"
#include "main.h"


//_____Interrupt vectors_____
//frequency measurement
//timer1 capture vector
ISR (TIMER1_CAPT_vect)
{
    //reset timer1 count value
    TCNT1 = 0;
    //record the timer1 capture value
    tick_val = ICR1;
    //calculate frequency
    frequency = F_CPU/(prescaler*tick_val);

    return;
}

//timer1 overflow vector
ISR(TIMER1_OVF_vect)
{
    //avoid timer1 overflow interrupt before input capture interrupt by increasing the prescaler value
    //set INCREASE_PRESCALER flag
    set_flag(INCREASE_PRESCALER);
    //set frequency to 0
    frequency = 0;

    return;
}

//timer 2 interrupt on compare match
ISR (TIMER2_COMPA_vect)
{
    if(button_time_count > 0)
    {
        button_time_count--;
    }

    if(button_event_handler_time_count > 0)
    {
        button_event_handler_time_count--;
    }

    if(measurement_time_count > 0)
    {
        measurement_time_count --;
    }

    if(autoranging_time_count > 0)
    {
        autoranging_time_count --;
    }

    if(lcd_time_count > 0)
    {
        lcd_time_count --;
    }

    return;
}

//ADC interrupt vector
ISR(ADC_vect)
{
    //read ADC value
    adcl = (uint16_t) ADCL;
    adch = ((uint16_t) ADCH) * 256;
    //calculate the value of voltage
    new_voltage = ((adch + adcl)*vref)/1023.0;

    return;
}


//_____MAIN_____
int main(void)
{
    //initialize hardware
    init();

    //scheduler
    while(1)
    {
        if(button_time_count == 0)
        {
            button_task();
        }

        if(button_event_handler_time_count == 0)
        {
            button_event_handler_task();
        }

        if(measurement_time_count == 0)
        {
            measurement_task();
        }

        if(autoranging_time_count == 0)
        {
            autoranging_task();
        }

        if(lcd_time_count == 0)
        {
            lcd_task();
        }
    }

    return (0);
}


//______Functions_______
void init(void)
{
    //initialize lcd
    lcd_init();

    //configure timer1 for input capture
    //enable input capture noise canceller and set input edge capture (positive edge)
    TCCR1B |= ((1<<ICNC1)|(1<<ICES1));
    //timer1 prescaling
    TCCR1B |= (0x07 & (prescaler_index+1));
    //enable input capture interrupt and overflow interrupt for timer 1
    TIMSK1 |= ((1<<ICIE1) | (1<<TOIE1));

    //connect the positive input of the comparator to a voltage divider (two 10k resistors, connected in series btw VCC and GND)
    //therefore internal bandgap reference, "ACBG" bit is not set
    //analog comparator input capture enable
    ACSR |= (1<<ACIC);
    //Note :- negative input to the analog comparator by default is pin AIN1

    //configure timer2 to generate 1ms time base (used for scheduling tasks)
    //enable interrupt on compare match
    TIMSK2 |= (1<<OCIE2A);
    //set the appropriate compare value
    OCR2A = 124;
    //set prescalar to 64 and timer 2 to CTC mode
    TCCR2B |= (1<<CS22);
    TCCR2A |= (1<<WGM21);

    //ADC configuration
    //initially set reference voltage equal to AVCC
    //left adjust of ADC result is disabled (ADLAR = 0)
    //ADC0 is the input channel (MUX3:0 = 0000)
    ADMUX |= (1<<REFS0);
    //enable ADC "ADEN"
    //enable ADC iterrupt flag "ADIE"
    //set adc prescaler to 64 (ADPS2 = 1 and ADPS1 = 1)
    ADCSRA |= ((1<<ADEN) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1));

    //IO pins to which buttons are connected are configured as inputs by default (DDRX = 0)
    //external pull-up resistors need to be connected

    //enable R_0 by default for resistance measurement
    R_0_CONFIG |= (1<<R_0_LOC);
    R_0_PORT |= (1<<R_0_LOC);

    //print initial message to lcd
    lcd_print_string_progmem(initial_message, 16, 0x80);
    lcd_print_string_progmem(&(initial_message[16]), 16, 0xC0);
    delayms(2000);

    //display initial default app state on lcd
    //set APP_STATE_CHANGE flag
    set_flag(APP_STATE_CHANGE);
    //enable autoranging by default
    set_flag(AUTORANGING);
    //set RANGE_DISPLAY_UPDATE flag
    set_flag(RANGE_DISPLAY_UPDATE);
    //set UPDATE_LCD flag
    set_flag(UPDATE_LCD);

    //enable global interrupts
    sei();

    return;
}

//tasks
//this task is used to detect button events
void button_task(void)
{
    //reset the value of button_time_count
    button_time_count = BUTTON_TIMEOUT;

    //update button_0 state
    switch (button_0_push_state)
    {
        case NO_PUSH:
        {
            if(~BUTTON_0_PORT & (1<<BUTTON_0_LOC))
            {
                button_0_push_state = MAY_BE_PUSH;
            }

            else
            {
                button_0_push_state = NO_PUSH;
            }

            break;
        }

        case MAY_BE_PUSH:
        {
            if(~BUTTON_0_PORT & (1<<BUTTON_0_LOC))
            {
                button_0_push_state = PUSH;
            }

            else
            {
                button_0_push_state = NO_PUSH;
            }

            break;
        }

        case PUSH:
        {
            if(~BUTTON_0_PORT & (1<<BUTTON_0_LOC))
            {
                button_0_push_state = PUSH;
            }

            else
            {
                button_0_push_state = MAY_BE_NO_PUSH;
            }

            break;
        }

        case MAY_BE_NO_PUSH:
        {
            if(~BUTTON_0_PORT & (1<<BUTTON_0_LOC))
            {
                button_0_push_state = PUSH;
            }

            else
            {
                button_0_push_state = NO_PUSH;

                //button 0 has been pushed and released (change app_state)
                set_flag(BUTTON_0_EVENT);
            }

            break;
        }
    }

    //update button 1 state
    switch (button_1_push_state)
    {
        case NO_PUSH:
        {
            if(~BUTTON_1_PORT & (1<<BUTTON_1_LOC))
            {
                button_1_push_state = MAY_BE_PUSH;
            }

            else
            {
                button_1_push_state = NO_PUSH;
            }

            break;
        }

        case MAY_BE_PUSH:
        {
            if(~BUTTON_1_PORT & (1<<BUTTON_1_LOC))
            {
                button_1_push_state = PUSH;
            }

            else
            {
                button_1_push_state = NO_PUSH;
            }

            break;
        }

        case PUSH:
        {
            if(~BUTTON_1_PORT & (1<<BUTTON_1_LOC))
            {
                button_1_push_state = PUSH;
            }

            else
            {
                button_1_push_state = MAY_BE_NO_PUSH;
            }

            break;
        }

        case MAY_BE_NO_PUSH:
        {
            if(~BUTTON_1_PORT & (1<<BUTTON_1_LOC))
            {
                button_1_push_state = PUSH;
            }

            else
            {
                button_1_push_state = NO_PUSH;

                //button 1 has been pushed and released (toggle autoranging)
                set_flag(BUTTON_1_EVENT);
            }

            break;
        }
    }

    //update button 2 state
    switch (button_2_push_state)
    {
        case NO_PUSH:
        {
            if(~BUTTON_2_PORT & (1<<BUTTON_2_LOC))
            {
                button_2_push_state = MAY_BE_PUSH;
            }

            else
            {
                button_2_push_state = NO_PUSH;
            }

            break;
        }

        case MAY_BE_PUSH:
        {
            if(~BUTTON_2_PORT & (1<<BUTTON_2_LOC))
            {
                button_2_push_state = PUSH;
            }

            else
            {
                button_2_push_state = NO_PUSH;
            }

            break;
        }

        case PUSH:
        {
            if(~BUTTON_2_PORT & (1<<BUTTON_2_LOC))
            {
                button_2_push_state = PUSH;
            }

            else
            {
                button_2_push_state = MAY_BE_NO_PUSH;
            }

            break;
        }

        case MAY_BE_NO_PUSH:
        {
            if(~BUTTON_2_PORT & (1<<BUTTON_2_LOC))
            {
                button_2_push_state = PUSH;
            }

            else
            {
                button_2_push_state = NO_PUSH;

                //button 2 has been pushed and released (change measurement range)
                if(!is_flag_set(AUTORANGING))
                {
                    set_flag(BUTTON_2_EVENT);
                }
            }

            break;
        }
    }
}

//this task is used to handle button events
void button_event_handler_task(void)
{
    if(is_flag_set(BUTTON_0_EVENT))
    {
        //update app_state
        if(app_state < 2)
        {
            app_state++;
        }

        else
        {
            app_state = 0;
        }

        //if app_state changed to RESISTANCE,
        //set the ADC reference voltage to 5.0V by default
        if(app_state == RESISTANCE)
        {
            if(vref == 1.1)
            {
                ADMUX &= ~(1<<REFS1);
                vref = 5.0;
            }
        }

        //enable auto ranging by default
        set_flag(AUTORANGING);
        //set APP_STATE_CHANGE flag
        set_flag(APP_STATE_CHANGE);
        //set RANGE_DISPLAY_UPDATE flag
        set_flag(RANGE_DISPLAY_UPDATE);
        //set UPDATE_LCD flag
        set_flag(UPDATE_LCD);
        //cler BUTTON_0_EVENT flag
        clear_flag(BUTTON_0_EVENT);
    }

    if(is_flag_set(BUTTON_1_EVENT))
    {
        //toggle autoranging
        toggle_flag(AUTORANGING);
        //set RANGE_DISPLAY_UPDATE flag
        set_flag(RANGE_DISPLAY_UPDATE);
        //set UPDATE_LCD flag
        set_flag(UPDATE_LCD);
        //clear BUTTON_1_EVENT flag
        clear_flag(BUTTON_1_EVENT);
    }

    if(is_flag_set(BUTTON_2_EVENT))
    {
        if(app_state == FREQUENCY)
        {
            //cycle through the available "prescaler" values
            if(prescaler_index < ((sizeof(prescaler_values)/sizeof(prescaler_values[0]))-1))
            {
                //increase prescaler
                prescaler = prescaler_values[++prescaler_index];
                //modify TCCR1B register to set the appropriate prescaler
                TCCR1B &= ~(0x07);
                TCCR1B |= (0x07 & (prescaler_index+1));
            }

            else
            {
                //reset prescaler to initial value
                prescaler_index = 0;
                prescaler = prescaler_values[prescaler_index];
                //modify TCCR1B register to set the appropriate prescaler
                TCCR1B &= ~(0x07);
                TCCR1B |= (0x07 & (prescaler_index+1));
            }
        }

        else if(app_state == VOLTAGE)
        {
            //cycle through the available vref values
            if(vref == 5.0)
            {
                //change vref to 1.1v
                ADMUX |= (1<<REFS1);
                vref = 1.1;
            }

            else
            {
                ADMUX &= ~(1<<REFS1);
                vref = 5.0;
            }
        }

        else if(app_state == RESISTANCE)
        {
            //cycle through the available "ref_resistance_val" values
            if(ref_resistance == R_0)
            {
                //change the reference resistance to 10Kohm
                //disable 1Kohm resistance
                R_0_PORT &= ~(1<<R_0_LOC);
                R_0_CONFIG &= ~(1<<R_0_LOC);
                //enable 10Kohm resistor
                R_1_CONFIG |= (1<<R_1_LOC);
                R_1_PORT |= (1<<R_1_LOC);
                ref_resistance = R_1;
                ref_resistance_val = 10000;
            }

            else
            {
                //change the reference resistance to 1Kohm
                //disable 10Kohm resistance
                R_1_PORT &= ~(1<<R_1_LOC);
                R_1_CONFIG &= ~(1<<R_1_LOC);
                //enable 10Kohm resistor
                R_0_CONFIG |= (1<<R_0_LOC);
                R_0_PORT |= (1<<R_0_LOC);
                ref_resistance = R_0;
                ref_resistance_val = 1000;
            }
        }

        //update range display on lcd
        set_flag(RANGE_DISPLAY_UPDATE);
        //set UPDATE_LCD flag
        set_flag(UPDATE_LCD);
        //cler "BUTTON_2_EVENT" flag
        clear_flag(BUTTON_2_EVENT);
    }
}

//this task is used to measure frequency, voltage and resistance
void measurement_task(void)
{
    //reset measurement_time_count
    measurement_time_count = MEASUREMENT_TIMEOUT;

    if(app_state == FREQUENCY)
    {
        //if the measured frequency changed, update it on lcd screen
        if(frequency != old_frequency)
        {
            //set MEASURED_VALUE_CHANGE
            set_flag(MEASURED_VALUE_CHANGE);
            //set UPDATE_LCD flag
            set_flag(UPDATE_LCD);
            old_frequency = frequency;
        }
    }

    else if((app_state == VOLTAGE) | (app_state == RESISTANCE))
    {
        if(new_voltage != voltage)
        {
            voltage = new_voltage;

            if(app_state == RESISTANCE)
            {
                //calculate the resistance value
                resistance = (voltage * ref_resistance_val) / (5.0 - voltage);
                //convert to Kohms
                resistance = resistance/1000.0;
            }

            //set MEASURED_VALUE_CHANGE
            set_flag(MEASURED_VALUE_CHANGE);
            //set UPDATE_LCD flag
            set_flag(UPDATE_LCD);
        }

        //if an ADC conversion is not currently going on, start a new conversion
        if (~ADCSRA & (1<<ADSC))
        {
            ADCSRA |= (1<<ADSC);
        }
    }
}

//this task is used to perform autoranging
void autoranging_task(void)
{
    //reset autoranging_time_count
    autoranging_time_count = AUTORANGING_TIMEOUT;

    if(is_flag_set(AUTORANGING))
    {
        switch(app_state)
        {
            case (FREQUENCY):
            {
                if(is_flag_set(INCREASE_PRESCALER))
                {
                    if(prescaler_index < ((sizeof(prescaler_values)/sizeof(prescaler_values[0]))-1))
                    {
                        //update the prescaler value
                        //increase the prescaler value, since timer1 overflows before input capture occurs
                        prescaler = prescaler_values[++prescaler_index];
                        //modify TCCR1B register to set the appropriate prescaler
                        TCCR1B &= ~(0x07);
                        TCCR1B |= (0x07 & (prescaler_index+1));
                        //update range display on lcd
                        set_flag(RANGE_DISPLAY_UPDATE);
                        //set UPDATE_LCD flag
                        set_flag(UPDATE_LCD);
                    }

                    //cler INCREASE_PRESCALER flag
                    clear_flag(INCREASE_PRESCALER);
                }

                else if(prescaler_index > 0)
                {
                    if(frequency > frequency_lower[prescaler_index-1])
                    {
                        //update the prescaler value
                        //decrease prescaler value so that frequency can be measured with higher precision
                        prescaler = prescaler_values[--prescaler_index];
                        //modify TCCR1B register to set the appropriate prescaler
                        TCCR1B &= ~(0x07);
                        TCCR1B |= (0x07 & (prescaler_index+1));
                        //update range display on lcd
                        set_flag(RANGE_DISPLAY_UPDATE);
                        //set UPDATE_LCD flag
                        set_flag(UPDATE_LCD);
                    }
                }

                break;
            }

            case (VOLTAGE):
            {
                if(voltage > 0.8)
                {
                    if(vref == 1.1)
                    {
                        //voltage is greater than 0.8v and vref is 1.1v, so change vref to 5.0v
                        ADMUX &= ~(1<<REFS1);
                        vref = 5.0;
                        //update range display on lcd
                        set_flag(RANGE_DISPLAY_UPDATE);
                        //set UPDATE_LCD flag
                        set_flag(UPDATE_LCD);
                    }
                }

                else if(voltage < 1.0)
                {
                    if(vref == 5.0)
                    {
                        //voltage is lesser than 1v and vref is 5.0v, so change vref to 1.1v
                        ADMUX |= (1<<REFS1);
                        vref = 1.1;
                        //update range display on lcd
                        set_flag(RANGE_DISPLAY_UPDATE);
                        //set UPDATE_LCD flag
                        set_flag(UPDATE_LCD);
                    }
                }

                break;
            }

            case(RESISTANCE):
            {
                //if measured resistance is greater than 8Kohm, change reference resistance to 10Kohm
                if(resistance > 8.0 && ref_resistance == R_0)
                {
                    //change the reference resistance to 10Kohm
                    //disable 1Kohm resistance
                    R_0_PORT &= ~(1<<R_0_LOC);
                    R_0_CONFIG &= ~(1<<R_0_LOC);
                    //enable 10Kohm resistor
                    R_1_CONFIG |= (1<<R_1_LOC);
                    R_1_PORT |= (1<<R_1_LOC);
                    ref_resistance = R_1;
                    ref_resistance_val = 10000;
                    //update range display on lcd
                    set_flag(RANGE_DISPLAY_UPDATE);
                    //set UPDATE_LCD flag
                    set_flag(UPDATE_LCD);
                }

                //if measured resistance is lesser than 6Kohm, change reference resistance to 1Kohm
                else if(resistance < 6.0 && ref_resistance == R_1)
                {
                    //change the reference resistance to 1Kohm
                    //disable 10Kohm resistance
                    R_1_PORT &= ~(1<<R_1_LOC);
                    R_1_CONFIG &= ~(1<<R_1_LOC);
                    //enable 10Kohm resistor
                    R_0_CONFIG |= (1<<R_0_LOC);
                    R_0_PORT |= (1<<R_0_LOC);
                    ref_resistance = R_0;
                    ref_resistance_val = 1000;
                    //update range display on lcd
                    set_flag(RANGE_DISPLAY_UPDATE);
                    //set UPDATE_LCD flag
                    set_flag(UPDATE_LCD);
                }

                break;
            }
        }
    }
}

//this task is used to control the lcd
void lcd_task(void)
{
    //reset lcd_time_count
    lcd_time_count = LCD_TIMEOUT;

    //handle any flags if they are set
    if(is_flag_set(UPDATE_LCD))
    {
        if(is_flag_set(APP_STATE_CHANGE))
        {
            if(app_state == FREQUENCY)
            {
                //reset the lcd screen
                lcd_reset();
                //write the appropriate heading string to lcd
                lcd_print_string_progmem(frequency_string, sizeof(frequency_string)/sizeof(frequency_string[0]),0x80);
                lcd_print_string_progmem(prescaler_string, sizeof(prescaler_string)/sizeof(prescaler_string[0]),0xC7);
            }

            else if(app_state == VOLTAGE)
            {
                //reset lcd
                lcd_reset();
                //write the appropriate heading string to lcd
                lcd_print_string_progmem(voltage_string, sizeof(voltage_string)/sizeof(voltage_string[0]),0x80);
                lcd_print_string_progmem(vref_string, sizeof(vref_string)/sizeof(vref_string[0]),0xC6);
            }

            else if(app_state == RESISTANCE)
            {
                //reset lcd
                lcd_reset();
                //write the appropriate heading string to lcd
                lcd_print_string_progmem(resistance_string, sizeof(resistance_string)/sizeof(resistance_string[0]),0x80);
                lcd_print_string_progmem(rref_string, sizeof(rref_string)/sizeof(rref_string[0]),0xC7);
            }

            //clear APP_STATE_CHANGE flag
            clear_flag(APP_STATE_CHANGE);
        }

        if(is_flag_set(MEASURED_VALUE_CHANGE))
        {
            if(app_state == FREQUENCY)
            {
                //clear the original number present
                lcd_clear_segment(7,0xC0);
                //print the new frequency value
                //number of digits is 7 (max. measurable frequency is 8MHz)
                lcd_print_num(frequency,7,0xC0);
            }

            else if(app_state == VOLTAGE)
            {
                //clear the original number present
                lcd_clear_segment(6,0xC0);
                //print the new voltage value
                //number of digits is 4
                char temp[6];
                dtostrf(voltage, 4, 3, temp);
                lcd_print_string(temp, 5, 0xC0);
            }

            else if(app_state == RESISTANCE)
            {
                //clear the original number present
                lcd_clear_segment(6,0xC0);
                //print the resistance value
                //number of digits is 5
                char temp[7];
                dtostrf(resistance, 5, 3, temp);
                lcd_print_string(temp, 6, 0xC0);
            }

            //clear MEASURED_VALUE_CHANGE flag
            clear_flag(MEASURED_VALUE_CHANGE);
        }

        if(is_flag_set(RANGE_DISPLAY_UPDATE))
        {
            if(app_state == FREQUENCY)
            {
                //clear the portion of the display used for displaying the prescaler value
                lcd_clear_segment(4,0xCA);

                if(is_flag_set(AUTORANGING))
                {
                    //display character "A" to indicate autoranging
                    lcd_print_string_progmem(auto_range_string, sizeof(auto_range_string)/sizeof(auto_range_string[0]), 0xCF);
                }

                else
                {
                    //display character "M" to indicate autoranging
                    lcd_print_string_progmem(manual_range_string, sizeof(manual_range_string)/sizeof(manual_range_string[0]), 0xCF);
                }

                //display the selected prescaler
                lcd_print_num(prescaler, 4, 0xCA);
            }

            else if(app_state == VOLTAGE)
            {
                //clear the portion of the display used for displaying the vref value
                lcd_clear_segment(4,0xCA);

                if(is_flag_set(AUTORANGING))
                {
                    //display character "A" to indicate autoranging
                    lcd_print_string_progmem(auto_range_string, sizeof(auto_range_string)/sizeof(auto_range_string[0]), 0xCF);
                }

                else
                {
                    //display character "M" to indicate autoranging
                    lcd_print_string_progmem(manual_range_string, sizeof(manual_range_string)/sizeof(manual_range_string[0]), 0xCF);
                }

                //display the selected vref value
                char temp[5];
                dtostrf(vref, 3, 2, temp);
                lcd_print_string(temp, 4, 0xCA);
            }

            else if(app_state == RESISTANCE)
            {
                //clear the portion of the display used for displaying the rref value
                lcd_clear_segment(3,0xCB);

                if(is_flag_set(AUTORANGING))
                {
                    //display character "A" to indicate autoranging
                    lcd_print_string_progmem(auto_range_string, sizeof(auto_range_string)/sizeof(auto_range_string[0]), 0xCF);
                }

                else
                {
                    //display character "M" to indicate autoranging
                    lcd_print_string_progmem(manual_range_string, sizeof(manual_range_string)/sizeof(manual_range_string[0]), 0xCF);
                }

                //display the selected vref value
                if(ref_resistance == R_0)
                {
                    lcd_print_string_progmem(r_0_string, sizeof(r_0_string)/sizeof(r_0_string[0]), 0xCB);
                }

                else
                {
                    lcd_print_string_progmem(r_1_string, sizeof(r_1_string)/sizeof(r_1_string[0]), 0xCB);
                }
            }

            //clear RANGE_DISPLAY_UPDATE flag
            clear_flag(RANGE_DISPLAY_UPDATE);
        }

        //clear UPDATE_LCD flag
        clear_flag(UPDATE_LCD);
    }
}

//inter task communication using flags
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

void toggle_flag(uint8_t val)
{
    flags ^= (1<<val);
}

