#include "stm8l15x.h"
#include "stm8l15x_adc.h"
#include "stm8l15x_clk.h"

#include "delay.h"
#include "battery.h"

/* Theorically BandGAP 1.224volt */
#define VREF 		1.224L

/*
	ADC Converter
	LSBIdeal = VREF/4096 or VDA/4096
*/
#define ADC_CONV 	4096

u16 get_ref_voltage_data(void)
{
	uint8_t i;
	uint16_t res;

	/* Enable ADC clock */
	CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, ENABLE);

	/* de-initialize ADC */
	ADC_DeInit(ADC1);

	/*ADC configuration
	ADC configured as follow:
	- Channel VREF
	- Mode = Single ConversionMode(ContinuousConvMode disabled)
	- Resolution = 12Bit
	- Prescaler = /1
	- sampling time 9 */

	ADC_VrefintCmd(ENABLE);
	delay_10us(3);


	ADC_Cmd(ADC1, ENABLE);
	ADC_Init(ADC1, ADC_ConversionMode_Single,
	ADC_Resolution_12Bit, ADC_Prescaler_1);

	ADC_SamplingTimeConfig(ADC1, ADC_Group_FastChannels, ADC_SamplingTime_9Cycles);
	ADC_ChannelCmd(ADC1, ADC_Channel_Vrefint, ENABLE);
	delay_10us(3);

	/* initialize result */
	res = 0;
	for(i=8; i>0; i--)
	{
		/* start ADC convertion by software */
		ADC_SoftwareStartConv(ADC1);
		/* wait until end-of-covertion */
		while( ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == 0 );
		/* read ADC convertion result */
		res += ADC_GetConversionValue(ADC1);
	}

	/* de-initialize ADC */
	ADC_VrefintCmd(DISABLE);

	ADC_DeInit(ADC1);

	/* disable SchmittTrigger for ADC_Channel_24, to save power */
	ADC_SchmittTriggerConfig(ADC1, ADC_Channel_24, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, DISABLE);
	ADC_ChannelCmd(ADC1, ADC_Channel_Vrefint, DISABLE);

	return (res>>3);
}


float read_battery_voltage_mv(void)
{
	float batt_vol_mv;
	u16 ref_vol_mv;

	ref_vol_mv = get_ref_voltage_data();

	batt_vol_mv = (VREF/ref_vol_mv) * ADC_CONV;

	return batt_vol_mv;
}
