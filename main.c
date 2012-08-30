#include <avr/eeprom.h>
#include <avr/io.h>
#include <math.h>
#include <stdlib.h>

#define BRIGHTNESS_MAX 40
#define FADESTEP 80
#define COLORSTEP 3200

static enum {
	M_RAND = 0, M_CIRCLE, M_STROBO, M_INVAL
} mode = M_RAND;

unsigned char ee_mode EEMEM;

static enum {
	RED = 0, YELLOW, GREEN, CYAN, BLUE, VIOLET, MAGIC
} color = RED;

inline void set_led(char red, char green, char blue)
{
	PORTD = _BV(PD1) | (blue << PD4) | (red << PD5);
	PORTB = _BV(PB6) | (blue << PB0) | (green << PB1);
}

int main (void)
{
	char red, green, blue;
	unsigned int b_red, b_green, b_blue;
	unsigned int n_red, n_green, n_blue;

	unsigned int step = 0;

	unsigned int cnt = 0;
	unsigned char cur = 0;
	unsigned char skip = 0;

	unsigned char pwm[16] = {
		0, 1, 1, 1, 1, 2, 2, 3, 4, 6, 8, 11, 15, 22, 26, 30
	};

	unsigned int cnt_max = BRIGHTNESS_MAX;

	DDRB = _BV(DDB0) | _BV(DDB1) | _BV(DDB4);
	DDRD = _BV(DDD4) | _BV(DDD5);

	MCUSR = 0;
	WDTCSR = _BV(WDE) | _BV(WDP3) | _BV(WDP0);

	ACSR |= _BV(ACD);

	CLKPR = _BV(CLKPCE);
	CLKPR = 0;

	b_red = b_green = b_blue = n_red = n_green = n_blue = 40;

	mode = eeprom_read_byte(&ee_mode);
	if (mode >= M_INVAL)
		mode = M_CIRCLE;

	while (1) {

		cnt++;

		if (cnt > cnt_max) {
			step++;
			cnt = 0;

			if (step == COLORSTEP) {
				step = 0;
				switch (mode) {
				case M_RAND:
					n_red = pwm[rand() % 16];
					n_green = pwm[rand() % 16];
					n_blue = pwm[rand() % 16];
					break;
				case M_CIRCLE:
				case M_STROBO:
					color = (color + 1) % MAGIC;
					n_red = (((color + 1) % MAGIC) < 3) * 30;
					n_green = (((color + 2) % MAGIC) > 2) * 30;
					n_blue = (color > 2) * 30;
					if (mode == M_STROBO) {
						b_red = n_red;
						b_green = n_green;
						b_blue = n_blue;
					}
					break;
				}
			}
			if (!(step % FADESTEP)) {
				asm("wdr");

				if (mode != M_STROBO) {
						if (n_red < b_red) b_red--;
					else if (n_red > b_red) b_red++;
						if (n_green < b_green) b_green--;
					else if (n_green > b_green) b_green++;
						if (n_blue < b_blue) b_blue--;
					else if (n_blue > b_blue) b_blue++;
				}

				if (skip)
					skip--;
				else if (!(PINB & _BV(PB6))) {
					mode = (mode + 1) % M_INVAL;
					skip = 10;
					n_red = n_green = n_blue = b_red = b_green = b_blue = 0;
					eeprom_write_byte(&ee_mode, mode);
				}
			}
			if (mode == M_STROBO) {
				if ((step % 60) == 0)
					b_red = b_green = b_blue = 0;
				else if ((step % 60) == 30) {
					b_red = (((color + 1) % MAGIC) < 3) * 40;
					b_green = (((color + 2) % MAGIC) > 2) * 40;
					b_blue = (color > 2) * 40;
				}
			}
		}

		cur = cnt % BRIGHTNESS_MAX;

		if (!cur) {
			if (b_red)
				red = 1;
			if (b_green)
				green = 1;
			if (b_blue)
				blue = 1;
		}

		if (cur == b_red)
			red = 0;
		if (cur == b_green)
			green = 0;
		if (cur == b_blue)
			blue = 0;

		set_led(red, green, blue);

	}

	return 0;
}
