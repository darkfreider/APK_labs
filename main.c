#include <stdio.h>
#include <assert.h>
#include <dos.h>



#define PIC1_COMMAND 0x20
#define PIC1_DATA (PIC1_COMMAND + 1)
#define PIC2_COMMAND 0xa0
#define PIC2_DATA (PIC2_COMMAND + 1)

#define PIC_READ_IRR 0x0a
#define PIC_READ_ISR 0x0b

#define OLD_MASTER_OFFSET 0x8
#define OLD_SLAVE_OFFSET 0x70

// TODO(max): redefine them according to your assignment variant
// 0x8 -> A8h for master
// 0x70 -> ? for slave, I think i need to pick random

#define NEW_MASTER_OFFSET 0xa8
#define NEW_SLAVE_OFFSET OLD_SLAVE_OFFSET

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

Int_description old_int_handlers[] = {0};

struct VIDEO
{
	unsigned char symbol;
	unsigned char attribute;
};
typedef struct VIDEO VIDEO;

#define COLOR_TABLE_SIZE 6
unsigned char color_table[] = 
{
	0x5e,
	0x3a,
	0x5a,
	0x3f,
	0x97,
	0x58,
};
int color_index = 0;

void print_binary_byte(int n, int pos, unsigned char attribute)
{
	int i = 0;
	VIDEO far *screen = (VIDEO far *)MK_FP(0xb800, 0);
	
	for (i = 7; i >= 0; i--)
	{
		screen[pos + i].symbol = (n % 2) + '0';
		screen[pos + i].attribute = attribute;
		n /= 2;
	}
}

void print_pic_regs_stat(unsigned char attribute)
{	
	// NOTE(max): printing mask registers
	print_binary_byte(inp(PIC1_DATA), 0, attribute);
	print_binary_byte(inp(PIC2_DATA), 9, attribute);
	
	// NOTE(max): printing IRRs
	outp(PIC1_COMMAND, PIC_READ_IRR);
	print_binary_byte(inp(PIC1_COMMAND), 80, attribute);
	
	outp(PIC2_COMMAND, PIC_READ_IRR);
	print_binary_byte(inp(PIC2_COMMAND), 89, attribute);
	
	// NOTE(max): printing ISRs
	outp(PIC1_COMMAND, PIC_READ_ISR);
	print_binary_byte(inp(PIC1_COMMAND), 160, attribute);
	
	outp(PIC2_COMMAND, PIC_READ_ISR);
	print_binary_byte(inp(PIC2_COMMAND), 169, attribute);
}

void interrupt new_kbd_int(void)
{
	unsigned char attribute = color_table[color_index % COLOR_TABLE_SIZE];
	print_pic_regs_stat(attribute);
	color_index++;
	
	old_int_handlers[KBD_IRQ].int_ptr();
}

#define GENERATE_GENERIC_INT_HANDLER_NAME(N) new_generic_ ## N ## _int

#define GENERATE_GENERIC_INT_HANDLER(N) \
	void interrupt new_generic_ ## N ## _int(void) { \
		print_pic_regs_stat(color_table[color_index % COLOR_TABLE_SIZE]); \
		old_int_handlers[N].int_ptr(); \
	}

GENERATE_GENERIC_INT_HANDLER(0)
GENERATE_GENERIC_INT_HANDLER(1)
GENERATE_GENERIC_INT_HANDLER(2)
GENERATE_GENERIC_INT_HANDLER(3)
GENERATE_GENERIC_INT_HANDLER(4)
GENERATE_GENERIC_INT_HANDLER(5)
GENERATE_GENERIC_INT_HANDLER(6)
GENERATE_GENERIC_INT_HANDLER(7)

GENERATE_GENERIC_INT_HANDLER(8)
GENERATE_GENERIC_INT_HANDLER(9)
GENERATE_GENERIC_INT_HANDLER(10)
GENERATE_GENERIC_INT_HANDLER(11)
GENERATE_GENERIC_INT_HANDLER(12)
GENERATE_GENERIC_INT_HANDLER(13)
GENERATE_GENERIC_INT_HANDLER(14)
GENERATE_GENERIC_INT_HANDLER(15)

