


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

#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define PS2_COMMAND 0x64


uint8_t ps2_in_data(void)
{
	return inportb(PS2_DATA);
}

struct KBD_message
{
	uint16_t msg;
	uint16_t vk;

	/*
		(1 << 0) : shift state
		(2 << 0) : ctrl state
	*/
	uint16_t flags; // state of shift, ctrl at the time of press or release
};
typedef struct KBD_message KBD_message;

#define KBD_MESSAGE_QUEUE_SIZE 1024
KBD_message g_kbd_message_queue[KBD_MESSAGE_QUEUE_SIZE];
int g_kbd_message_queue_nitems = 0;
int g_kbd_message_queue_front = 0;
int g_kbd_message_queue_back  = 0;

KBD_message get_kbd_message(void)
{
	KBD_message result;
	
	int spin = 1;
	while (spin)
	{
		_disable();
		if (g_kbd_message_queue_nitems > 0)
		{
			spin = 0;
		}
		else
		{
			_enable();
		}
	}
	
	
	if (g_kbd_message_queue_nitems > 0)
	{
		result = g_kbd_message_queue[g_kbd_message_queue_front];
		
		g_kbd_message_queue_front++;
		if (g_kbd_message_queue_front == KBD_MESSAGE_QUEUE_SIZE)
		{
			g_kbd_message_queue_front = 0;
		}
		g_kbd_message_queue_nitems--;
	}
	
	_enable();
	
	return (result);
}

void put_kbd_message(KBD_message msg)
{
	_disable();

	g_kbd_message_queue[g_kbd_message_queue_back++] = msg;
	if (g_kbd_message_queue_back == KBD_MESSAGE_QUEUE_SIZE)
	{
		g_kbd_message_queue_back = 0;
	}

	g_kbd_message_queue_nitems++;
	if (g_kbd_message_queue_nitems > KBD_MESSAGE_QUEUE_SIZE)
	{
		g_kbd_message_queue_front = g_kbd_message_queue_back; // always start with the latest message if the queue is full
		g_kbd_message_queue_nitems = KBD_MESSAGE_QUEUE_SIZE;
	}
	
	_enable();
}

#define VK_BACK   0x08
#define VK_TAB    0x09
#define VK_RETURN 0x0D
#define VK_ESC    0x1B
#define VK_SPACE  0x20
#define VK_EQUALS '='
#define VK_PLUS   '+'
#define VK_MINUS  '-'
#define VK_UNDERSCORE '_'
#define VK_SEMMICOLON ';'
#define VK_COLON      ':'
#define VK_ASTERIC    '*'

#define MSG_KEYUP   0x01
#define MSG_KEYDOWN 0x02




enum Kbd_dirver_state
{
	KBD_DRIVER_STATE_DEFAULT,
	KBD_DRIVER_STATE_BREAK_1,
	KBD_DRIVER_STATE_BREAK_2,
};
typedef enum Kbd_dirver_state Kbd_dirver_state;
Kbd_dirver_state g_kbd_driver_state = KBD_DRIVER_STATE_DEFAULT;

int g_shift_down;
int g_ctrl_down;

#define NUMBERS_LIST(code) \
	code( 0x02, '1') \
	code( 0x03, '2') \
	code( 0x04, '3') \
	code( 0x05, '4') \
	code( 0x06, '5') \
	code( 0x07, '6') \
	code( 0x08, '7') \
	code( 0x09, '8') \
	code( 0x0a, '9') \
	code( 0x0b, '0')

#define FIRST_ROW_LETTERS_LIST(code) \
	code( 0x10, 'Q') \
	code( 0x11, 'W') \
	code( 0x12, 'E') \
	code( 0x13, 'R') \
	code( 0x14, 'T') \
	code( 0x15, 'Y') \
	code( 0x16, 'U') \
	code( 0x17, 'I') \
	code( 0x18, 'O') \
	code( 0x19, 'P')
	
