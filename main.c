/*----------------------------------------------------------------------*/
/* 赤外線リモコン制御モジュール サンプルコード                          */
/*----------------------------------------------------------------------*/


#define __PROG_TYPES_COMPAT__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>
#include <ctype.h>
#include "uart.h"
#include "xitoa.h"
#include "ir_ctrl.h"
#include "util.h"


/*--------------------------------------------------------------------------*/
/* 受信フレームの表示                                                       */

static
void put_rcvr (void)
{
	uint8_t d, i, l;
	uint16_t a;


	l = IrCtrl.len;
	switch (IrCtrl.fmt) {	/* Which frame arrived? */
#if IR_USE_NEC
	case NEC:	/* NEC format data frame */
		if (l == 32) {	/* Only 32-bit frame is valid */
			xprintf(PSTR("N 0x%02X 0x%02X 0x%02X 0x%02X\n"), IrCtrl.rxdata[0], IrCtrl.rxdata[1], IrCtrl.rxdata[2], IrCtrl.rxdata[3]);
		}
		break;

	case NEC|REPT:	/* NEC repeat frame */
		xputs(PSTR("N repeat\n"));
		break;
#endif
#if IR_USE_AEHA
	case AEHA:		/* AEHA format data frame */
		if ((l >= 48) && (l % 8 == 0)) {	/* Only multiple of 8 bit frame is valid */
			xputc('A');
			l /= 8;
			for (i = 0; i < l; i++) xprintf(PSTR(" 0x%02X"), IrCtrl.rxdata[i]);
			xputc('\n');
		}
		break;

	case AEHA|REPT:	/* AEHA format repeat frame */
		xputs(PSTR("A repeat\n"));
		break;
#endif
#if IR_USE_SONY
	case SONY:
		d = IrCtrl.rxdata[0];
		a = ((uint16_t)IrCtrl.rxdata[2] << 9) + ((uint16_t)IrCtrl.rxdata[1] << 1) + ((d & 0x80) ? 1 : 0);
		d &= 0x7F;
		switch (l) {	/* Only 12, 15 or 20 bit frames are valid */
		case 12:
			xprintf(PSTR("S12 %u %u\n"), d, a & 0x1F);
			break;
		case 15:
			xprintf(PSTR("S15 %u %u\n"), d, a & 0xFF);
			break;
		case 20:
			xprintf(PSTR("S20 %u %u\n"), d, a & 0x1FFF);
			break;
		}
		break;
#endif
	}

	IrCtrl.stat = IR_IDLE;		/* Prepare to receive next frame */
}



/*--------------------------------------------------------------------------*/
/* モニタ                                                                   */


/* 1行入力 */
static
void get_line (char* buff, int len)
{
	char c;
	int idx = 0;


	xputc('>');
	for (;;) {
		if (IrCtrl.stat == IR_RECVED) put_rcvr();	/* 受信フレームの処理 */
		if (!uart_test()) continue;
		c = uart_get();
		if (c == '\r') break;
		if ((c == '\b') && idx) {
			idx--; xputc(c);
		}
		if (((uint8_t)c >= ' ') && (idx < len - 1)) {
			buff[idx++] = c; xputc(c);
		}
	}
	buff[idx] = 0;
	xputc('\n');
}




/*-----------------------------------------------------------------------*/
/* Main                                                                  */
/*-----------------------------------------------------------------------*/

int main (void)
{
	char line[64], *p;
	long val;
	uint8_t n;

	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Enable External Clock
	CLKSEL0 |= (1 << EXTE);// CKSEL0.EXTE = 1;// Enable_external_clock();
	while ( (CLKSTA & (1 << EXTON)) == 0 ){} // while (CLKSTA.EXTON != 1);// while (!External_clock_ready());
	CLKSEL0 |= (1 << CLKS);// CLKSEL0.CLKS = 1;//Select_external_clock();
	PLLCSR |= (1 << PLLE);// PLLCSR.PLLE = 1;// Enable_pll();
	CLKSEL0 &= ~(1 << RCE);// CLKSEL0.RCE = 0;// Disable_RC_clock();
	while ( (PLLCSR & (1 << PLOCK)) == 0){}// while (PLLCSR.PLOCK != 1);// while (!Pll_ready());
	USBCON &= ~(1 << FRZCLK);// USBCON.FRZCLK = 0;// Usb_unfreeze_clock();

	clock_prescale_set(clock_div_1);
	set_bit(DDRC, 7);
	set_bit(PORTC, 7);
	clear_bit(DDRD, 4);

#if 0
	/* ポート初期化 */
	PORTB = 0b00100111; 	/* PortB, IR input */ //PB0:ICP1,PB1:OC1A,PB2:OC1B
	DDRB  = 0b00000000;
	PORTD = 0b11011111;		/* PortD, IR drive, Comm */ // PD0:RXD, PD1:TXD
	DDRD  = 0b00100010;
	PORTC = 0b00111111;		/* PortC */
	DDRC  = 0b00000000;
#endif

	uart_init();			/* Initialize UART driver */
	xfunc_out = (void(*)(char))uart_put;	/* Join xitoa module to communication module */

	/* 赤外線機能の初期化 */
	IR_initialize();

	sei();

	xputs(PSTR("IR remote control test program\n"));

	clear_bit(PORTC, 7);

	/* ユーザコマンド処理ループ */
	for (;;) {
		get_line(line, sizeof line);
		p = line;
		switch (tolower(*p++)) {
#if IR_USE_NEC
		case 'n' :	/* NECフォーマットで送信 >N [byte1 byte2 byte3 byte4] */
			n = 0;
			while (xatoi(&p, &val)) line[n++] = (uint8_t)val;
			if (IR_xmit(NEC, line, n * 8)) xputs(PSTR("OK\n"));
			break;
#endif
#if IR_USE_AEHA
		case 'a' :	/* 家製協フォーマットで送信 >A [byte1 [byte2 [byte3]]]... */
			n = 0;
			while (xatoi(&p, &val)) line[n++] = (uint8_t)val;
			if (IR_xmit(AEHA, line, n * 8)) xputs(PSTR("OK\n"));
			break;
#endif
#if IR_USE_SONY
		case 's' :	/* SONYフォーマットで送信 >S length command address */
			if (!xatoi(&p, &val)) break;	/* Length (12, 15 or 20) */
			n = (uint8_t)val;
			if (!xatoi(&p, &val)) break;	/* Command */
			line[0] = (uint8_t)val & 0x7F;
			if (!xatoi(&p, &val)) break;	/* Address */
			line[0] |= (val & 1) ? 0x80 : 0;
			line[1] = (uint8_t)(val >> 1);
			line[2] = (uint8_t)(val >> 9);
			if (IR_xmit(SONY, line, n)) xputs(PSTR("OK\n"));
#endif
		}

	}
}


