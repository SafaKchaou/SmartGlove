#define _XTAL_FREQ 20000000
#include <xc.h>

/*=============================================================================
 *  CONFIG BITS
 *=============================================================================*/
#pragma config FOSC  = HS
#pragma config WDTE  = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP   = OFF
#pragma config CPD   = OFF
#pragma config WRT   = OFF
#pragma config CP    = OFF

/*=============================================================================
 *  PIN DEFINITIONS
 *=============================================================================*/
/* LCD - 4-bit mode on PORTD */
#define RS   PORTDbits.RD0
#define E    PORTDbits.RD1
#define D4   PORTDbits.RD4
#define D5   PORTDbits.RD5
#define D6   PORTDbits.RD6
#define D7   PORTDbits.RD7

/* Traffic light LEDs on PORTB */
#define LED_RED     PORTBbits.RB0
#define LED_YELLOW  PORTBbits.RB1
#define LED_GREEN   PORTBbits.RB2

/*=============================================================================
 *  LCD DRIVER
 *=============================================================================*/
void LCD_EnablePulse(void)
{
    E = 1; __delay_us(2);
    E = 0; __delay_us(100);
}

void LCD_SendNibble(unsigned char nibble)
{
    D4 = (nibble >> 0) & 1;
    D5 = (nibble >> 1) & 1;
    D6 = (nibble >> 2) & 1;
    D7 = (nibble >> 3) & 1;
    LCD_EnablePulse();
}

void LCD_Command(unsigned char cmd)
{
    RS = 0;
    LCD_SendNibble(cmd >> 4);
    LCD_SendNibble(cmd & 0x0F);
    __delay_ms(2);
}

void LCD_Data(unsigned char data)
{
    RS = 1;
    LCD_SendNibble(data >> 4);
    LCD_SendNibble(data & 0x0F);
    __delay_us(50);
}

void LCD_Print(const char *str)
{
    while (*str)
        LCD_Data((unsigned char)*str++);
}

void LCD_SetCursor(unsigned char row, unsigned char col)
{
    LCD_Command(row == 1 ? 0x80 + col : 0xC0 + col);
}

void LCD_Clear(void)
{
    LCD_Command(0x01);
    __delay_ms(2);
}

void LCD_ClearLine(unsigned char row)
{
    unsigned char i;
    LCD_SetCursor(row, 0);
    for (i = 0; i < 16; i++) LCD_Data(' ');
    LCD_SetCursor(row, 0);
}

void LCD_Init(void)
{
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    TRISDbits.TRISD4 = 0;
    TRISDbits.TRISD5 = 0;
    TRISDbits.TRISD6 = 0;
    TRISDbits.TRISD7 = 0;
    PORTD  = 0x00;
    ADCON1 = 0x07;
    __delay_ms(50);

    RS = 0;
    LCD_SendNibble(0x03); __delay_ms(5);
    LCD_SendNibble(0x03); __delay_us(150);
    LCD_SendNibble(0x03); __delay_us(150);
    LCD_SendNibble(0x02); __delay_ms(2);

    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x01); __delay_ms(2);
    LCD_Command(0x06);
}
/*=============================================================================
 *  TRAFFIC LIGHT DRIVER
 *=============================================================================*/
void TrafficLight_Init(void)
{
    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB1 = 0;
    TRISBbits.TRISB2 = 0;
    LED_RED    = 0;
    LED_YELLOW = 0;
    LED_GREEN  = 0;
}

void TrafficLight_Set(unsigned char red, unsigned char yellow, unsigned char green)
{
    LED_RED    = 0;
    LED_YELLOW = 0;
    LED_GREEN  = 0;
    LED_RED    = red;
    LED_YELLOW = yellow;
    LED_GREEN  = green;
}

void TrafficLight_SelfTest(void)
{
    LED_RED    = 1; __delay_ms(350); LED_RED    = 0;
    LED_YELLOW = 1; __delay_ms(350); LED_YELLOW = 0;
    LED_GREEN  = 1; __delay_ms(350); LED_GREEN  = 0;
}

/*=============================================================================
 *  I2C DRIVER
 *=============================================================================*/
void I2C_Wait(void)
{
    unsigned int timeout = 10000;
    while (((SSPCON2 & 0x1F) || SSPSTATbits.R_W) && timeout)
        timeout--;
}

void I2C_Init(void)
{
    TRISCbits.TRISC3 = 1;
    TRISCbits.TRISC4 = 1;
    SSPCON  = 0x28;
    SSPCON2 = 0x00;
    SSPSTAT = 0x00;
    SSPADD  = 49;
}

void I2C_Start(void)         { I2C_Wait(); SSPCON2bits.SEN  = 1; I2C_Wait(); }
void I2C_RepeatedStart(void) { I2C_Wait(); SSPCON2bits.RSEN = 1; I2C_Wait(); }
void I2C_Stop(void)          { I2C_Wait(); SSPCON2bits.PEN  = 1; I2C_Wait(); }

void I2C_Write(unsigned char data)
{
    I2C_Wait();
    SSPBUF = data;
    I2C_Wait();
}

unsigned char I2C_Read(unsigned char sendAck)
{
    unsigned char val;
    I2C_Wait();
    SSPCON2bits.RCEN  = 1;
    I2C_Wait();
    val = SSPBUF;
    I2C_Wait();
    SSPCON2bits.ACKDT = sendAck ? 0 : 1;
    SSPCON2bits.ACKEN = 1;
    return val;
}