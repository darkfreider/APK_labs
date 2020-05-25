#include <stdio.h>
#include <string.h>
#include <dos.h>

typedef unsigned int uint16_t;
typedef unsigned char uint8_t;

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
	outp(com_port + IER_OFFS, 0x00); // TODO(max): set interrupst
	
	outp(com_port + LCR_OFFS, 0x80); // set DLAB
	outp(com_port + LO_OFFS, 0x60);
	outp(com_port + HI_OFFS, 0x00);
	
	outp(com_port + LCR_OFFS, 0x03); // 8 bits, no parity, one stop bit
	
	outp(com_port + IIR_OFFS, 0x04); // 0b00000100
	outp(com_port + MCR_OFFS, 0x0b);
}


int main(int argc, char *argv[])
{
	char msg[] = "hello world\n";
	char *c = msg;
	
	_disable();
	init_serial(COM2);
	_enable();
	
	if (argv[1])
	{
		c = argv[1];
	
		while (*c)
		{
			write_serial(*c, COM2);
			c++;
		}
		write_serial('\n', COM2);
	}
	
	return (0);
}





