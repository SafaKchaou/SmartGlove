/* Crystal frequency — tells the compiler we're using a 20 MHz clock
   so that __delay_ms() and __delay_us() calculate timing correctly */
#define _XTAL_FREQ 20000000
#include <xc.h>

/*=============================================================================
 *  CONFIG BITS — Hardware settings burned into the chip at programming time
 *=============================================================================*/
#pragma config FOSC  = HS       /* Use the external high-speed 20 MHz crystal */
#pragma config WDTE  = OFF      /* Watchdog timer off (we don't need auto-reset) */
#pragma config PWRTE = ON       /* Wait a bit at power-on for voltages to stabilize */
#pragma config BOREN = ON       /* Reset the chip if voltage drops too low */
#pragma config LVP   = OFF      /* Disable low-voltage programming (frees up RB3 pin) */
#pragma config CPD   = OFF      /* Don't protect EEPROM data */
#pragma config WRT   = OFF      /* Allow writing to flash memory normally */
#pragma config CP    = OFF      /* Don't protect program memory */

/*=============================================================================
 *  PIN DEFINITIONS — Friendly names for the physical pins we're using
 *=============================================================================*/

/* LCD is wired in 4-bit mode on PORTD
   We only need 6 pins: RS, Enable, and 4 data lines (D4-D7) */
#define RS   PORTDbits.RD0      /* Register Select: 0 = command, 1 = character */
#define E    PORTDbits.RD1      /* Enable: pulse this to tell LCD to read data */
#define D4   PORTDbits.RD4      /* LCD data bit 4 */
#define D5   PORTDbits.RD5      /* LCD data bit 5 */
#define D6   PORTDbits.RD6      /* LCD data bit 6 */
#define D7   PORTDbits.RD7      /* LCD data bit 7 */

/* Traffic light LEDs on PORTB — each LED shows a different hand state */
#define LED_RED     PORTBbits.RB0   /* Lights up when hand tilts RIGHT */
#define LED_YELLOW  PORTBbits.RB1   /* Lights up when hand tilts LEFT */
#define LED_GREEN   PORTBbits.RB2   /* Lights up when hand is CENTER */

/*=============================================================================
 *  LCD DRIVER — Functions to talk to the 16x2 character LCD
 *=============================================================================*/

/* Pulse the Enable pin — this is how the LCD knows to read the data lines */
void LCD_EnablePulse(void)
{
    E = 1; __delay_us(2);    /* Bring Enable high */
    E = 0; __delay_us(100);  /* Bring it low — LCD latches data on this falling edge */
}

/* Send 4 bits (one nibble) to the LCD data pins */
void LCD_SendNibble(unsigned char nibble)
{
    D4 = (nibble >> 0) & 1;  /* Extract bit 0 and send to D4 */
    D5 = (nibble >> 1) & 1;  /* Extract bit 1 and send to D5 */
    D6 = (nibble >> 2) & 1;  /* Extract bit 2 and send to D6 */
    D7 = (nibble >> 3) & 1;  /* Extract bit 3 and send to D7 */
    LCD_EnablePulse();        /* Tell LCD to read these 4 bits */
}

/* Send a command byte to the LCD (like "clear screen" or "move cursor") */
void LCD_Command(unsigned char cmd)
{
    RS = 0;                       /* RS=0 means this is a command, not a character */
    LCD_SendNibble(cmd >> 4);     /* Send the upper 4 bits first */
    LCD_SendNibble(cmd & 0x0F);   /* Then the lower 4 bits */
    __delay_ms(2);                /* Give the LCD time to process */
}

/* Send a character to display on the LCD */
void LCD_Data(unsigned char data)
{
    RS = 1;                       /* RS=1 means this is a character to display */
    LCD_SendNibble(data >> 4);    /* Upper nibble first */
    LCD_SendNibble(data & 0x0F);  /* Then lower nibble */
    __delay_us(50);               /* Short delay — characters are faster than commands */
}

/* Print a whole string to the LCD, one character at a time */
void LCD_Print(const char *str)
{
    while (*str)
        LCD_Data((unsigned char)*str++);
}

