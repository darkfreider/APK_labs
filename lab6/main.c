



#include <assert.h>
#include <dos.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef char  int8_t;
typedef short int16_t;

#include "console.c"
#include "kbd.c"
#include "io.c"


typedef void interrupt (*Interrupt_handler_type)(void);
Interrupt_handler_type old_kbd_handler;

int get_line(char far *str, int len)
{
	int c;
	int i = 0;
	
	while ((c = get_char()) != EOF && c != '\n' && i < len)
	{
		str[i++] = c;
	}
	str[i] = '\0';
	
	return (i);
}


int main(void)
{
	int running = 1;
	char str_buf[512];
	
	old_kbd_handler = getvect(PIC_MASTER_OFFSET + 1);
	setvect(PIC_MASTER_OFFSET + 1, keyboard_isr);
	
	console_set_text_color(BLACK, GREY);
	console_cls();
	

	while (running)
	{
		put_str("$: ");
		
		if (get_line(str_buf, 512) == 0)
			break;
		else if (strcmp(str_buf, "exit") == 0)
			running = 0;
		else if (strcmp(str_buf, "fuck") == 0)
			put_str("Don't use bad words\n");
	}
	
	setvect(PIC_MASTER_OFFSET + 1, old_kbd_handler);
	
	return (0);
}


















