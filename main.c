/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */
 /****AVERAGE CUREENT CALCULATOR
 Author: Andrii Androsovych
 description- this software is using
for measure average value of current
 on frame period.In this app frame = 2.0 Sec.
 When you have pulse skipping TRIAC control AC circuit -
 this software count an average current along frame time.
 In other words - it  measure  half-wave area of input signal
 For example- frame=2Sec i.e. 200 cycles when 50Hz.
 200cycles = I, 100Cycles = 1/2, 50cycles = I/4.
 #INPUT SIGNAL GPIOD 2 - AC voltage.Input pin must be connect 
 to Vcc/2. 
 #OUTPUT SIGNAL - PWM GPIOC 4. OUTPUT PWM 8KHz 
 ***************************************************/
#include "stm8s.h"
#include "stdlib.h"

 volatile char overSamplingCounter = 0;
 volatile short overSamplingAcc = 0;
 
 static long average, avg, avg1, avg2;
 static int filterVar1;
 static int filterVar2;
 static int averageCounter = 0;
 static int resultFinal;
//15 Hz high pass filter
#define FRAME_SIZE 

long highPassAC (  long new_sample) {
 
	 //to increase acuracy of filter - multiply an input signal
	  new_sample  <<= 11;
		
    avg -= avg >> 8;
    avg += new_sample >> 8;
		
		avg1 -= avg1 >> 8;
    avg1 += avg >> 8;
		
	 
		//restore an input amplitude
    return (new_sample - avg1) >> 11;
}


 
 


int lowPassDC1 ( int new_sample) {
 
 //firrst order average moving filter
  static int acc = 0;
	 unsigned short result = 0;
	 static unsigned short counter=0;
	 
		 acc += new_sample;
		 counter++;
		 if(counter > 64) {
			 result = acc >> 6;
			 counter = 0;
		 }
		
    return result;
}

/*****/

void output_pwm_conf(void);
void sampling_timer_cfg(void);
void adc_init(void);

void delay (unsigned short x) {
	while(x) {
		x--;
	}
}






main() {
  
  	
  CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1); // set 16 MHz for CPU
	
	GPIO_DeInit(GPIOB);
	GPIO_Init(GPIOB,GPIO_PIN_5,GPIO_MODE_OUT_PP_LOW_FAST);//LED
	GPIO_DeInit(GPIOC);
	GPIO_Init(GPIOC,GPIO_PIN_4,GPIO_MODE_OUT_PP_LOW_FAST );//pwm
	GPIO_DeInit(GPIOD);
	GPIO_Init(GPIOD, GPIO_PIN_1, GPIO_MODE_IN_FL_NO_IT);//ADC
	
	
	
	  adc_init();
  ///PWM output channel (like DAC) config
	//tim4 - output pwm timebase
	 output_pwm_conf();
	 //tim4 - adc samling timebase
	sampling_timer_cfg();
	enableInterrupts();
	while (1){
   //is oversampling full?
		if (overSamplingCounter >= 15) {
			///shifting an oversampling result
				filterVar1 = overSamplingAcc >> 2;
				//reset oversampling accumulator variables
				overSamplingAcc = 0;
				overSamplingCounter = 0;
					//toggle a GPIO pin
				GPIO_WriteReverse(GPIOB,GPIO_PIN_5 );
				//cutting DC (goughly)
				filterVar1 = (filterVar1 - 2048);
				//filtering 
				 filterVar2 = highPassAC((long)filterVar1);
				 //convert to DC
				 filterVar1 = abs(filterVar2);
				 //add to the common result
				 average += filterVar1;
				 averageCounter++;
					//toggle a GPIO pin
			GPIO_WriteReverse(GPIOB,GPIO_PIN_5 );
		}
		//has a whole period (2Sec) ellapsed?
			if (averageCounter >= FRAME_SIZE) {
				//calculating verage per period 2Sec
				resultFinal = average >> 12;
				//clear variables
				average = 0;
				averageCounter = 0;
				//to PWM output
				TIM1_SetCompare4(resultFinal);
			
			}
		

	}
}

void sampling_timer_cfg (void) {
	//TIM4 start procedure - it defines ADC program sampling
	TIM4_Cmd(DISABLE);       // stop
	//2048 x 16 = 32768 samples per second
	TIM4_TimeBaseInit(TIM4_PRESCALER_4, 0x81);
	TIM4_ClearFlag(TIM4_FLAG_UPDATE);
	TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
	TIM4_Cmd(ENABLE);
	
}


void adc_init (void) {
	ADC1_DeInit();
	
	ADC1_Init(ADC1_CONVERSIONMODE_SINGLE, ADC1_CHANNEL_3,
	ADC1_PRESSEL_FCPU_D2, ADC1_EXTTRIG_GPIO,
	DISABLE, ADC1_ALIGN_RIGHT,
	ADC1_SCHMITTTRIG_ALL, DISABLE);

	ADC1_Cmd(ENABLE);
}


 void output_pwm_conf(void) {

   TIM1_DeInit();

  /* Time Base configuration */
  /*
  TIM1_Period = 1024
  TIM1_Prescaler = 0
  TIM1_CounterMode = TIM1_COUNTERMODE_UP
  TIM1_RepetitionCounter = 0
  */
    /// 11bit resolution pwm
  TIM1_TimeBaseInit(0, TIM1_COUNTERMODE_UP, 2048, 0); 

  /* Channel 1, 2,3 and 4 Configuration in PWM mode */
  
  /*
  TIM1_OCMode = TIM1_OCMODE_PWM2
  TIM1_OutputState = TIM1_OUTPUTSTATE_ENABLE
  TIM1_OutputNState = TIM1_OUTPUTNSTATE_ENABLE
  TIM1_Pulse = CCR1_Val
  TIM1_OCPolarity = TIM1_OCPOLARITY_LOW
  TIM1_OCNPolarity = TIM1_OCNPOLARITY_HIGH
  TIM1_OCIdleState = TIM1_OCIDLESTATE_SET
  TIM1_OCNIdleState = TIM1_OCIDLESTATE_RESET
  
  */
  

  /*TIM1_Pulse = CCR4_Val*/
  TIM1_OC4Init(TIM1_OCMODE_PWM1, TIM1_OUTPUTSTATE_ENABLE, 0x00ff, TIM1_OCPOLARITY_HIGH, TIM1_OCIDLESTATE_SET);

  /* TIM1 counter enable */
  TIM1_Cmd(ENABLE);

  /* TIM1 Main Output Enable */
  TIM1_CtrlPWMOutputs(ENABLE);
}