




#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef char  int8_t;
typedef short int16_t;

#include "console.c"

uint16_t random16(void)
{
	static uint16_t seed = 0xdeadbeef;
	
	// xorshift
	seed ^= seed << 7;
	seed ^= seed >> 9;
	seed ^= seed << 8;
	
	return (seed);
}

#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define PS2_COMMAND 0x64


uint8_t ps2_in_data(void)
{
	return inportb(PS2_DATA);
}

#define PIC_MASTER_OFFSET 0x8
#define PIC_SLAVE_OFFSET 0x70

#define PIC1_COMMAND 0x20
#define PIC1_DATA (PIC1_COMMAND + 1)
#define PIC2_COMMAND 0xa0
#define PIC2_DATA (PIC2_COMMAND + 1)
#define PIC_EOI		0x20

#define KBD_IRQ 1
 
void pic_send_eoi(unsigned char irq)
{
	if(irq >= 8)
		outportb(PIC2_COMMAND,PIC_EOI);
 
	outportb(PIC1_COMMAND,PIC_EOI);
}


#define VK_BACK   0x08
#define VK_TAB    0x09
#define VK_RETURN 0x0D
#define VK_ESC    0x1B
#define VK_SPACE  0x20

#define MSG_KEYUP   0x01
#define MSG_KEYDOWN 0x02

struct Kbd_message
{
	uint16_t msg;
	uint16_t vk;
	uint16_t meta; // state of shift, ctrl at the time of press or release
};
typedef struct Kbd_message Kbd_message;

Kbd_message kbd_message_queue[512];
int kbd_message_queue_front = -1;
int kbd_message_queue_back  = 0;




enum Kbd_dirver_state
{
	KBD_DRIVER_STATE_DEFAULT,
	KBD_DRIVER_STATE_BREAK_1,
	KBD_DRIVER_STATE_BREAK_2,
};
typedef enum Kbd_dirver_state Kbd_dirver_state;
Kbd_dirver_state g_kbd_driver_state = KBD_DRIVER_STATE_DEFAULT;

char ascii_buffer[1024];
int ascii_buffer_index = 0;
int g_shift_down;
int g_ctrl_down;

typedef void interrupt (*Interrupt_handler_type)(void);
Interrupt_handler_type old_kbd_handler;