/* Move the cursor to a specific row and column (row 1 or 2, col 0-15) */
void LCD_SetCursor(unsigned char row, unsigned char col)
{
    /* Row 1 starts at address 0x80, Row 2 starts at 0xC0 */
    LCD_Command(row == 1 ? 0x80 + col : 0xC0 + col);
}

/* Clear the entire LCD screen */
void LCD_Clear(void)
{
    LCD_Command(0x01);   /* Clear display command */
    __delay_ms(2);       /* This command takes a bit longer */
}

/* Clear just one row by writing 16 spaces over it */
void LCD_ClearLine(unsigned char row)
{
    unsigned char i;
    LCD_SetCursor(row, 0);
    for (i = 0; i < 16; i++) LCD_Data(' ');  /* Overwrite with spaces */
    LCD_SetCursor(row, 0);                    /* Move cursor back to start of the row */
}

/* Initialize the LCD — must be called once at startup
   Follows the standard HD44780 initialization sequence for 4-bit mode */
void LCD_Init(void)
{
    /* Set LCD pins as outputs */
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    TRISDbits.TRISD4 = 0;
    TRISDbits.TRISD5 = 0;
    TRISDbits.TRISD6 = 0;
    TRISDbits.TRISD7 = 0;
    PORTD  = 0x00;       /* Start with all pins low */
    ADCON1 = 0x07;       /* Make all analog pins digital (important for PORTD) */
    __delay_ms(50);      /* Wait for LCD to power up */

    /* The LCD starts in an unknown state — this wake-up sequence
       forces it into a known 8-bit mode, then switches to 4-bit */
    RS = 0;
    LCD_SendNibble(0x03); __delay_ms(5);     /* First wake-up call */
    LCD_SendNibble(0x03); __delay_us(150);   /* Second wake-up call */
    LCD_SendNibble(0x03); __delay_us(150);   /* Third wake-up call */
    LCD_SendNibble(0x02); __delay_ms(2);     /* NOW switch to 4-bit mode */

    LCD_Command(0x28);   /* 4-bit mode, 2 lines, 5x8 font */
    LCD_Command(0x0C);   /* Display on, cursor off, no blinking */
    LCD_Command(0x01); __delay_ms(2);  /* Clear screen */
    LCD_Command(0x06);   /* Cursor moves right after each character */
}

/*=============================================================================
 *  TRAFFIC LIGHT DRIVER — Controls the 3 LEDs that show hand direction
 *=============================================================================*/

/* Set the 3 LED pins as outputs and turn them all off */
void TrafficLight_Init(void)
{
    TRISBbits.TRISB0 = 0;   /* Red LED pin = output */
    TRISBbits.TRISB1 = 0;   /* Yellow LED pin = output */
    TRISBbits.TRISB2 = 0;   /* Green LED pin = output */
    LED_RED    = 0;
    LED_YELLOW = 0;
    LED_GREEN  = 0;
}

/* Turn on specific LEDs — pass 1 to turn on, 0 to turn off */
void TrafficLight_Set(unsigned char red, unsigned char yellow, unsigned char green)
{
    /* Turn everything off first to avoid brief flicker of wrong color */
    LED_RED    = 0;
    LED_YELLOW = 0;
    LED_GREEN  = 0;
    /* Now turn on the requested ones */
    LED_RED    = red;
    LED_YELLOW = yellow;
    LED_GREEN  = green;
}

/* Blink each LED once at startup to prove they all work */
void TrafficLight_SelfTest(void)
{
    LED_RED    = 1; __delay_ms(350); LED_RED    = 0;
    LED_YELLOW = 1; __delay_ms(350); LED_YELLOW = 0;
    LED_GREEN  = 1; __delay_ms(350); LED_GREEN  = 0;
}

/*=============================================================================
 *  I2C DRIVER — Bit-level communication with the MPU6050 sensor
 *  I2C is a 2-wire protocol: SDA (data) on RC4, SCL (clock) on RC3
 *=============================================================================*/

/* Wait until the I2C hardware is done with the previous operation.
   The timeout prevents the program from hanging if something goes wrong. */
