
#define EOF (-1)

#define INPUT_BUFFER_SIZE 1024
char g_input_buffer[INPUT_BUFFER_SIZE];
int g_input_buffer_front = 0;
int g_input_buffer_back = 0;
int g_input_buffer_nitems = 0;

void add_input_buffer_char(char c)
{
	g_input_buffer[g_input_buffer_back++] = c;
	if (g_input_buffer_back == INPUT_BUFFER_SIZE)
	{
		g_input_buffer_back = 0;
	}
	g_input_buffer_nitems++;
	if (g_input_buffer_nitems > INPUT_BUFFER_SIZE)
	{
		g_input_buffer_front = g_input_buffer_back;
		g_input_buffer_nitems = INPUT_BUFFER_SIZE;
	}
}

char remove_input_buffer_char(void)
{
	char result;
	
	if (g_input_buffer_nitems > 0)
	{
		result = g_input_buffer[g_input_buffer_front];
		
		g_input_buffer_front++;
		if (g_input_buffer_front == INPUT_BUFFER_SIZE)
		{
			g_input_buffer_front = 0;
		}
		g_input_buffer_nitems--;
	}
	
	return (result);
}

int g_generate_eof = 0;

int get_char(void)
{
	if (g_generate_eof)
	{
		return (EOF);
	}
	
	if (g_input_buffer_nitems == 0)
	{
		for (;;)
		{
			KBD_message msg;
	
			msg = get_kbd_message();
			if (msg.msg == MSG_KEYDOWN)
			{
				if (msg.flags & 0x02) // ctrl is down
				{
					if (msg.vk == 'Z')
					{
						put_char('^');
						put_char('Z');
						add_input_buffer_char(EOF);
						g_generate_eof = 1;
					}
				}
				else
				{
					if (isprint(msg.vk))
					{
						char c = (msg.flags & 0x01) ? msg.vk : tolower(msg.vk);
						put_char(c);
						
						add_input_buffer_char(c);
					}
					else if (msg.vk == VK_BACK && g_input_buffer_nitems)
					{
						int c;
						put_char('\b');
						console_place_byte(' ');
						
						c = remove_input_buffer_char();
						if (c == EOF)
						{
							put_char('\b');
							console_place_byte(' ');
						}
					}
					else if (msg.vk == VK_RETURN)
					{
						put_char('\n');
						put_char('\r');
						
						add_input_buffer_char('\n');
						
						break;
					}
				}
			}
		}
	}
	
	return remove_input_buffer_char();
}