


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
	else if (c == '\n')
	{
		g_cursor_y++;
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