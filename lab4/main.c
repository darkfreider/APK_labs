
#include <stdio.h>
#include <dos.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long long uint32_t;

typedef char int8_t;
typedef short int16_t;
typedef long long int32_t;

enum PIC_definitions
{
	PIC1_COMMAND = 0x20,
	PIC1_DATA = 0x21,
	PIC2_COMMAND = 0xa0,
	PIC2_DATA = 0xa1,
	
	PIC_EOI = 0x20
};

void PIC_send_eoi(unsigned char irq)
{
	if (irq >= 8)
	{
		outp(PIC2_COMMAND, PIC_EOI);
	}
	outp(PIC1_COMMAND, PIC_EOI);
}


enum PIT_ports
{
	PIT_CH0_DATA = 0x40,
	PIT_CH1_DATA = 0x41,
	PIT_CH2_DATA = 0x42,
	PIT_MODCMD  = 0x43,
};

enum PIT_CMDMOD_OPTIONS
{
	PIT_SEL_CH_0 = (0 << 6),
	PIT_SEL_CH_1 = (1 << 6),
	PIT_SEL_CH_2 = (2 << 6),
	PIT_READ_BACK_CMD = (3 << 6),

	PIT_LATCH_COUNT_VAL_CMD = (0 << 4),
	PIT_ACCESS_MODE_1 = (1 << 4),
	PIT_ACCESS_MODE_2 = (2 << 4),
	PIT_ACCESS_MODE_3 = (3 << 4),
	
	PIT_OP_MODE_0 = (0 << 1),
	PIT_OP_MODE_1 = (1 << 1), 
	PIT_OP_MODE_2 = (2 << 1),
	PIT_OP_MODE_3 = (3 << 1),
	PIT_OP_MODE_4 = (4 << 1),
	PIT_OP_MODE_5 = (5 << 1),
	
	PIT_BIN_MODE = (0 << 0),
};

enum PS2_kbd_ports
{
	PS2_KBD_DATA   = 0x60,
	PS2_KBD_STATUS = 0x64,
	PS2_KBD_CMD    = 0x64,
};

enum Notes
{
	NOTE_C = 262,
	NOTE_D = 294,
	NOTE_E = 330,
	NOTE_F = 349,
	NOTE_G = 392,
	NOTE_A = 440,
	NOTE_B = 493,
};

#define PC_SPEAKER_PORT 0x61
#define OSCILLATOR_FREQUENCY_HZ 1193182


#define PIT_COUNTER_MAX_VAL 65535
#define COUNT_TILL_NEW_RANDOM_NUM 5
uint16_t random16(void)
{
	static uint16_t get_new_counter = 0;
	static uint16_t seed = 0;
	
	if (get_new_counter == 0)
	{
		outp(PIT_MODCMD, PIT_SEL_CH_0 | PIT_LATCH_COUNT_VAL_CMD);
		seed = inp(PIT_CH0_DATA);
		seed |= (inp(PIT_CH0_DATA) << 8);
		
		get_new_counter = COUNT_TILL_NEW_RANDOM_NUM;
	}
	get_new_counter--;
	
	// xorshift
	seed ^= seed << 7;
	seed ^= seed >> 9;
	seed ^= seed << 8;
	
	return (seed);
}

void (interrupt far * volatile old_irq0)(void);

struct NoteDescription
{
	int16_t note;
	int16_t duration_ms;
} g_melody[] = {
	{ NOTE_C, 780 },
	{ NOTE_A, 780 },
	{ NOTE_F, 780 },
	{ NOTE_E, 780 },
};

#define array_count(a) (sizeof(a) / sizeof((a)[0]))

static uint32_t display_count = 0;
static float irq_ms_delay = 1000.0f * (1193.0f / 1193182.0f);
static int16_t count_down_ms = 0;
static uint16_t curr_note_index = 0;
	
void interrupt new_irq0(void)
{	
	display_count++;
	if (count_down_ms < 0)
	{
		if (curr_note_index >= array_count(g_melody))
		{
			curr_note_index = 0;
		}
		
		count_down_ms = g_melody[curr_note_index].duration_ms;
		
		{
			int16_t divisor = 0;
				
			outp(PC_SPEAKER_PORT, inp(PC_SPEAKER_PORT) & 0xfc);
				
			divisor = (OSCILLATOR_FREQUENCY_HZ / g_melody[curr_note_index].note);
				
			outp(PIT_MODCMD, PIT_SEL_CH_2 | PIT_ACCESS_MODE_3 | PIT_OP_MODE_3 | PIT_BIN_MODE);
			outp(PIT_CH2_DATA, (uint8_t)divisor);
			outp(PIT_CH2_DATA, (uint8_t)(divisor >> 8));
				
			outp(PC_SPEAKER_PORT, inp(PC_SPEAKER_PORT) | 0x3);
		}
		
		curr_note_index++;
	}
	
	count_down_ms -= 1;
	
	old_irq0();
}

#define MASTER_PIC_OFFSET 0x8

int main(void)
{
	unsigned far *fp = (unsigned far *)MK_FP(_psp, 0x2c);
	uint16_t pit_reload_value = 1193;
	
	_disable();
	
	outp(PIT_MODCMD, PIT_SEL_CH_0 | PIT_ACCESS_MODE_3 | PIT_OP_MODE_2 | PIT_BIN_MODE);
	outp(PIT_CH0_DATA, (uint8_t)pit_reload_value);
	outp(PIT_CH0_DATA, (uint8_t)(pit_reload_value >> 8));

	//outp(PIT_MODCMD, PIT_SEL_CH_2 | PIT_ACCESS_MODE_3 | PIT_OP_MODE_3 | PIT_BIN_MODE);
	//outp(PIT_CH2_DATA, (uint8_t)2);
	//outp(PIT_CH2_DATA, (uint8_t)0);
			
			
	old_irq0 = getvect(MASTER_PIC_OFFSET + 0);
	setvect(MASTER_PIC_OFFSET + 0, new_irq0);
	
	_enable();
	
	_dos_freemem(*fp);
	_dos_keep(0, (_DS + 1024 - _CS) + (_SP / 16) + 1);
	
	
	return (0);
}












