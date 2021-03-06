/* FCM Bootloader - a simple and rather quick and dirty Bootloader, which allows binary download by FCM-Manager.
 *
 * (c) 2013-2014 by Fabian Huslik
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <asf.h>
#include "conf_usb.h"
#include "fcmcp_bl.h"

#define BOOTLOADERSIZE 16384
#define FLASHSTART 0x80000000

bool waitforUSB(void);
void init_pins(void);
void StartApplication(void); 

static volatile bool main_b_cdc_enable = false;


// wait 3 seconds for USB
bool waitforUSB(void)
{
	int ms=3000;
	while(ms >0)
	{
		delay_ms(100);
		LED_GREEN_TOG;
		ms -=100;
		if(main_b_cdc_enable)
		{
			LED_GREEN_OFF;
			return true;
		}		
	}
	LED_GREEN_OFF;
	return false;
}

typedef void(*vfpv_t)(void); // void function pointer

void StartApplication(void) 
{
	uint32_t* p = (uint32_t*) FLASHSTART + BOOTLOADERSIZE;
	if(*p != 0xffffffff && *p != 0x00000000)
	{
		// hop to start of code App
		//asm("lda.w   pc, _stext");
		vfpv_t vp = (vfpv_t)0x80004000;
		asm("ssrf 16");asm("nop");asm("nop"); // disable interrupts and flush pipeline
		vp();
		
	}else
	{
		AVR32_PM.gplp[0] = 0xB00710AD; // stay in BL
		LED_RED_ON;
		delay_ms(1000);
		LED_RED_OFF;
	}
}


int main(void)
{
	/*wdt_opt_t opt = {
			.us_timeout_period = 100000; // 100ms
	};
	wdt_enable(&opt);*/
	
	
	irq_initialize_vectors();
	cpu_irq_enable();

	sysclk_init();
	
	init_pins();
	LED_RED_OFF;
	LED_GREEN_OFF;
	LED_BLUE_OFF;	
	
// 	if(AVR32_PM.RCAUSE.por || AVR32_PM.RCAUSE.bod)
// 	{
// 		LED_BLUE_ON;
// 	}
// 	else if (AVR32_PM.RCAUSE.wdt)
// 	{
// 		LED_RED_ON;
// 		LED_GREEN_ON;
// 		LED_BLUE_ON;
// 		// fixme create fast path to "flight" if this happens!!!
// 		// meanwhile we crash here
// 		//emstop(12);
// 	}
	wdt_opt_t wdopt;
	wdopt.us_timeout_period = 10000000; // 10s // this is the only one parameter.
	wdt_enable(&wdopt); // enable WD

	// Start USB stack to authorize VBus monitoring
	udc_start();

	// The main loop manages only the power mode
	// because the USB management is done by interrupt
	while(1) 
	{
		wdt_clear();
		if((AVR32_PM.gplp[0] == 0xB00710AD || AVR32_USBB.USBSTA.vbus) && !(AVR32_PM.gplp[0] == 0xB007)) // jump to BL if connected to USB
		{
			
			// wait for 3s that USB becomes active, if yes, stay here
			if(waitforUSB() == 1)
			{
				while(1)
				{
					// sit here and BL until Reset
					wdt_clear();
					// comm works in ISR
					delay_ms(300);
					LED_BLUE_TOG;
				}
			}
			else
			{
				
			}
		}
		else
		{
			// check, if there is sth.
			wdt_disable();
			StartApplication();
		}
		
		
		

	}
}

void main_suspend_action(void)
{
	
}

void main_resume_action(void)
{
	
}

uint8_t USB_buffer[1999];
int rem;
 
void main_notifyRX(void)
{
	uint32_t leftover;
	
	// received:
	uint32_t len = udi_cdc_get_nb_received_data();
	if (len > 0)
	{
		leftover = udi_cdc_read_buf(&USB_buffer,len); // there might be some left!
		
		fcmcp_analyzePacket(USB_buffer,len);
		
	}
	else
	{
		// Fifo empty
	}

}

void main_notifyTXE(void)
{
	// send stuff on empty during read mode
	// here we could send in isr, but we poll.
	
}

void main_sof_action(void)
{
	if (!main_b_cdc_enable)
		return;
	
}

bool main_cdc_enable(uint8_t port)
{
	main_b_cdc_enable = true;
	// Open communication
	
	return true;
}

void main_cdc_disable(uint8_t port)
{
	main_b_cdc_enable = false;
	// Close communication
	
}

