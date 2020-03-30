#include <stdio.h>
#include <dos.h>



#define PIC1_COMMAND 0x20
#define PIC1_DATA (PIC1_COMMAND + 1)
#define PIC2_COMMAND 0xa0
#define PIC2_DATA (PIC2_COMMAND + 1)
#define PIC_EOI 0x20

void pic_send_eoi(unsigned char irq)
{
	if (irq >= 8)
	{
		outp(PIC2_COMMAND, PIC_EOI);
	}
	outp(PIC1_COMMAND, PIC_EOI);
}

#define OLD_MASTER_OFFSET 0x8
#define OLD_SLAVE_OFFSET 0x70

#define NEW_MASTER_OFFSET 0
#define NEW_SLAVE_OFFSET 0

#define PIT_IRQ 0
#define KBD_IRQ 1
#define SLAVE_CNTRLR_IRQ 2
#define COM2_IRQ 3
#define COM1_IRQ 4
#define LPT2_IRQ 5
#define FLOPPY_IRQ 6
#define LPT1_IRQ 7

#define RTC_IRQ 8
#define UNASSIGNED_0_IRQ 9
#define UNASSIGNED_1_IRQ 10
#define UNASSIGNED_2_IRQ 11
#define MOUSE_IRQ 12
#define MATH_COPROCESSOR_IRQ 13
#define HARD_DISK_CNTRL_1_IRQ 14
#define HARD_DISK_CNTRL_2_IRQ 15

struct Int_description
{
	int int_num;
	void interrupt (*int_ptr)(void);
};
typedef struct Int_description Int_description;

Int_description old_int_handlers[] = {
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
};

struct VIDEO
{
	unsigned char symbol;
	unsigned char attribute;
};
typedef struct VIDEO VIDEO;

#define COLOR_TABLE_SIZE 6
char color_table[] = 
{
	0x5e,
	0x5e,
	0x5a,
	0x5a,
	0x58,
	0x58
};
int color_index = 0;

void interrupt new_kbd_int(void)
{
	VIDEO far *screen = (VIDEO far *)MK_FP(0xb800, 0);
	
	char *msg = "Keyboard interrupt";
	char *c = msg;
	int n = 0;
	
	while (*c != 0)
	{
		screen[n].symbol = *c++;
		screen[n].attribute = color_table[color_index % COLOR_TABLE_SIZE];
		n++;
	}
	color_index++;
	
	old_int_handlers[KBD_IRQ].int_ptr();
}

void interrupt new_mouse_int(void)
{
	VIDEO far *screen = (VIDEO far *)MK_FP(0xb800, 0);
	
	char *msg = "Mouse Interrupt";
	char *c = msg;
	int n = 0;
	
	while (*c != 0)
	{
		screen[n].symbol = *c++;
		screen[n].attribute = color_table[color_index % COLOR_TABLE_SIZE];
		n++;
	}
	color_index++;
	
	old_int_handlers[MOUSE_IRQ].int_ptr();
}

void interrupt new_generic_int(void)
{
	old_int_handlers[irq].int_ptr();
}

Int_description new_int_handlers[] = {
	{ NEW_MASTER_OFFSET + 0, 0 },
	{ NEW_MASTER_OFFSET + 1, new_kbd_int },
	{ NEW_MASTER_OFFSET + 2, 0 },
	{ NEW_MASTER_OFFSET + 3, 0 },
	{ NEW_MASTER_OFFSET + 4, 0 },
	{ NEW_MASTER_OFFSET + 5, 0 },
	{ NEW_MASTER_OFFSET + 6, 0 },
	{ NEW_MASTER_OFFSET + 7, 0 },
	
	{ NEW_SLAVE_OFFSET + 0, 0 },
	{ NEW_SLAVE_OFFSET + 1, 0 },
	{ NEW_SLAVE_OFFSET + 2, 0 },
	{ NEW_SLAVE_OFFSET + 3, 0 },
	{ NEW_SLAVE_OFFSET + 4, new_mouse_int },
	{ NEW_SLAVE_OFFSET + 5, 0 },
	{ NEW_SLAVE_OFFSET + 6, 0 },
	{ NEW_SLAVE_OFFSET + 7, 0 },
}


void init(void)
{
	int i = 0;
	
	for (i = 0; i < 8; i++)
	{
		old_int_handlers[i].int_num = OLD_MASTER_OFFSET + i;
		old_int_handlers[i].int_ptr = getvect(OLD_MASTER_OFFSET + i);
	}
	
	for (i = 0; i < 8; i++)
	{
		old_int_handlers[i + 8].int_num = OLD_SLAVE_OFFSET + i;
		old_int_handlers[i + 8].int_ptr = getvect(OLD_SLAVE_OFFSET + i);
	}
	
	
	
	setvect(OLD_MASTER_OFFSET + KBD_IRQ, new_kbd_int);
	setvect(OLD_SLAVE_OFFSET + MOUSE_IRQ - 8, new_mouse_int);
}

int main(void)
{	
	unsigned far *fp = (unsigned far *)MK_FP(_psp, 0x2c);
	
	_disable();
	init();
	_enable();
	
	_dos_freemem(*fp);
	_dos_keep(0, (_DS - _CS) + (_SP / 16) + 1);
	return (0);
}

