void I2C_Wait(void)
{
    unsigned int timeout = 10000;
    while (((SSPCON2 & 0x1F) || SSPSTATbits.R_W) && timeout)
        timeout--;
}

/* Set up the I2C module as a master running at 100 kHz */
void I2C_Init(void)
{
    TRISCbits.TRISC3 = 1;   /* SCL pin must be set as input (I2C hardware controls it) */
    TRISCbits.TRISC4 = 1;   /* SDA pin must be set as input (I2C hardware controls it) */
    SSPCON  = 0x28;          /* Enable I2C master mode */
    SSPCON2 = 0x00;          /* Clear all control bits */
    SSPSTAT = 0x00;          /* Clear status */
    SSPADD  = 49;            /* Clock speed: 20MHz / (4 * (49+1)) = 100 kHz */
}

/* I2C bus signals — these are the building blocks of every I2C transaction */
void I2C_Start(void)         { I2C_Wait(); SSPCON2bits.SEN  = 1; I2C_Wait(); }  /* "Hey sensor, I want to talk" */
void I2C_RepeatedStart(void) { I2C_Wait(); SSPCON2bits.RSEN = 1; I2C_Wait(); }  /* "Wait, I want to switch direction" */
void I2C_Stop(void)          { I2C_Wait(); SSPCON2bits.PEN  = 1; I2C_Wait(); }  /* "OK, I'm done talking" */

/* Send one byte over I2C */
void I2C_Write(unsigned char data)
{
    I2C_Wait();
    SSPBUF = data;    /* Load the byte into the buffer — hardware sends it automatically */
    I2C_Wait();
}

/* Receive one byte over I2C.
   sendAck=1 means "I want more bytes after this",
   sendAck=0 means "this is the last byte I need" */
unsigned char I2C_Read(unsigned char sendAck)
{
    unsigned char val;
    I2C_Wait();
    SSPCON2bits.RCEN = 1;     /* Tell hardware to receive one byte */
    I2C_Wait();
    val = SSPBUF;              /* Read the received byte */
    I2C_Wait();
    SSPCON2bits.ACKDT = sendAck ? 0 : 1;  /* 0=ACK (send more), 1=NACK (stop) */
    SSPCON2bits.ACKEN = 1;     /* Send the ACK/NACK signal */
    return val;
}

/*=============================================================================
 *  MPU6050 ACCELEROMETER — I2C addresses and register map
 *=============================================================================*/

/* I2C device addresses (AD0 pin tied to GND gives address 0x68,
   shifted left by 1 for the R/W bit: write=0xD0, read=0xD1) */
#define MPU_WRITE         0xD0
#define MPU_READ          0xD1

/* Internal registers we need to configure or read from */
#define REG_PWR_MGMT_1    0x6B  /* Power management — write 0x00 to wake up the sensor */
#define REG_ACCEL_CONFIG  0x1C  /* Accelerometer range — 0x00 = most sensitive (±2g) */

/* Accelerometer output registers — raw 16-bit readings for each axis */
#define REG_ACCEL_XOUT_H  0x3B  /* X-axis high byte */
#define REG_ACCEL_XOUT_L  0x3C  /* X-axis low byte */
#define REG_ACCEL_YOUT_H  0x3D  /* Y-axis high byte */
#define REG_ACCEL_YOUT_L  0x3E  /* Y-axis low byte */
#define REG_ACCEL_ZOUT_H  0x3F  /* Z-axis high byte (not used in this project) */
#define REG_ACCEL_ZOUT_L  0x40  /* Z-axis low byte (not used in this project) */

/* Sign multipliers — set to -1 to flip a direction if wiring is reversed */
#define AXIS_SIGN_X  1   /* X-axis: positive = RIGHT, negative = LEFT */
#define AXIS_SIGN_Y  1   /* Y-axis: positive = FORWARD (tilt fingers down) */

/*=============================================================================
 *  GESTURE TUNING — Thresholds that decide when a tilt counts as a gesture
 *  The MPU6050 in ±2g mode outputs ~16384 per 1g of acceleration.
 *  8000 is roughly half of gravity — a solid tilt, not too sensitive.
 *=============================================================================*/
