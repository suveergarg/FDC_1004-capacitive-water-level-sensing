#define F_CPU 16000000UL                                    // Clock speed

#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_TRIES 50
#define BAUD 9600                                           //Baud Rate:9600
#define BAUD_PRESCALE (((F_CPU / (BAUD * 16UL))) - 1)
#define dev_address 0b01010000
#define I2C_START 0
#define I2C_DATA  1
#define I2C_STOP  2

void FDC1004_init(void);
void TWI_start(void);
void TWI_repeated_start(void);
void TWI_init_master(void);
void TWI_write_address(unsigned char);
void TWI_read_address(unsigned char);
void TWI_write_data(unsigned char);
void TWI_read_data(void);
void TWI_stop(void);
void USART_send( unsigned char );
void USART_init(void);
uint16_t Read_Frame(char);
void Write_Frame(char,char,char);
void UART_Puts(char*);

int main(void)
{
	USART_init();          // Initialize USART : Used for Debugging
	
	TWI_init_master();	// Initialize I2C interface : Used for communicating with FDC1004
	
	FDC1004_init();		// Configure the FDC1004 IC 
	
	double result, result2;			
	int32_t Meas_1, Meas_2, Meas_3, C0,C01[100]; // To store channel Measurements
	char str[20], str_C0[20],str_Meas_1[20],str_Meas_2[20],str_Meas_3[20],str_result1[20],str_result2[20];
	uint16_t LW=0,HW=0;
	uint8_t i=0,j=0;
	// Collecting forst 100 samples to pick value for calibration
	for(i=0;i<100;i++)
	{HW =  Read_Frame(0x04);
	LW =  Read_Frame(0x05);
	C01[i]= (HW * 256.0 + (LW>>8));
	
	_delay_ms(100);
	}
	
	//Filtering
	for ( i =0; i< 99 ; i++) {
		for ( j = (i+1); j< 100; j++) {
			if (C01[i] > C01[j]) {
				uint32_t medaux = C01[i];
				C01[i] = C01[j];
				C01[j] = medaux;
			}
		}
	}
	 C0=C01[50]; 
	 i=0;j=0;
	
    while(1)
	{
		
		//FDC1004_init();
		HW =  Read_Frame(0x02);
		LW =  Read_Frame(0x03);
		Meas_1 =  (HW * 256.0) + (LW/256.0);
		
		HW =  Read_Frame(0x04);
		LW =  Read_Frame(0x05);
		//         USART_send(HW>>8);       : For Debugging
		//         USART_send(HW);
		//         USART_send(LW>>8);
		Meas_2 =  (HW * 256.0) + (LW/260.0);
		
		HW =  Read_Frame(0x06);
		LW =  Read_Frame(0x07);
		Meas_3 =  (HW * 256.0) + (LW/256.0);
		
		result=((double)Meas_2-(double)C0)/100000.0;
		
		//reset
		
		C01[i++]=Meas_2;
		if(C01[i-1]==C01[i])j++;
		if(j==5 || result<0)
		{
			Write_Frame(0x0C,0xFF,0xFF);
			FDC1004_init();i=0,j=0;
		}
		if(i==10)i=0;
		
		double result2=((double)Meas_2-(double)C0)/((double)Meas_3-(double)Meas_1);
		
		//result=((double)Meas_2-(double)C0);
	    
		double result1=(double)Meas_3-(double)Meas_1;
		
		memset(str,         0, sizeof str);
		memset(str_C0,      0, sizeof str_C0);
		memset(str_Meas_1,  0, sizeof str_Meas_1);
		memset(str_Meas_2,  0, sizeof str_Meas_2);
		memset(str_Meas_3,  0, sizeof str_Meas_3);
		memset(str_result1, 0, sizeof str_result1);
		memset(str_result2, 0, sizeof str_result2);
		
		dtostrf(result,		9, 5, str);
		dtostrf(C0,			9, 1, str_C0);
		dtostrf(Meas_2,		9, 1, str_Meas_2);
		dtostrf(Meas_1,		9, 1, str_Meas_1);
		dtostrf(Meas_3,		9, 1, str_Meas_3);
		dtostrf(result1,	9, 1, str_result1);
		dtostrf(result2,	9, 6, str_result2);
		// Dispay results through USART for Debugging
		if(result>0)
		{
		UART_Puts("C0: ");   UART_Puts(str_C0);
		UART_Puts(" M2: ");   UART_Puts(str_Meas_2);
		UART_Puts(" M3: ");   UART_Puts(str_Meas_3);
		UART_Puts(" M1: ");   UART_Puts(str_Meas_1);
		UART_Puts(" M3-M1 "); UART_Puts(str_result1);
		UART_Puts(" R: ");    UART_Puts(str);
		UART_Puts(" D: ");    UART_Puts(str_result2);
		UART_Puts("\r\n");
		}
		_delay_ms(3000);
		
		
	}
	
	return 0;
}


