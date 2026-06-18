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
/*=============================================================================
 *  MPU6050 - ACCELEROMETER
 *=============================================================================*/
#define MPU_WRITE         0xD0
#define MPU_READ          0xD1
#define REG_PWR_MGMT_1    0x6B
#define REG_ACCEL_CONFIG  0x1C

#define REG_ACCEL_XOUT_H  0x3B
#define REG_ACCEL_XOUT_L  0x3C
#define REG_ACCEL_YOUT_H  0x3D
#define REG_ACCEL_YOUT_L  0x3E
#define REG_ACCEL_ZOUT_H  0x3F
#define REG_ACCEL_ZOUT_L  0x40

#define AXIS_SIGN_X  1
#define AXIS_SIGN_Y  1

/*=============================================================================
 *  GESTURE TUNING
 *=============================================================================*/
#define RIGHT_THRESHOLD   8000
#define LEFT_THRESHOLD   -8000
#define FORWARD_THRESHOLD  8000
#define DEAD_ZONE         3000
#define CONFIRM_COUNT        3

#define CAL_SAMPLES    64
#define CAL_DELAY_MS    5
#define FILTER_SAMPLES   8
#define FILTER_DELAY_MS  5

/*=============================================================================
 *  STATES
 *=============================================================================*/
typedef enum {
    STATE_CENTER = 0,
    STATE_LEFT,
    STATE_RIGHT,
    STATE_FORWARD,
    STATE_UNKNOWN
} GloveState;

/*=============================================================================
 *  UART DRIVER
 *=============================================================================*/
void UART_Init(void)
{
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 1;
    SPBRG  = 129;
    TXSTAbits.BRGH = 1;
    TXSTAbits.SYNC = 0;
    TXSTAbits.TXEN = 1;
    RCSTAbits.SPEN = 1;
}

void UART_SendChar(unsigned char ch)
{
    while (!TXSTAbits.TRMT);
    TXREG = ch;
}

/*=============================================================================
 *  MPU6050 DRIVER
 *=============================================================================*/
void MPU6050_WriteReg(unsigned char reg, unsigned char val)
{
    I2C_Start();
    I2C_Write(MPU_WRITE);
    I2C_Write(reg);
    I2C_Write(val);
    I2C_Stop();
    __delay_ms(5);
}

signed int MPU6050_ReadAxis(unsigned char regH)
{
    unsigned char hi, lo;
    I2C_Start();
    I2C_Write(MPU_WRITE);
    I2C_Write(regH);
    I2C_RepeatedStart();
    I2C_Write(MPU_READ);
    hi = I2C_Read(1);
    lo = I2C_Read(0);
    I2C_Stop();
    return (signed int)(((unsigned int)hi << 8) | lo);
}

void MPU6050_Init(void)
{
    __delay_ms(150);
    MPU6050_WriteReg(REG_PWR_MGMT_1, 0x00);
    MPU6050_WriteReg(REG_ACCEL_CONFIG, 0x00);
}
/*=============================================================================
 *  CALIBRATION - both X and Y axes
 *=============================================================================*/
void Calibrate(signed long *baseX, signed long *baseY)
{
    unsigned char i;
    signed long   sumX = 0;
    signed long   sumY = 0;

    LCD_Clear();
    LCD_SetCursor(1, 0); LCD_Print("  CALIBRATING   ");
    LCD_SetCursor(2, 0); LCD_Print("  KEEP STILL..  ");

    for (i = 0; i < CAL_SAMPLES; i++)
    {
        sumX += MPU6050_ReadAxis(REG_ACCEL_XOUT_H);
        sumY += MPU6050_ReadAxis(REG_ACCEL_YOUT_H);
        __delay_ms(CAL_DELAY_MS);
    }
    *baseX = sumX / CAL_SAMPLES;
    *baseY = sumY / CAL_SAMPLES;
}

/*=============================================================================
 *  FILTERED ACCEL READ
 *=============================================================================*/