#define RIGHT_THRESHOLD    8000   /* X must exceed this to count as RIGHT */
#define LEFT_THRESHOLD    -8000   /* X must go below this to count as LEFT */
#define FORWARD_THRESHOLD  8000   /* Y must exceed this to count as FORWARD */
#define DEAD_ZONE          3000   /* Values between -3000 and +3000 = hand is flat */
#define CONFIRM_COUNT         3   /* Must detect the same state 3 times in a row to switch */

/* Calibration: how many samples to average when finding the "flat" baseline */
#define CAL_SAMPLES    64   /* Take 64 readings and average them at startup */
#define CAL_DELAY_MS    5   /* 5ms pause between each calibration sample */

/* Filtering: averaging smooths out sensor noise during normal operation */
#define FILTER_SAMPLES   8   /* Average 8 readings per measurement cycle */
#define FILTER_DELAY_MS  5   /* 5ms between each filter sample */

/*=============================================================================
 *  STATES — All possible hand positions the glove can detect
 *=============================================================================*/
typedef enum {
    STATE_CENTER = 0,   /* Hand flat — no action */
    STATE_LEFT,         /* Hand tilted left — previous slide/photo */
    STATE_RIGHT,        /* Hand tilted right — next slide/photo */
    STATE_FORWARD,      /* Hand tilted forward (fingers down) — exit/escape */
    STATE_UNKNOWN       /* Startup state before first detection */
} GloveState;

/*=============================================================================
 *  UART DRIVER — Sends single characters to the ESP32-CAM over a wire
 *  TX pin: RC6 (Pin 25) → goes through a voltage divider → ESP32 GPIO 13
 *  Settings: 9600 baud, 8 data bits, no parity, 1 stop bit (8N1)
 *=============================================================================*/
void UART_Init(void)
{
    TRISCbits.TRISC6 = 0;   /* TX pin as output (we're sending data) */
    TRISCbits.TRISC7 = 1;   /* RX pin as input (not used, but good practice) */

    /* Baud rate formula with BRGH=1: SPBRG = (20MHz / (16 * 9600)) - 1 = 129 */
    SPBRG  = 129;
    TXSTAbits.BRGH = 1;     /* High-speed mode for more accurate baud rate */
    TXSTAbits.SYNC = 0;     /* Asynchronous mode (standard UART) */
    TXSTAbits.TXEN = 1;     /* Turn on the transmitter */
    RCSTAbits.SPEN = 1;     /* Enable the serial port hardware */
}

/* Send one character over UART to the ESP32 */
void UART_SendChar(unsigned char ch)
{
    while (!TXSTAbits.TRMT);  /* Wait until any previous byte has finished sending */
    TXREG = ch;               /* Load the character — hardware sends it automatically */
}

/*=============================================================================
 *  MPU6050 DRIVER — Higher-level functions to configure and read the sensor
 *=============================================================================*/

/* Write a value to one of the MPU6050's internal registers */
void MPU6050_WriteReg(unsigned char reg, unsigned char val)
{
    I2C_Start();
    I2C_Write(MPU_WRITE);   /* Address the sensor in write mode */
    I2C_Write(reg);          /* Tell it which register we want to write to */
    I2C_Write(val);          /* Send the value */
    I2C_Stop();
    __delay_ms(5);           /* Small delay for the sensor to process */
}

/* Read a 16-bit value from one accelerometer axis.
   regH is the high-byte register address — the low byte is always the next register */
signed int MPU6050_ReadAxis(unsigned char regH)
{
    unsigned char hi, lo;

    I2C_Start();
    I2C_Write(MPU_WRITE);    /* Tell sensor we want to set the read pointer */
    I2C_Write(regH);          /* Point to the high byte register */
    I2C_RepeatedStart();      /* Switch to read mode without releasing the bus */
    I2C_Write(MPU_READ);     /* Now read from the sensor */
    hi = I2C_Read(1);         /* Read high byte (ACK = more coming) */
    lo = I2C_Read(0);         /* Read low byte (NACK = we're done) */
    I2C_Stop();

    /* Combine the two bytes into a signed 16-bit value (-32768 to +32767) */
    return (signed int)(((unsigned int)hi << 8) | lo);
}