void FDC1004_init()
{
	// Writing specific values to FDC registers based on the Datasheet
	Write_Frame(0x08,0x1C,0x00);		
	Write_Frame(0x09,0x3C,0x00);
	Write_Frame(0x0A,0x5C,0x00);
	Write_Frame(0x0B,0x7C,0x00);
	Write_Frame(0x0C,0x05,0xFF);
	Write_Frame(0x11,0x40,0x00);
	Write_Frame(0x12,0x40,0x00);
	Write_Frame(0x13,0x40,0x00);
	Write_Frame(0x14,0x40,0x00);
}

void USART_init(void)
{
	
	UCSR0B |= (1<<RXEN0)  | (1<<TXEN0);
	UCSR0C |= (1<<UCSZ00) | (1<<UCSZ01);
	UBRR0H  = (BAUD_PRESCALE >> 8);
	UBRR0L  = BAUD_PRESCALE;
}

void USART_send(unsigned char data)
{
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

void TWI_init_master(void) // Function to initialize master
{
	TWBR=0x48;  // Bit rate
	TWSR=(0<<TWPS1)|(0<<TWPS0); // Setting prescalar bits
	// SCL freq= F_CPU/(16+2(TWBR).4^TWPS)
}

void TWI_start(void)
{
	// Clear TWI interrupt flag, Put start condition on SDA, Enable TWI
	
	TWCR= (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT))); // Wait till start condition is transmitted
	while((TWSR & 0xF8)!= TW_START)USART_send('e'); // Check for the acknowledgement
}

void TWI_repeated_start(void)
{
	// Clear TWI interrupt flag, Put start condition on SDA, Enable TWI
	
	TWCR= (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT))); // Wait till start condition is transmitted
	while((TWSR & 0xF8)!= TW_REP_START); // Check for the acknowledgement
}

void TWI_write_address(unsigned char data)
{
	TWDR=data;// Address and write instruction
	//USART_send(TWDR);
	TWCR=(1<<TWINT)|(1<<TWEN);    // Clear TWI interrupt flag,Enable TWI
	while (!(TWCR & (1<<TWINT)));// Wait till complete TWDR byte transmitted
	//USART_send(TWSR);
	while((TWSR & 0xF8)!= TW_MT_SLA_ACK)USART_send('e'); // Check for the acknowledgement

}

void TWI_read_address(unsigned char data)
{
	TWDR=data;  // Address and read instruction
	TWCR=(1<<TWINT)|(1<<TWEN);    // Clear TWI interrupt flag,Enable TWI
	while (!(TWCR & (1<<TWINT))); // Wait till complete TWDR byte received
	while((TWSR & 0xF8)!= 0x40);  // Check for the acknowledgement
}

void TWI_write_data(unsigned char data)
{
	TWDR=data;  // put data in TWDR
	TWCR=(1<<TWINT)|(1<<TWEN);    // Clear TWI interrupt flag,Enable TWI
	while (!(TWCR & (1<<TWINT))); // Wait till complete TWDR byte transmitted
	while((TWSR & 0xF8) != TW_MT_DATA_ACK); // Check for the acknowledgement
}

void TWI_read_data(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);    // Clear TWI interrupt flag,Enable TWI
	while (!(TWCR & (1<<TWINT))); // Wait till complete TWDR byte transmitted
	while((TWSR & 0xF8) != TW_MR_DATA_ACK); // Check for the acknowledgement
}
void TWI_read_dataN(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN);    // Clear TWI interrupt flag,Enable TWI
	while (!(TWCR & (1<<TWINT))); // Wait till complete TWDR byte transmitted
	while((TWSR & 0xF8) != TW_MR_DATA_NACK); // Check for the acknowledgement
}
void TWI_stop(void)
{
	// Clear TWI interrupt flag, Put stop condition on SDA, Enable TWI
	TWCR= (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	while(!(TWCR & (1<<TWSTO)));  // Wait till stop condition is transmitted
}

uint16_t Read_Frame(char pointer_addr)  // Function to communicate with the FDC and organise the received frames
{
	
	uint16_t result;
	uint8_t dataMSB,dataLSB;
	TWI_start();
	TWI_write_address(dev_address<<1|TW_WRITE);
	TWI_write_data(pointer_addr);
	TWI_repeated_start();
	TWI_read_address(dev_address<<1|TW_READ);
	TWI_read_data();
	dataMSB=TWDR;
	//    USART_send(dataMSB);
	TWI_read_dataN();
	dataLSB=TWDR;
	//    USART_send(dataLSB);
	result=(dataMSB<<8|dataLSB);
	TWI_stop();
	_delay_ms(1);
	return result;
}

void Write_Frame(char pointer_addr,char dataMSB,char dataLSB) // Function to write to the FDC registers
{
	TWI_start();
	TWI_write_address(dev_address<<1|TW_WRITE);
	TWI_write_data(pointer_addr);
	TWI_write_data(dataMSB);
	TWI_write_data(dataLSB);
	TWI_stop();
	_delay_ms(100);
	
}
void UART_Puts(char *str) // To put String on the UART
{
	
	while( *str != '\0' ){
		USART_send( *str++ );
	}
}
