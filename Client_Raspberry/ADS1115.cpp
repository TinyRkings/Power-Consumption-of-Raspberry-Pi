#include "ParallelKMeans.h"

int Current_Power(int file)
{
	//A0_A1_voltage
	// Select configuration register(0x01)
	// AINP = AIN0 and AINN = AIN1, +/- 6.144V
	// Continuous conversion mode, 128 SPS(0x84, 0x83)
	char config[3] = {0};
	config[0] = 0x01;
	config[1] = 0x80;
	config[2] = 0x83;
	int raw_adc = 0;
	// AINP = AIN0 and AINN = AIN3, +/- 0.256V
	char config2[3] = {0};
	config2[0] = 0x01;
	config2[1] = 0xAA;
	config2[2] = 0x83;
	int raw_adc2 = 0;
	char reg[1] = {0x00};
	
	//A0_A1
	write(file, config, 3);
	usleep(30000);//30 ms
	// Read 2 bytes of data from register(0x00)
	// raw_adc msb, raw_adc lsb
	write(file, reg, 1);
	char data[2]={0};
	if(read(file, data, 2) != 2)
	{
		printf("Error : Input/Output Error \n");
	}
	else 
	{
		// Convert the data
		raw_adc = (data[0] * 256 + data[1]);
		if (raw_adc > 32767)
		{
			raw_adc -= 65535;
		}
		//printf("Digital Value of Analog Input: %d \n", raw_adc);
	}
	//A1_A3
	write(file, config2, 3);
	usleep(30000);
	// Read 2 bytes of data from register(0x00)
	// raw_adc msb, raw_adc lsb
	write(file, reg, 1);
	char data2[2]={0};
	if(read(file, data2, 2) != 2)
	{
		printf("Error : Input/Output Error \n");
	}
	else 
	{
		// Convert the data
		raw_adc2 = (data2[0] * 256 + data2[1]);
		if (raw_adc2 > 32767)
		{
			raw_adc2 -= 65535;
		}
		//printf("Digital Value of Analog Input: %d \n", raw_adc2);
	}
	int power = (int)raw_adc * 0.1875 * raw_adc2 * 0.0078125/100; //mw
	return power;
}