/* Wake up the MPU6050 and set it to most sensitive accelerometer range */
void MPU6050_Init(void)
{
    __delay_ms(150);                            /* Let the sensor boot up */
    MPU6050_WriteReg(REG_PWR_MGMT_1, 0x00);    /* Clear sleep bit — sensor wakes up */
    MPU6050_WriteReg(REG_ACCEL_CONFIG, 0x00);   /* ±2g range — maximum sensitivity */
}

/*=============================================================================
 *  CALIBRATION — Finds the "neutral" reading when hand is held flat
 *  We average many samples so small vibrations don't throw off the baseline.
 *  This baseline gets subtracted from all future readings.
 *=============================================================================*/
void Calibrate(signed long *baseX, signed long *baseY)
{
    unsigned char i;
    signed long   sumX = 0;
    signed long   sumY = 0;

    /* Show calibration message on LCD */
    LCD_Clear();
    LCD_SetCursor(1, 0); LCD_Print("  CALIBRATING   ");
    LCD_SetCursor(2, 0); LCD_Print("  KEEP STILL..  ");

    /* Take 64 samples from both axes and sum them up */
    for (i = 0; i < CAL_SAMPLES; i++)
    {
        sumX += MPU6050_ReadAxis(REG_ACCEL_XOUT_H);
        sumY += MPU6050_ReadAxis(REG_ACCEL_YOUT_H);
        __delay_ms(CAL_DELAY_MS);
    }

    /* Return the average as the baseline for each axis */
    *baseX = sumX / CAL_SAMPLES;
    *baseY = sumY / CAL_SAMPLES;
}

/*=============================================================================
 *  FILTERED ACCEL READ — Smooths out sensor noise during normal operation
 *  Instead of reading the sensor once (which can be noisy), we take 8
 *  readings and average them for a much cleaner, more stable value.
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
 *  STATE OUTPUT — When the hand state changes, update everything:
 *   1. LCD shows the current gesture
 *   2. Traffic light LEDs show the direction visually
 *   3. UART sends a command character to the ESP32 for WiFi forwarding
 *
 *  Traffic light mapping:
 *    CENTER  = Green only
 *    LEFT    = Yellow only
 *    RIGHT   = Red only
 *    FORWARD = All three on (clearly different from the others)
 *=============================================================================*/
void ApplyState(GloveState state)
{
    /* Always show project name on line 1 */
    LCD_SetCursor(1, 0); LCD_Print("  SMART GLOVE   ");
    LCD_ClearLine(2);    /* Clear line 2 before writing new state */

    switch (state)
    {
        case STATE_CENTER:
            TrafficLight_Set(0, 0, 1);              /* Green LED on */
            UART_SendChar('C');                      /* Send 'C' to ESP32 */
            LCD_SetCursor(2, 0); LCD_Print(" Hand: CENTER   ");
            break;
        case STATE_LEFT:
            TrafficLight_Set(0, 1, 0);              /* Yellow LED on */
            UART_SendChar('L');                      /* Send 'L' to ESP32 */
            LCD_SetCursor(2, 0); LCD_Print(" Hand: LEFT <<< ");
            break;
        case STATE_RIGHT:
            TrafficLight_Set(1, 0, 0);              /* Red LED on */
            UART_SendChar('R');                      /* Send 'R' to ESP32 */
            LCD_SetCursor(2, 0); LCD_Print(" Hand: RIGHT >>>");
            break;
        case STATE_FORWARD:
            TrafficLight_Set(1, 1, 1);              /* All three LEDs on */
            UART_SendChar('F');                      /* Send 'F' to ESP32 */
            LCD_SetCursor(2, 0); LCD_Print(" Hand: EXIT vvv ");
            break;
        default:
            break;
    }
}

/*=============================================================================
 *  MAIN — Program entry point. Sets up all hardware, calibrates, then runs
 *  the infinite loop that reads the sensor and reacts to hand movements.
 *=============================================================================*/
