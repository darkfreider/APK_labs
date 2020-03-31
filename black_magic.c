#include <stdio.h>
#include <assert.h>
#include <dos.h>

#define NUMBER_OF_INTERRUPT_HANDLERS 16

#define OLD_MASTER_OFFSET 0x8
#define OLD_SLAVE_OFFSET 0x70

#define NEW_MASTER_OFFSET 0xa8
#define NEW_SLAVE_OFFSET OLD_SLAVE_OFFSET


#define PIC1_COMMAND 0x20
#define PIC1_DATA (PIC1_COMMAND + 1)
#define PIC2_COMMAND 0xa0
#define PIC2_DATA (PIC2_COMMAND + 1)

#define PIC_READ_IRR 0x0a
#define PIC_READ_ISR 0x0b

#define IRQ_LIST(code)          \
	code(PIT_IRQ              ) \
    code(KBD_IRQ              ) \
    code(SLAVE_CNTRLR_IRQ     ) \
    code(COM2_IRQ             ) \
    code(COM1_IRQ             ) \
    code(LPT2_IRQ             ) \
    code(FLOPPY_IRQ           ) \
    code(LPT1_IRQ             ) \
	code(RTC_IRQ              ) \
	code(UNASSIGNED_0_IRQ     ) \
	code(UNASSIGNED_1_IRQ     ) \
	code(UNASSIGNED_2_IRQ     ) \
	code(MOUSE_IRQ            ) \
	code(MATH_COPROCESSOR_IRQ ) \
	code(HARD_DISK_CNTRL_1_IRQ) \
	code(HARD_DISK_CNTRL_2_IRQ)
	

#define GENERATE_ENUM_TYPE(type) enum_##type,
enum Irq_enum_type
{
	IRQ_LIST(GENERATE_ENUM_TYPE)
};
typedef enum Irq_enum_type Irq_enum_type; 
#undef GENERATE_ENUM_TYPE
	

typedef void interrupt (*Int_description)(void);

Int_description old_int_handlers[NUMBER_OF_INTERRUPT_HANDLERS] = {0};

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
	print_pic_regs_stat(color_table[color_index % COLOR_TABLE_SIZE]);
	color_index++;
	
	old_int_handlers[enum_KBD_IRQ]();
}

#define GENERATE_GENERIC_INT_HANDLER(irq_name) \
	void interrupt new_generic_##irq_name##_int(void) { \
		print_pic_regs_stat(color_table[color_index % COLOR_TABLE_SIZE]); \
		old_int_handlers[enum_##irq_name](); }

IRQ_LIST(GENERATE_GENERIC_INT_HANDLER)
#undef GENERATE_GENERIC_INT_HANDLER

#define GENERATE_GENERIC_INT_HANDLER_NAME(name) new_generic_##name##_int,

Int_description new_int_handlers[NUMBER_OF_INTERRUPT_HANDLERS] = {
		IRQ_LIST(GENERATE_GENERIC_INT_HANDLER_NAME)
};
#undef GENERATE_GENERIC_INT_HANDLER_NAME


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
	outp(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	outp(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	
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
	
	// NOTE(max): save old interrupt handlers into a table
	for (i = 0; i < 8; i++)
	{
		old_int_handlers[i] = getvect(OLD_MASTER_OFFSET + i);
	}
	for (i = 0; i < 8; i++)
	{
		old_int_handlers[i + 8] = getvect(OLD_SLAVE_OFFSET + i);
	}
	
	// NOTE(max): install your custom hardware interrupt handler
	new_int_handlers[enum_KBD_IRQ] = new_kbd_int;
	
	for (i = 0; i < 8; i++)
	{
		setvect(NEW_MASTER_OFFSET + i, new_int_handlers[i]);
	}
	for (i = 0; i < 8; i++)
	{
		setvect(NEW_SLAVE_OFFSET + i, new_int_handlers[i + 8]);
	}
	
	pic_remap(NEW_MASTER_OFFSET, OLD_SLAVE_OFFSET);
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





