fndef LCD_H_INCLUDED
#define LCD_H_INCLUDED

/*include avr/pgmspace.h so that when lcd.h is included in main,
main knows how the data type "prog_uchar" is defined*/
#include <avr/pgmspace.h>

void lcd_init(void);
void lcd_cmd(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_reset(void);
void lcd_demo(void);
void lcd_print_string(const char* ptr, uint8_t num_chars, char loc);
void lcd_print_string_progmem(const prog_uchar* ptr, uint8_t num_chars, char loc);
void lcd_print_num(uint16_t val, uint8_t num_digits, char loc);
void lcd_clear_segment(uint8_t num_segments, char loc);

#endif // LCD_H_INCLUDED

