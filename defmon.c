#define F_CPU 20000000L

#define USART_BAUDRATE 31250
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdbool.h>

int main(void) {

	int LED_pin = 3, button_pin = 1, blink_counter = 0, i, div1 = 0, divtab[4] = {1,3,6,12}, counter3;
	long hold_counter = 0, blink_counter2 = 0, div2;
	bool gb_mode = true, sync, state = true, button_state = true, button, nano = true, midi_state = true, div_state = false, button_state2, start = true;

	DDRB |= _BV(3)|_BV(4)|_BV(0)|_BV(2); // led pins as outputs; gb pins as outputs (0 - lsdj, 2 - nl)
	DDRB &= ~_BV(button_pin); // button pin as input
	DDRC |= _BV(0)|_BV(1)|_BV(5); // sync24out pin as output (0,1 - sync, 5 - en)
	DDRD &= ~(_BV(5)|_BV(6)); // sync24in pins as inputs (5 - sync, 6 - en)
	PORTB |= _BV(button_pin); // internal pullup on the button input
	PORTB |= _BV(LED_pin); //led on

	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
	UBRR0H = (BAUD_PRESCALE >> 8);
	UBRR0L = BAUD_PRESCALE;

	while (1) { 
		if (start) {
			while (!eeprom_is_ready()); 
			counter3 = eeprom_read_byte(0);
			if (counter3 != 1 && counter3 != 3 && counter3 != 6 && counter3 != 12) counter3 = 1;
			div2 = counter3;
			start = false;
			}
		button = PINB & _BV(button_pin); 
		if (!button && button_state) { 
			gb_mode = !gb_mode;
			button_state = false;
			button_state2 = true;
			if (LED_pin == 3) {
				PORTB &= ~_BV(LED_pin);
				LED_pin = 4;
				PORTB |= _BV(LED_pin);
                }
			else {
				PORTB &= ~_BV(LED_pin);
				LED_pin = 3;
				PORTB |= _BV(LED_pin);
				}
			_delay_ms(10);
			}
		if (button && !button_state) {
			button_state = true;
			hold_counter = 0;
			_delay_ms(10);
			}
		if (!button_state && button_state2) {
			hold_counter++;
			if (hold_counter > 500000) {
				div_state = true;
				hold_counter = 0;
				button_state2 = false;
				}
			while(div_state) {
				blink_counter2++;
				if (blink_counter2 > div2 * 8000) {
						blink_counter2 = 0;
						if (LED_pin == 3) {
							PORTB &= ~_BV(LED_pin);
							LED_pin = 4;
							PORTB |= _BV(LED_pin);
							}
						else {
							PORTB &= ~_BV(LED_pin);
							LED_pin = 3;
							PORTB |= _BV(LED_pin);
							}
						}
				button = PINB & _BV(button_pin); 
				if (!button && button_state) {
					button_state = false;
					div1++;
					if(div1 > 3) div1 = 0;
					div2 = divtab[div1];
					counter3 = div2;
					button_state2 = true;
					_delay_ms(10);
					}
				if (button && !button_state) {
					button_state = true;
					hold_counter = 0;
					_delay_ms(10);
					}
				if (!button_state && button_state2) {
					hold_counter++;
					if (hold_counter > 500000) {
						while (!eeprom_is_ready());
						eeprom_update_byte(0, counter3);
						div_state = false;
						hold_counter = 0;
						button_state2 = false;
						}
					}	
					
				}
			}
			
  
		if (PIND & _BV(6)) {
			if (midi_state) {
				while (!(UCSR0A & (1<<UDRE0))); // Do nothing until UDR is ready for more data to be written to it
				UDR0 = 0xFA; // Send out the byte value in the variable "ByteToSend"
				PORTC |= _BV(5); // sync24 en out on
				midi_state = false;
				}
			sync = PIND & _BV(5);
			if (sync && state) {
				if (gb_mode) {
					PORTB &= ~_BV(2);
					for(i = 0; i < 8; i ++) {
						PORTB &= ~_BV(0);
						_delay_us(60);
						PORTB |= _BV(0);
						}
					}
				else {
					if (nano) PORTB |= _BV(2);
					else PORTB &= ~_BV(2);
					nano = !nano;
					}

				while (!(UCSR0A & (1<<UDRE0))); // Do nothing until UDR is ready for more data to be written to it
				UDR0 = 0xF8; // Send out the byte value in the variable "ByteToSend"
					
				if (counter3 == div2) {
					PORTC |= _BV(0); // sync24 sync out high
					counter3 = 0;
					}
					counter3++;
				PORTC |= _BV(1); // sync24 sync out high
				_delay_ms(5);
				PORTC &= ~_BV(0); // sync24 sync out low
				PORTC &= ~_BV(1); // sync24 sync out low
	  
				state = false;
				blink_counter++;
				if (blink_counter == 6) {
					PORTB |= _BV(LED_pin);
					blink_counter = 0;
					}
					else PORTB &= ~_BV(LED_pin);
				}
			if (!sync && !state) state = true;
			}
		else {
			if (!midi_state) {
				while (!(UCSR0A & (1<<UDRE0))); // Do nothing until UDR is ready for more data to be written to it
				UDR0 = 0xFC; // Send out the byte value in the variable "ByteToSend"
				blink_counter = 0;
				PORTB &= ~_BV(2);
				PORTB |= _BV(LED_pin);
				nano = true;
				state = true;
				PORTC &= ~_BV(5); // sync en out off
				midi_state = true;
				counter3 = div2;
				}
			}
		}
	}