#define SECOND_ROW_LETTERS_LIST(code) \
	code( 0x1e, 'A') \
	code( 0x1f, 'S') \
	code( 0x20, 'D') \
	code( 0x21, 'F') \
	code( 0x22, 'G') \
	code( 0x23, 'H') \
	code( 0x24, 'J') \
	code( 0x25, 'K') \
	code( 0x26, 'L')

#define THIRD_ROW_LETTERS_LIST(code) \
	code( 0x2c, 'Z') \
	code( 0x2d, 'X') \
	code( 0x2e, 'C') \
	code( 0x2f, 'V') \
	code( 0x30, 'B') \
	code( 0x31, 'N') \
	code( 0x32, 'M')

#define GENERATE_MESSAGE_FORMATION_CODE(__scane_code, __letter)                               \
						case (__scane_code):                                                  \
						case ((__scane_code) +  0x80): {                                      \
						KBD_message msg;                                                      \
						msg.msg = ((scan_code) & 0x80) ? MSG_KEYUP : MSG_KEYDOWN;             \
						msg.vk = (__letter);                                                  \
						msg.flags = (g_shift_down) ? (msg.flags | 0x01) : (msg.flags & 0xfe); \
						msg.flags = (g_ctrl_down)  ? (msg.flags | 0x02) : (msg.flags & 0xfd); \
						g_kbd_driver_state = KBD_DRIVER_STATE_DEFAULT;                        \
						put_kbd_message(msg); } break;

						



void interrupt keyboard_isr(void)
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
				NUMBERS_LIST(GENERATE_MESSAGE_FORMATION_CODE)
				FIRST_ROW_LETTERS_LIST(GENERATE_MESSAGE_FORMATION_CODE)
				SECOND_ROW_LETTERS_LIST(GENERATE_MESSAGE_FORMATION_CODE)
				THIRD_ROW_LETTERS_LIST(GENERATE_MESSAGE_FORMATION_CODE)
				
				GENERATE_MESSAGE_FORMATION_CODE(0x2B, '\\')
				GENERATE_MESSAGE_FORMATION_CODE(0x35, '/')
				GENERATE_MESSAGE_FORMATION_CODE(0x0C, '-')
				GENERATE_MESSAGE_FORMATION_CODE(0x0D, '=')
				GENERATE_MESSAGE_FORMATION_CODE(0x27, ';')
				GENERATE_MESSAGE_FORMATION_CODE(0x28, '\'')
				
				GENERATE_MESSAGE_FORMATION_CODE(0x0e, VK_BACK)
				GENERATE_MESSAGE_FORMATION_CODE(0x39, VK_SPACE)
				GENERATE_MESSAGE_FORMATION_CODE(0x1c, VK_RETURN)
				GENERATE_MESSAGE_FORMATION_CODE(0x01, VK_ESC)
				GENERATE_MESSAGE_FORMATION_CODE(0x0f, VK_TAB)
				
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
	
	{
		int index = g_kbd_message_queue_back - 1;
		if (index < 0)
			index = KBD_MESSAGE_QUEUE_SIZE - 1;
		
		switch (g_kbd_message_queue[index].vk)
		{
			case '9':
			{
				if (g_shift_down) g_kbd_message_queue[index].vk = '(';
			} break;
			case '0':
			{
				if (g_shift_down) g_kbd_message_queue[index].vk = ')';
			} break;
			case VK_MINUS:
			{
				if (g_shift_down) g_kbd_message_queue[index].vk = '_';
			} break;
			case VK_EQUALS:
			{
				if (g_shift_down) g_kbd_message_queue[index].vk = '+';
			} break;
			case '\'':
			{
				if (g_shift_down) g_kbd_message_queue[index].vk = '\"';
			} break;
			case ';':
			{
				if (g_shift_down) g_kbd_message_queue[index].vk = ':';
			} break;
			case '8':
			{
				if (g_shift_down) g_kbd_message_queue[index].vk = '*';
			} break;
		}
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



