void interrupt new_kbd_handler(void)
{
	uint8_t scan_code;
	
	pic_send_eoi(KBD_IRQ);
	
	scan_code = ps2_in_data();
	switch (g_kbd_driver_state)
	{
		case KBD_DRIVER_STATE_DEFAULT:
		{
			switch (scan_code)
			{
				case 0x02: // 1
				case 0x03: // ...
				case 0x04: // ...
				case 0x05: // ...
				case 0x06: // ...
				case 0x07: // ...
				case 0x08: // ...
				case 0x09: // ...
				case 0x0a: // 9
				case 0x0b: // 0
				{
					char code;
					if (scan_code == 0x0b)
					{
						code = '0';
					}
					else
					{
						code = '0' - 1 + scan_code;
					}
					ascii_buffer[ascii_buffer_index++] = code;
				} break;
				
				
				// first row of letters
				case 0x10: { ascii_buffer[ascii_buffer_index++] = 'Q'; } break;
				case 0x11: { ascii_buffer[ascii_buffer_index++] = 'W'; } break;
				case 0x12: { ascii_buffer[ascii_buffer_index++] = 'E'; } break;
				case 0x13: { ascii_buffer[ascii_buffer_index++] = 'R'; } break;
				case 0x14: { ascii_buffer[ascii_buffer_index++] = 'T'; } break;
				case 0x15: { ascii_buffer[ascii_buffer_index++] = 'Y'; } break;
				case 0x16: { ascii_buffer[ascii_buffer_index++] = 'U'; } break;
				case 0x17: { ascii_buffer[ascii_buffer_index++] = 'I'; } break;
				case 0x18: { ascii_buffer[ascii_buffer_index++] = 'O'; } break;
				case 0x19: { ascii_buffer[ascii_buffer_index++] = 'P'; } break;
				
				// second row of letters
				case 0x1e: { ascii_buffer[ascii_buffer_index++] = 'A'; } break;
				case 0x1f: { ascii_buffer[ascii_buffer_index++] = 'S'; } break;
				case 0x20: { ascii_buffer[ascii_buffer_index++] = 'D'; } break;
				case 0x21: { ascii_buffer[ascii_buffer_index++] = 'F'; } break;
				case 0x22: { ascii_buffer[ascii_buffer_index++] = 'G'; } break;
				case 0x23: { ascii_buffer[ascii_buffer_index++] = 'H'; } break;
				case 0x24: { ascii_buffer[ascii_buffer_index++] = 'J'; } break;
				case 0x25: { ascii_buffer[ascii_buffer_index++] = 'K'; } break;
				case 0x26: { ascii_buffer[ascii_buffer_index++] = 'L'; } break;
				
				case 0x2a:        // left shift
				case 0x36:        // right shift
				case 0x2a + 0x80: // left shift up
				case 0x36 + 0x80: // right shift up
				{
					g_shift_down = 1;
					if (scan_code & 0x80)
					{
						g_shift_down = 0;
					}
					g_kbd_driver_state = KBD_DRIVER_STATE_DEFAULT;
				} break;
				
				case 0x1C: // enter
				{
					ascii_buffer[ascii_buffer_index++] = '\n';
					g_kbd_driver_state = KBD_DRIVER_STATE_DEFAULT;
				} break;
				
				case 0x1d:        // left control
				case 0x1d + 0x80: // left control up
				{
					g_ctrl_down = 1;
					if (scan_code & 0x80)
					{
						g_ctrl_down = 0;
					}
					g_kbd_driver_state = KBD_DRIVER_STATE_DEFAULT;
				} break;
				
				case 0x0E: // backspace
				case 0x0E + 0x80: // backspace up
				{
					if (scan_code & 0x80 == 0)
					{
						ascii_buffer[ascii_buffer_index++] = '\b';
					}		
				} break;
				
				case 0xe0: // break code
				{
					g_kbd_driver_state = KBD_DRIVER_STATE_BREAK_1;
				} break;
			}
		} break;
		
		case KBD_DRIVER_STATE_BREAK_1:
		{
			switch (scan_code)
			{
				case 0x2a: // 0xe0 (0x2a | 0xb7) print screen pressed
				case 0xb7:
				{
					g_kbd_driver_state = KBD_DRIVER_STATE_BREAK_2;
				} break;
				
				case 0x1d:        // right control
				case 0x1d + 0x80: // right control up
				{
					g_ctrl_down = 1;
					if (scan_code & 0x80)
					{
						g_ctrl_down = 0;
					}
					g_kbd_driver_state = KBD_DRIVER_STATE_DEFAULT;
				} break;
				
				default: // 0xe0 0x??
				{
					g_kbd_driver_state = KBD_DRIVER_STATE_DEFAULT;
				} break;
			}
		};
		
		case KBD_DRIVER_STATE_BREAK_2:
		{
			switch (scan_code)
			{
				case 0xe0:
				{
					g_kbd_driver_state = KBD_DRIVER_STATE_BREAK_2;
				} break;
				
				case 0x37: // 0x0e (0x2a | 0xb7) 0x0e (0x37 | 0xb7) print screen pressed
				{
					g_kbd_driver_state = KBD_DRIVER_STATE_DEFAULT;
				} break;
				case 0xaa:
				{
					g_kbd_driver_state = KBD_DRIVER_STATE_DEFAULT;
				} break;
			}
		} break;
	}
	
	/*{
		char* foo = "0x00\n\r";
		char* hex = "0123456789ABCDEF";
		foo[2] = hex[(scan_code >> 4) & 0xF];
		foo[3] = hex[scan_code & 0xF];
		put_str(foo);
	}*/
	
	_enable();
}

int main(void)
{
	//int n = 10;
	int index = 0;
	
	console_set_text_color(BLACK, GREY);
	console_cls();
	
	
	old_kbd_handler = getvect(PIC_MASTER_OFFSET + 1);
	setvect(PIC_MASTER_OFFSET + 1, new_kbd_handler);
	
	for(;;)
	{
		uint8_t c;
		int shift;
		int ctrl;
		
		_disable();
		
		shift = g_shift_down;
		ctrl = g_ctrl_down;
		c = ascii_buffer[index];
		if (c != 0)
		{
			index++;
		}
		
		_enable();
		
		if (c != 0)
		{
			c = !shift ? tolower(c) : c;
			if (ctrl)
			{
				console_set_text_color(BLUE, GREY);
			}
			else
			{
				console_set_text_color(BLACK, GREY);
			}
			put_char(c);
			if (c == '\n')
			{
				put_char('\r');
			}
			if (c == '\b')
			{
				console_place_byte(' ');
			}
		}
		
		//delay(2000);
	}
	
	setvect(PIC_MASTER_OFFSET + 1, old_kbd_handler);
	
	return (0);
}


