void main_cdc_set_dtr(uint8_t port, bool b_enable)
{
	if (b_enable) {
		// Host terminal has open COM
		
	}else{
		// Host terminal has close COM
		
	}
}


void init_pins(void)
{
	// init all pins


	// switch the GPIOs to their functions:
	// 0 = A, 1 = B, 2 = C, (3 = D only for UC3B512)
	static const gpio_map_t iomap =
	{
		{AVR32_PIN_PA05, 0 }, // EIC - EXTINT[0] "Int1 Accel"
		{AVR32_PIN_PA06, 0 }, // EIC - EXTINT[1] "RC2" // debug

		{AVR32_PIN_PA09, 0 }, // TWI - SCL
		{AVR32_PIN_PA10, 0 }, // TWI - SDA
		{AVR32_PIN_PA11, 2 }, // PWM - PWM[0]  "PWM 1"
		{AVR32_PIN_PA12, 2 }, // PWM - PWM[1]  "PWM 2"
		{AVR32_PIN_PA13, 1 }, // PWM - PWM[2]  "PWM 3"
		{AVR32_PIN_PA14, 2 }, // EIC - EXTINT[2] "RC3" // debug
		{AVR32_PIN_PA15, 0 }, // SPI0 - SCK
		{AVR32_PIN_PA16, 2 }, // PWM - PWM[4]  "PWM 5"

		{AVR32_PIN_PA23, 2 }, // EIC - EXTINT[3] "RC4" // debug
		{AVR32_PIN_PA24, 1 }, // SPI0 - NPCS[0]
		{AVR32_PIN_PA25, 1 }, // PWM - PWM[3]  "PWM 4"
		{AVR32_PIN_PA26, 1 }, // USART2 - TXD
		{AVR32_PIN_PA27, 1 }, // USART2 - RXD
		{AVR32_PIN_PA28, 2 }, // SPI0 - MISO
		{AVR32_PIN_PA29, 2 }, // SPI0 - MOSI

		{AVR32_PIN_PA31, 2 }, // PWM - PWM[6]  "PWM 6"
		{AVR32_PIN_PB00, 0 }, // TC - A0
		{AVR32_PIN_PB01, 0 }, // TC - B0
		{AVR32_PIN_PB02, 0 }, // EIC - EXTINT[6] "Gyro L3GD20 Int2"
		
		// next will be switched dynamically, if serial is required.
		{AVR32_PIN_PB03, 0 }, // EIC - EXTINT[7] "RC1"
		//{AVR32_PIN_PB03, 2 }, // USART1 - RXD "serial input RX"



		{AVR32_PIN_PB10, 2 }, // USART0 - RXD
		{AVR32_PIN_PB11, 2 } // USART0 - TXD
	};
	// enable modules
	gpio_enable_module(iomap, sizeof(iomap) / sizeof(iomap[0]));


	// ***************************************
	// init GPIOs

	gpio_enable_gpio_pin(AVR32_PIN_PA20); // LED
	gpio_enable_gpio_pin(AVR32_PIN_PA21); // LED
	gpio_enable_gpio_pin(AVR32_PIN_PA22); // LED
	gpio_enable_gpio_pin(AVR32_PIN_PA30); // RX power switch 3.3V




	/*
	// debug
	gpio_enable_gpio_pin(AVR32_PIN_PA06);
	gpio_enable_gpio_pin(AVR32_PIN_PA14);
	gpio_enable_gpio_pin(AVR32_PIN_PA23);
	gpio_configure_pin(AVR32_PIN_PA06, GPIO_DIR_OUTPUT | GPIO_DRIVE_HIGH);
	gpio_configure_pin(AVR32_PIN_PA14, GPIO_DIR_OUTPUT | GPIO_DRIVE_HIGH);
	gpio_configure_pin(AVR32_PIN_PA23, GPIO_DIR_OUTPUT | GPIO_DRIVE_HIGH);
	*/

	gpio_configure_pin(AVR32_PIN_PA20, GPIO_DIR_OUTPUT | GPIO_DRIVE_HIGH);
	gpio_configure_pin(AVR32_PIN_PA21, GPIO_DIR_OUTPUT | GPIO_DRIVE_HIGH);
	gpio_configure_pin(AVR32_PIN_PA22, GPIO_DIR_OUTPUT | GPIO_DRIVE_HIGH);
	gpio_configure_pin(AVR32_PIN_PA30, GPIO_DIR_OUTPUT | GPIO_DRIVE_HIGH);



}