Int_description new_int_handlers[] = {
	{ NEW_MASTER_OFFSET + 0, GENERATE_GENERIC_INT_HANDLER_NAME(0) },
	{ NEW_MASTER_OFFSET + 1, new_kbd_int },
	{ NEW_MASTER_OFFSET + 2, GENERATE_GENERIC_INT_HANDLER_NAME(2) },
	{ NEW_MASTER_OFFSET + 3, GENERATE_GENERIC_INT_HANDLER_NAME(3) },
	{ NEW_MASTER_OFFSET + 4, GENERATE_GENERIC_INT_HANDLER_NAME(4) },
	{ NEW_MASTER_OFFSET + 5, GENERATE_GENERIC_INT_HANDLER_NAME(5) },
	{ NEW_MASTER_OFFSET + 6, GENERATE_GENERIC_INT_HANDLER_NAME(6) },
	{ NEW_MASTER_OFFSET + 7, GENERATE_GENERIC_INT_HANDLER_NAME(7) },
	
	{ NEW_SLAVE_OFFSET + 0, GENERATE_GENERIC_INT_HANDLER_NAME(8) },
	{ NEW_SLAVE_OFFSET + 1, GENERATE_GENERIC_INT_HANDLER_NAME(9) },
	{ NEW_SLAVE_OFFSET + 2, GENERATE_GENERIC_INT_HANDLER_NAME(10) },
	{ NEW_SLAVE_OFFSET + 3, GENERATE_GENERIC_INT_HANDLER_NAME(11) },
	{ NEW_SLAVE_OFFSET + 4, GENERATE_GENERIC_INT_HANDLER_NAME(12) },
	{ NEW_SLAVE_OFFSET + 5, GENERATE_GENERIC_INT_HANDLER_NAME(13) },
	{ NEW_SLAVE_OFFSET + 6, GENERATE_GENERIC_INT_HANDLER_NAME(14) },
	{ NEW_SLAVE_OFFSET + 7, GENERATE_GENERIC_INT_HANDLER_NAME(15) },
};


#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW1_INTERVAL4 0x4
#define ICW4_8086 0x1

void pic_remap(int master_offset, int slave_offset)
{
	unsigned char master_mask;
	unsigned char slave_mask;

	assert(master_offset % 8 == 0);
	assert(slave_offset % 8 == 0);
	
	master_mask = inp(PIC1_DATA);
	slave_mask = inp(PIC2_DATA);
	
	// NOTE(max): ICW1
	outp(PIC1_COMMAND, ICW1_INIT);
	outp(PIC2_COMMAND, ICW1_INIT);
	
	// NOTE(max): ICW2
	outp(PIC1_DATA, master_offset);
	outp(PIC2_DATA, slave_offset);
	
	// NOTE(max): ICW3
	outp(PIC1_DATA, 0x4);
	outp(PIC2_DATA, 0x2);
	
	// NOTE(max): ICW4
	outp(PIC1_DATA, ICW4_8086);
	outp(PIC2_DATA, ICW4_8086);
	
	// NOTE(max): restore interrupt mask
	outp(PIC1_DATA, master_mask);
	outp(PIC2_DATA, slave_mask);
}

void init(void)
{
	int i = 0;
	
	_disable();
	
	
	// NOTE(max): save old interrupt handlers into a table
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
	
	for (i = 0; i < 8; i++)
	{
		setvect(NEW_MASTER_OFFSET + i, new_int_handlers[i].int_ptr);
	}
	for (i = 0; i < 8; i++)
	{
		setvect(NEW_SLAVE_OFFSET + i, new_int_handlers[i + 8].int_ptr);
	}
	
	pic_remap(NEW_MASTER_OFFSET, OLD_SLAVE_OFFSET);
	
	_enable();
}

int main(void)
{	
	unsigned far *fp = (unsigned far *)MK_FP(_psp, 0x2c);
	
	init();
	
	_dos_freemem(*fp);
	_dos_keep(0, (_DS - _CS) + (_SP / 16) + 1);
	return (0);
}

















