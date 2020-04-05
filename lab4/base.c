#include <stdio.h>
#include <dos.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long long uint32_t;

typedef char int8_t;
typedef short int16_t;
typedef long long int32_t;

#define array_cound(a) (sizeof(a) / sizeof((a)[0]))


#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xa0
#define PIC2_DATA    0xa1	

#define PIT_CH0_DATA_PORT 0x40
#define PIT_CH1_DATA_PORT 0x41
#define PIT_CH2_DATA_PORT 0x42
#define PIT_MODCMD_PORT   0x43

#define PIT_SEL_CH_0            (0 << 6)
#define PIT_SEL_CH_1            (1 << 6)
#define PIT_SEL_CH_2            (2 << 6)
#define PIT_ACCESS_MODE_1       (1 << 4)
#define PIT_ACCESS_MODE_2       (2 << 4)
#define PIT_ACCESS_MODE_3       (3 << 4)
#define PIT_OP_MODE_0           (0 << 1)
#define PIT_OP_MODE_1           (1 << 1) 
#define PIT_OP_MODE_2           (2 << 1)
#define PIT_OP_MODE_3           (3 << 1)
#define PIT_OP_MODE_4           (4 << 1)
#define PIT_OP_MODE_5           (5 << 1)
#define PIT_BIN_MODE            (0 << 0)

#define PIT_LATCH_COUNT_VAL_CMD (0 << 4)
#define PIT_READ_BACK_CMD       (3 << 6)
#define PIT_READ_BACK_DONT_LATCH_COUNT    (1 << 5)
#define PIT_READ_BACK_DONT_LATCH_STATUS   (1 << 4)
#define PIT_READ_BACK_CH_0      (1 << 1)
#define PIT_READ_BACK_CH_1      (1 << 2)
#define PIT_READ_BACK_CH_2      (1 << 3)

#define PC_SPEAKER_PORT 0x61

#define NOTE_SILENCE 0
#define NOTE_C 262
#define NOTE_D 294
#define NOTE_E 330
#define NOTE_F 349
#define NOTE_G 392
#define NOTE_A 440
#define NOTE_B 493

#define OSCILLATOR_FREQUENCY_HZ 1193182

#define PIT_COUNTER_MAX_VAL 65535
#define COUNT_TILL_NEW_RANDOM_NUM 5
uint16_t random16(void)
{
	static uint16_t get_new_counter = 0;
	static uint16_t seed = 0;
	
	if (get_new_counter == 0)
	{
		_disable();
		outp(PIT_MODCMD_PORT, PIT_SEL_CH_0 | PIT_LATCH_COUNT_VAL_CMD);
		seed = inp(PIT_CH0_DATA_PORT);
		seed |= (inp(PIT_CH0_DATA_PORT) << 8);
		_enable();
		
		get_new_counter = COUNT_TILL_NEW_RANDOM_NUM;
	}
	get_new_counter--;
	
	// xorshift
	seed ^= seed << 7;
	seed ^= seed >> 9;
	seed ^= seed << 8;
	
	return (seed);
}
	
struct NoteDescription
{
	int16_t note_hz;
	int16_t duration_ms;
} g_melody[] = {
	
	{ NOTE_G, 400 },
	{ NOTE_SILENCE, 100},
	{ NOTE_G, 400 },
	{ NOTE_SILENCE, 100},
	{ NOTE_G, 400 },
	{ NOTE_SILENCE, 100},
	{ NOTE_G, 1000 },
	{ NOTE_SILENCE, 100},
	{ NOTE_F, 400 },
	{ NOTE_SILENCE, 100},
	{ NOTE_F, 400 },
	{ NOTE_SILENCE, 100},
	{ NOTE_F, 400 },
	{ NOTE_SILENCE, 100},
	{ NOTE_D, 800 },
	{ NOTE_SILENCE, 100},
	{ NOTE_D, 1000 },
	
};

#define BYTE_TO_BINARY_PATTERN "%c%c %c%c %c%c%c %c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 
  

int main(void)
{
	int i = 0;
	
	outp(PIT_MODCMD_PORT, PIT_SEL_CH_2 | PIT_ACCESS_MODE_3 | PIT_OP_MODE_3 | PIT_BIN_MODE);
	for (i = 0; i < array_cound(g_melody); i++)
	{
		int16_t divisor = 0;
	
		outp(PC_SPEAKER_PORT, inp(PC_SPEAKER_PORT) & 0xfc);
	
		if (g_melody[i].note_hz != NOTE_SILENCE)
		{
			divisor = (OSCILLATOR_FREQUENCY_HZ / g_melody[i].note_hz);
		
			_disable();
			outp(PIT_CH2_DATA_PORT, (uint8_t)divisor);
			outp(PIT_CH2_DATA_PORT, (uint8_t)(divisor >> 8));
			_enable();
			
			outp(PC_SPEAKER_PORT, inp(PC_SPEAKER_PORT) | 0x3);
		}
		
		delay(g_melody[i].duration_ms);
	}
	
	// NOTE(max): 0b 1111 1101, we don't disable counting in channel 2, but we are only turning off the speaker
	//                          so that we can approximate reload counter in channel 2 later
	outp(PC_SPEAKER_PORT, inp(PC_SPEAKER_PORT) & 0xfd);
	
	{
		uint8_t status_ch_0 = 0;
		uint8_t status_ch_2 = 0;
		
		for (i = 0; i < 3; i++)
		{
			_disable();
			
			outp(PIT_MODCMD_PORT, PIT_READ_BACK_CMD | PIT_READ_BACK_CH_0 | PIT_READ_BACK_DONT_LATCH_COUNT);
			status_ch_0 = inp(PIT_CH0_DATA_PORT);
			
			outp(PIT_MODCMD_PORT, PIT_READ_BACK_CMD | PIT_READ_BACK_CH_2 | PIT_READ_BACK_DONT_LATCH_COUNT);
			status_ch_2 = inp(PIT_CH2_DATA_PORT);
			
			_enable();
			
			printf("\rch_0 status: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(status_ch_0));
			printf("\rch_2 status: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(status_ch_2));
			
			delay(1000);
		}
	}	
	
	
	{
		uint16_t max_reload_counter_ch0 = 0;
		uint16_t max_reload_counter_ch2 = 0;
		
		for (i = 0; i < 100; i++)
		{
			uint16_t temp_ch0 = 0;
			uint16_t temp_ch2 = 0;
			
			_disable();
			
			outp(PIT_MODCMD_PORT, PIT_SEL_CH_0 | PIT_LATCH_COUNT_VAL_CMD);
			temp_ch0 = inp(PIT_CH0_DATA_PORT);
			temp_ch0 |= (inp(PIT_CH0_DATA_PORT) << 8);
			
			outp(PIT_MODCMD_PORT, PIT_SEL_CH_2 | PIT_LATCH_COUNT_VAL_CMD);
			temp_ch2 = inp(PIT_CH2_DATA_PORT);
			temp_ch2 |= (inp(PIT_CH2_DATA_PORT) << 8);
			
			_enable();
			
			if (temp_ch0 > max_reload_counter_ch0)
			{
				max_reload_counter_ch0 = temp_ch0;
			}
			if (temp_ch2 > max_reload_counter_ch2)
			{
				max_reload_counter_ch2 = temp_ch2;
			}
			
			delay(10);
		}
		
		printf("approximate ch0 reload counter: %u\n", max_reload_counter_ch0);
		printf("approximate ch2 reload counter: %u\n", max_reload_counter_ch2);
	}
	
	
	return (0);
}



