void GetFilteredAccel(signed int *outX, signed int *outY)
{
    unsigned char i;
    signed long   sumX = 0;
    signed long   sumY = 0;

    for (i = 0; i < FILTER_SAMPLES; i++)
    {
        sumX += MPU6050_ReadAxis(REG_ACCEL_XOUT_H);
        sumY += MPU6050_ReadAxis(REG_ACCEL_YOUT_H);
        __delay_ms(FILTER_DELAY_MS);
    }
    *outX = (signed int)(sumX / FILTER_SAMPLES);
    *outY = (signed int)(sumY / FILTER_SAMPLES);
}

/*=============================================================================
 *  STATE OUTPUT
 *=============================================================================*/
void ApplyState(GloveState state)
{
    LCD_SetCursor(1, 0); LCD_Print("  SMART GLOVE   ");
    LCD_ClearLine(2);

    switch (state)
    {
        case STATE_CENTER:
            TrafficLight_Set(0, 0, 1);
            UART_SendChar('C');
            LCD_SetCursor(2, 0); LCD_Print(" Hand: CENTER   ");
            break;
        case STATE_LEFT:
            TrafficLight_Set(0, 1, 0);
            UART_SendChar('L');
            LCD_SetCursor(2, 0); LCD_Print(" Hand: LEFT <<< ");
            break;
        case STATE_RIGHT:
            TrafficLight_Set(1, 0, 0);
            UART_SendChar('R');
            LCD_SetCursor(2, 0); LCD_Print(" Hand: RIGHT >>>");
            break;
        case STATE_FORWARD:
            TrafficLight_Set(1, 1, 1);
            UART_SendChar('F');
            LCD_SetCursor(2, 0); LCD_Print(" Hand: EXIT vvv ");
            break;
        default:
            break;
    }
}

/*=============================================================================
 *  MAIN
 *=============================================================================*/
void main(void)
{
    signed long   baselineX, baselineY;
    signed int    rawX, rawY;
    signed int    corrX, corrY;
    GloveState    currentState = STATE_UNKNOWN;
    GloveState    detectedState;
    GloveState    pendingState = STATE_UNKNOWN;
    unsigned char confirmCount = 0;

    ADCON1 = 0x07;
    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB1 = 0;
    TRISBbits.TRISB2 = 0;
    PORTB = 0x00;
    PORTC = 0x00;
    PORTD = 0x00;

    LCD_Init();
    TrafficLight_Init();
    I2C_Init();
    MPU6050_Init();
    UART_Init();

    LCD_SetCursor(1, 0); LCD_Print("  SMART GLOVE   ");
    LCD_SetCursor(2, 0); LCD_Print(" LED Self-Test  ");
    TrafficLight_SelfTest();

    Calibrate(&baselineX, &baselineY);

    LCD_Clear();
    LCD_SetCursor(1, 0); LCD_Print("  SMART GLOVE   ");
    LCD_SetCursor(2, 0); LCD_Print("   CAL DONE!    ");
    __delay_ms(800);

    ApplyState(STATE_CENTER);
    currentState = STATE_CENTER;

    while (1)
    {
        GetFilteredAccel(&rawX, &rawY);
        corrX = (signed int)(rawX - (signed int)baselineX) * AXIS_SIGN_X;
        corrY = (signed int)(rawY - (signed int)baselineY) * AXIS_SIGN_Y;

        if (corrX > RIGHT_THRESHOLD)
            detectedState = STATE_RIGHT;
        else if (corrX < LEFT_THRESHOLD)
            detectedState = STATE_LEFT;
        else if (corrY > FORWARD_THRESHOLD)
            detectedState = STATE_FORWARD;
        else if (corrX > -DEAD_ZONE && corrX < DEAD_ZONE &&
                 corrY > -DEAD_ZONE && corrY < DEAD_ZONE)
            detectedState = STATE_CENTER;
        else
            detectedState = currentState;

        if (detectedState == pendingState)
            confirmCount++;
        else
        {
            pendingState = detectedState;
            confirmCount = 1;
        }

        if (confirmCount >= CONFIRM_COUNT && pendingState != currentState)
        {
            currentState = pendingState;
            confirmCount = 0;
            ApplyState(currentState);
        }

        __delay_ms(50);
    }
}