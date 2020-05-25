
#include <stdio.h>
#include <string.h>
#include <dos.h>

typedef unsigned int uint16_t;
typedef unsigned char uint8_t;

#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 25

enum TEXT_COLORS
{
	BLACK = 0,
	BLUE = 1,
	GREEN = 2,
	RED = 4,
	GREY = 7,
};

uint16_t far *g_console_memory = MK_FP(0xb800, 0);
int g_text_attr = 0x0f;
int g_cursor_x = 0;
int g_cursor_y = 0;

void console_set_text_color(uint8_t fg, uint8_t bg)
{
	g_text_attr = (bg << 4) | (fg & 0x0F);
}

void console_move_cursor(int x, int y)
{
	uint16_t temp;
	
	g_cursor_x = x;
	g_cursor_y = y;
	
	temp = y * CONSOLE_WIDTH + x;
	
	// TODO(max): verify that cursor still blinks
	outportb(0x3D4, 14);
    outportb(0x3D5, temp >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, temp);
}

void console_cls(void)
{
	uint16_t blank;
	int i;
	
	blank = ' ' | (g_text_attr << 8);
	
	for (i = 0; i < CONSOLE_HEIGHT * CONSOLE_WIDTH; i++)
	{
		g_console_memory[i] = blank;
	}
	
	console_move_cursor(0, 0);
}

void console_scroll(void)
{
	
	if (g_cursor_y >= CONSOLE_HEIGHT)
	{
		int i;
		uint16_t blank = ' ' | (g_text_attr << 8);
		
		uint16_t chars_to_copy = ((CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH);
		uint16_t far *dest = g_console_memory;
		uint16_t far *src = g_console_memory + CONSOLE_WIDTH;
		
		for (i = 0; i < chars_to_copy; i++)
		{
			dest[i] = src[i];
		}
		
		for (i = 0; i < CONSOLE_WIDTH; i++)
		{
			g_console_memory[(CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH + i] = blank;
		}
		
		console_move_cursor(g_cursor_x, CONSOLE_HEIGHT - 1);
	}
}

void console_place_byte(uint16_t c)
{
	g_console_memory[g_cursor_y * CONSOLE_WIDTH + g_cursor_x] = c | (g_text_attr << 8);
}

void put_char(uint8_t c)
{
	
	if (c == '\r')
	{
		g_cursor_x = 0;
	}
	else if (c == '\t')
	{
		g_cursor_x = (g_cursor_x + 8) & ~(8 - 1);
	}
	else if (c == '\n')
	{
		g_cursor_y++;
		g_cursor_x = 0;
	}
	else if (c == '\b')
	{
		if (g_cursor_x > 0)
		{
			g_cursor_x--;
		}
	}
	else if (c >= ' ') // isprint(c)
	{
		g_console_memory[g_cursor_y * CONSOLE_WIDTH + g_cursor_x] = c | (g_text_attr << 8);
		g_cursor_x++;
	}
	
	if (g_cursor_x >= CONSOLE_WIDTH)
	{
		g_cursor_x = 0;
		g_cursor_y++;
	}
	
	console_scroll();
	console_move_cursor(g_cursor_x, g_cursor_y);
}

void put_str(const char *s)
{
	int i;
	for (i = 0; i < strlen(s); i++)
	{
		put_char(s[i]);
	}
}



#define MASTER_OFFSET 0x8
#define SLAVE_OFFSET 0x70

#define PIC1_COMMAND 0x20
#define PIC1_DATA (PIC1_COMMAND + 1)
#define PIC2_COMMAND 0xa0
#define PIC2_DATA (PIC2_COMMAND + 1)
#define PIC_EOI		0x20

#define COM1_IRQ 4
#define COM2_IRQ 3
 
void pic_send_eoi(uint8_t irq)
{
	if(irq >= 8)
		outportb(PIC2_COMMAND,PIC_EOI);
 
	outportb(PIC1_COMMAND,PIC_EOI);
}

void irq_clear_mask(unsigned char irq) {
    uint16_t port;
    uint8_t value;
 
    if(irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inp(port) & ~(1 << irq);
    outp(port, value);        
}

#define COM1 0x3f8
#define COM2 0x2f8

#define LO_OFFS 0
#define HI_OFFS 1

#define IER_OFFS 1
#define IIR_OFFS 2
#define LCR_OFFS 3
#define MCR_OFFS 4
#define LSR_OFFS 5


uint8_t serial_received(uint16_t com_port) {
   return inp(com_port + LSR_OFFS) & 1;
}
 
uint8_t read_serial(uint16_t com_port) {
   while (serial_received(com_port) == 0);
 
   return inp(com_port);
}

uint8_t is_transmit_empty(uint16_t com_port) {
   return inp(com_port + LSR_OFFS) & 0x20;
}
 
void write_serial(uint8_t c, uint16_t com_port) {
   while (is_transmit_empty(com_port) == 0);
 
   outp(com_port, c);
}

void init_serial(uint16_t com_port)
{	
	outp(com_port + IER_OFFS, 0x01); // TODO(max): set interrupst
	
	outp(com_port + LCR_OFFS, 0x80); // set DLAB
	outp(com_port + LO_OFFS, 0x60);
	outp(com_port + HI_OFFS, 0x00);
	
	outp(com_port + LCR_OFFS, 0x03); // 8 bits, no parity, one stop bit
	
	outp(com_port + IIR_OFFS, 0x04); // 0b00000100
	outp(com_port + MCR_OFFS, 0x0b);
}

int my_turn = 1;

void interrupt serial_isr(void)
{
	uint8_t c;
	
	pic_send_eoi(COM2_IRQ);
	
	c = read_serial(COM2);
	put_char(c);
	if (c == '\n')
	{
		my_turn = 1;
	}
}

typedef void interrupt (*Int_description)(void);

int main(int argc, char *argv[])
{
	console_move_cursor(0, 0);
	console_cls();
	
	_disable();
	setvect(MASTER_OFFSET + COM2_IRQ, serial_isr);
	init_serial(COM2);
	irq_clear_mask(COM2_IRQ);
	_enable();
	
	for (;;)
	{
		if (my_turn)
		{
			char *c;
			char buf[512];
			gets(buf);
			buf[511] = 0;
			
			c = buf;
			while (*c)
			{
				write_serial(*c, COM2);
				c++;
			}
			write_serial('\n', COM2);
			
			my_turn = 0;
		}
	}
	
	
	return (0);
}