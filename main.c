#include "Board_LED.h"
#include "LPC17xx.h"                    // Device header
#include "RTE_Components.h"             // Component selection
#include "GPIO_LPC17xx.h"               // Keil::Device:GPIO
#include "PIN_LPC17xx.h"                // Keil::Device:PIN
#include "Board_Joystick.h"             // ::Board Support:Joystick
#include "Open1768_LCD.h"
#include "LCD_ILI9325.h"
#include <stdio.h>
#include <math.h>
#include "asciiLib.h"
#include "TP_Open1768.h"
#include "stdlib.h"

//////////////////////////////////////////////////////////////////////////////////////
#define PI 3.14159265359

uint8_t table[8] = { false };
uint32_t akordy[8] = { 262, 294, 330, 350, 392, 440, 493, 523 };
unsigned char* litery[8] = { "do", "re", "mi", "fa", "so", "la", "si", "do" };
uint16_t sinus[100];
int y = 0;
int a, b, i, j, k, l;
int y_coords[100];
int tmp = 240 * 320;
int sum, counter, y_temp;

struct {
	uint32_t source;
	uint32_t destination;
	uint32_t next;
	uint32_t control;
} LLIO;
//////////////////////////////////////////////////////////////////////////////////////
int freq(int value) {
	return 250000. / value;
}
//////////////////////////////////////////////////////////////////////////////////////
void send(char *);
//////////////////////////////////////////////////////////////////////////////////////
void setup_LCD(void) {
    LPC_GPIO0->FIODIR &= (0 << 19);
    LPC_GPIOINT->IO0IntEnR  |= (1 << 19);
    PIN_Configure (0, 19, 0, PIN_PINMODE_PULLUP , 0);
    NVIC_EnableIRQ(EINT3_IRQn);
    lcdConfiguration();
    init_ILI9325();
		touchpanelInit();
}
//////////////////////////////////////////////////////////////////////////////////////
void TIMER0_IRQHandler(void) {
	LPC_TIM0->IR = 1;
}
//////////////////////////////////////////////////////////////////////////////////////
void LCD_ascii_write(int font, unsigned char * c, int x, int y, int color) {
    int i, j;
    unsigned char buffer[16];  
    int k = 0;
		lcdWriteReg(ENTRYM, 0x1038);
    while (c[k] != '\0') {
			GetASCIICode(font, buffer, c[k]);
			for (i = 0; i < 30; ++i) {
				for (l = 0; l < 2; ++l) {
					lcdSetCursor(x - i - l, y + k * 15);
					lcdWriteReg(DATA_RAM, LCDGreen);
					for (j = 14; j >= 0; --j) {
							if ((buffer[i/2] & (1 << j/2)))
									lcdWriteData(color);
							else
									lcdWriteData(LCDGreen);
					}
				}
			}
			++k;
    }        
}
//////////////////////////////////////////////////////////////////////////////////////
/* Wymiary:
			[x_max, y_max] = [3700, 3700]
			[x_max, y_min] = [3700, 300]
			[x_min, y_max] = [430, 3700]
			[x_min, y_min] = [430, 300]
*/
void EINT3_IRQHandler(void) {
	counter = 0;
	sum = 0;
	for (i = 0; i < 50; ++i) {
		y_coords[i] = touchpanelReadX();
		if (y_coords[i] != 4095)
			++counter;
		else
			y_coords[i] = 0;
	}
	if (counter > 40 && y_coords[49] != 4095) {
		for (i = 0; i < 50; ++i) {
			sum += y_coords[i];
		}
		
		y = sum / counter;
		y = (double)(y - 220.) * (320. / 3500.);
		y = (y > 0) ? y : 1;
		y = (y < 320) ? y : 320;
		y = y - (y % 40);
		y = y / 40;
		if (y < 8) {
			LPC_TIM0->TCR = 1;
			LPC_TIM0->MR0 = freq(akordy[y]);
			LPC_TIM0->TC = 0;
			table[y] = true;
			y_temp = y * 40;
			lcdSetCursor(0, y_temp);
			lcdWriteReg(DATA_RAM, LCDBlack);
			lcdWriteIndex(DATA_RAM);
			for (k = 0; k < 7800; ++k)
				lcdWriteData(LCDTouch);
		}	
	}
	else
		LPC_TIM0->TCR = 0;
	
  LPC_GPIOINT->IO0IntClr |= (1 << 19);
}
//////////////////////////////////////////////////////////////////////////////////////
void TIMER1_IRQHandler(void) {
	if (touchpanelReadX() > 4000) {
		if (touchpanelReadX() > 4000) {
			LPC_TIM0->TCR = 0;
			for (k = 0; k < 8; ++k) {
				if (table[k]) {
					table[k] = false;
					lcdSetCursor(0, k * 40);
					lcdWriteIndex(DATA_RAM);
					lcdWriteReg(DATA_RAM, LCDBlack);
					lcdWriteIndex(DATA_RAM);
					for (l = 0; l < 7800; ++l)
						lcdWriteData(LCDButton);
				}
			}
		} 
	}
	LPC_TIM1->IR = (1 << 0);
}
//////////////////////////////////////////////////////////////////////////////////////
int main(void) {
	/*
		LCD and touchpanel
	*/
	setup_LCD();
	/*
		Timer 0
	*/
	LPC_TIM0->PR = 0;
	LPC_TIM0->MR0 = freq(262);
	LPC_TIM0->MCR = 3;
	LPC_TIM0->TCR = 1;
	NVIC_EnableIRQ(TIMER0_IRQn);
	/*
		Timer 1
	*/
	LPC_TIM1->PR = 0;
	LPC_TIM1->MCR = 3;
	LPC_TIM1->MR0 = 4000000;
	LPC_TIM1->TCR = 1;
	NVIC_EnableIRQ(TIMER1_IRQn);
	/*
		rysowanie organkow
	*/
	lcdWriteReg(ADRX_RAM, 0);
	lcdWriteReg(ADRY_RAM, 0);
	lcdSetCursor(0, 0);
	lcdWriteIndex(DATA_RAM);
  int i = 0;
	int j = 0;
	lcdSetCursor(0, 0);
	lcdWriteIndex(DATA_RAM);
	for (i = 0; i < 7; ++i) {
		for (j = 0; j < 39; ++j) {
			for (k = 0; k < 200; ++k)
				lcdWriteData(LCDButton);
			for (l = 0; l < 40; ++l)
				lcdWriteData(LCDGreen);
		}
		for (j = 0; j < 240; ++j)
			lcdWriteData(LCDBlack);
	}
	for (j = 0; j < 39; ++j) {
		for (k = 0; k < 200; ++k)
			lcdWriteData(LCDButton);
		for (l = 0; l < 40; ++l)
			lcdWriteData(LCDGreen);
	}
	for (j = 0; j < 240; ++j)
		lcdWriteData(LCDBlack);
	/*
		rysowanie legendy organków
	*/
	lcdWriteReg(ADRX_RAM, 0);
	lcdWriteReg(ADRY_RAM, 0);
	lcdWriteReg(DATA_RAM, LCDWhite);
	for (i = 0; i < 8; ++i) {
			LCD_ascii_write(1, litery[i], 235, 4 + (i * 40), LCDBlack);
	}
	lcdWriteReg(HADRPOS_RAM_END, 0xC7);
	lcdWriteReg(ENTRYM, 0x1030);
	/*
		sinus tablica
	*/
	for (i = 0; i < 100; ++i) {
		sinus[i] = 100 * sin((float)i * PI / 50.) + 512;
		sinus[i] = ((sinus[i]) << 6) | 1 << 16;
	}
	/*
		DMA konfiguracja
	*/
	// LLIO struct
	LLIO.source 				= (uint32_t)sinus;
	LLIO.destination 		= (uint32_t)&(LPC_DAC->DACR);
	LLIO.next 					
	= (uint32_t)&LLIO;
	LLIO.control				= (100 << 0) | (1 << 26) | (1U << 31) | (0 << 12) | (0 << 15) | (1 << 18) | (1 << 21);
	// DMA rejestry
	LPC_SC->PCONP 			       |= 1 << 29;
	LPC_SC->DMAREQSEL 			    = 1 << 0;
	LPC_GPDMA->DMACConfig 		  = 1 << 0;
	LPC_GPDMA->DMACIntTCClear   = 1 << 0;
	LPC_GPDMA->DMACIntErrClr 	  = 1 << 0;
	LPC_GPDMACH0->DMACCSrcAddr  = (uint32_t)sinus;
	LPC_GPDMACH0->DMACCDestAddr = (uint32_t)&(LPC_DAC->DACR);
	LPC_GPDMACH0->DMACCLLI		  = (uint32_t)&LLIO;
	LPC_GPDMACH0->DMACCControl 	= LLIO.control;
	LPC_GPDMACH0->DMACCConfig 	= (1 << 0) | (8 << 6) | (1 << 11) | (1 << 14) | (1 << 15);
	LPC_TIM0->TCR = 0;
	/*
		DAC konfiguracja
	*/
	PIN_Configure(0, 26, PIN_FUNC_2, PIN_PINMODE_TRISTATE, 0);
	while (1) {}

  return 0;
}