void main(void)
{
    /* Variables for sensor data and state tracking */
    signed long   baselineX, baselineY;    /* "Flat hand" reference values */
    signed int    rawX, rawY;              /* Fresh sensor readings */
    signed int    corrX, corrY;            /* Readings with baseline subtracted */
    GloveState    currentState = STATE_UNKNOWN;   /* What state we're currently in */
    GloveState    detectedState;                   /* What the sensor sees right now */
    GloveState    pendingState = STATE_UNKNOWN;    /* Candidate for next state change */
    unsigned char confirmCount = 0;                /* How many times we've seen pendingState in a row */

    /* Make all analog pins digital — prevents interference with PORTB/PORTD */
    ADCON1 = 0x07;

    /* Set traffic light pins as outputs */
    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB1 = 0;
    TRISBbits.TRISB2 = 0;

    /* Start with all ports low */
    PORTB = 0x00;
    PORTC = 0x00;
    PORTD = 0x00;

    /* Initialize all peripherals */
    LCD_Init();
    TrafficLight_Init();
    I2C_Init();
    MPU6050_Init();
    UART_Init();

    /* Show startup message and blink each LED to confirm they work */
    LCD_SetCursor(1, 0); LCD_Print("  SMART GLOVE   ");
    LCD_SetCursor(2, 0); LCD_Print(" LED Self-Test  ");
    TrafficLight_SelfTest();

    /* Calibrate: user holds hand flat while we record the baseline */
    Calibrate(&baselineX, &baselineY);

    /* Show that calibration is done */
    LCD_Clear();
    LCD_SetCursor(1, 0); LCD_Print("  SMART GLOVE   ");
    LCD_SetCursor(2, 0); LCD_Print("   CAL DONE!    ");
    __delay_ms(800);

    /* Start in CENTER state */
    ApplyState(STATE_CENTER);
    currentState = STATE_CENTER;

    /*=========================================================================
     *  MAIN LOOP — Runs forever, reading the sensor ~20 times per second
     *=========================================================================*/
    while (1)
    {
        /* Step 1: Read the accelerometer (averaged over 8 samples for stability) */
        GetFilteredAccel(&rawX, &rawY);

        /* Step 2: Subtract the baseline so "flat hand" reads as ~0
           and multiply by the sign constant in case we need to flip direction */
        corrX = (signed int)(rawX - (signed int)baselineX) * AXIS_SIGN_X;
        corrY = (signed int)(rawY - (signed int)baselineY) * AXIS_SIGN_Y;

        /* Step 3: Classify the tilt into a state.
           X-axis (left/right) has priority — Y-axis (forward) only checked
           when X is near center, to prevent conflicts during diagonal tilts */
        if (corrX > RIGHT_THRESHOLD)
            detectedState = STATE_RIGHT;       /* Strong tilt to the right */
        else if (corrX < LEFT_THRESHOLD)
            detectedState = STATE_LEFT;        /* Strong tilt to the left */
        else if (corrY > FORWARD_THRESHOLD)
            detectedState = STATE_FORWARD;     /* Fingers pointing down */
        else if (corrX > -DEAD_ZONE && corrX < DEAD_ZONE &&
                 corrY > -DEAD_ZONE && corrY < DEAD_ZONE)
            detectedState = STATE_CENTER;      /* Hand is flat (both axes near zero) */
        else
            detectedState = currentState;      /* In the gap — keep current to avoid flickering */

        /* Step 4: Debounce — require the same state 3 times in a row
           before switching, to prevent accidental triggers from noise */
        if (detectedState == pendingState)
            confirmCount++;   /* Same state seen again — one step closer */
        else
        {
            /* Different state — restart the confirmation count */
            pendingState = detectedState;
            confirmCount = 1;
        }

        /* Step 5: If confirmed enough times AND it's a new state,
           update the LCD + LEDs + send UART command to ESP32 */
        if (confirmCount >= CONFIRM_COUNT && pendingState != currentState)
        {
            currentState = pendingState;
            confirmCount = 0;
            ApplyState(currentState);
        }

        __delay_ms(50);   /* Small pause before next reading (~20 loops per second) */
    }
}