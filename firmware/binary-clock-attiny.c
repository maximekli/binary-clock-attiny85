#define F_CPU 1000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define NB_ROWS 4
#define NB_COLS 4

#define DATA_PIN PB0
#define LATCH_PIN PB1
#define CLK_PIN PB2

#define BUTTON_HOURS_PIN PB3
#define BUTTON_MINUTES_PIN PB4

uint8_t rowPins[NB_ROWS] = {1, 3, 5, 7};
uint8_t colPins[NB_COLS] = {2, 4, 6, 0};

volatile uint16_t milliseconds = 0;
volatile uint8_t min0 = 0;
volatile uint8_t min1 = 0;
volatile uint8_t h0 = 0;
volatile uint8_t h1 = 0;

volatile uint8_t matrixData[NB_ROWS] = {0};

volatile uint8_t displayedRowId = 0;
volatile uint8_t registerData[NB_ROWS] = {0};

volatile uint8_t buttonHoursPrevReading = 1;
volatile uint8_t buttonHoursWasPressed = 0;

volatile uint8_t buttonMinutesPrevReading = 1;
volatile uint8_t buttonMinutesWasPressed = 0;

static inline void initPins()
{
    // clearing registers
    DDRB = 0;
    PORTB = 0;

    // setting shift register pins as outputs, leaving button pins as inputs
    DDRB |= (1 << DATA_PIN) | (1 << LATCH_PIN) | (1 << CLK_PIN);

    // enabling internal pull-up resistors for the button pins
    PORTB |= (1 << BUTTON_HOURS_PIN) | (1 << BUTTON_MINUTES_PIN);
}

static inline void initTimer0()
{
    // clearing registers
    TCCR0A = 0;
    TCCR0B = 0;
    OCR0A = 0;

    // setting prescaler to 8
    TCCR0B |= (1 << CS01);

    // setting clear timer on compare match (CTC)
    TCCR0A |= (1 << WGM01);

    // setting output compare to 249
    OCR0A = 249;

    // clearing clock
    TCNT0 = 0;
}

static inline void initTimer0Interrupt()
{
    // clearing register
    TIMSK = 0;

    // timer 0
    TIMSK |= (1 << OCIE0A);
}

static inline void addHour()
{
    // adding 1 hour, updating the rest if necessary

    h0++;
    if (h0 == 10)
    {
        h0 = 0;
        h1++;
    }
    else if (h1 == 2 && h0 == 4)
    {
        h0 = 0;
        h1 = 0;
    }
}

static inline void addMinute()
{
    // adding 1 minute, updating the rest if necessary

    min0++;
    if (min0 == 10)
    {
        min0 = 0;
        min1++;
        if (min1 == 6) min1 = 0;
    }
}

static inline void updateMatrixData()
{
    // updating the matrix to be displayed on the led matrix with the current time

    uint8_t rowId;
    for (rowId = 0; rowId < NB_ROWS; rowId++)
    {
        matrixData[rowId] = 0;

        uint8_t bitPos = (NB_ROWS - 1) - rowId; // going through each number in reverse because MSB will be associated with row 0

        matrixData[rowId] |= (((h1 & (1 << bitPos)) >> bitPos) << 3);
        matrixData[rowId] |= (((h0 & (1 << bitPos)) >> bitPos) << 2);
        matrixData[rowId] |= (((min1 & (1 << bitPos)) >> bitPos) << 1);
        matrixData[rowId] |= (((min0 & (1 << bitPos)) >> bitPos) << 0);
    }
}

static inline void updateRegisterData()
{
    // converting matrix data to the 8-bit words to send to the register

    uint8_t rowId;
    for (rowId = 0; rowId < NB_ROWS; rowId++)
    {
        registerData[rowId] = 0;

        // row bits
        registerData[rowId] |= (1 << rowPins[rowId]);

        // columns bits
        uint8_t invRowData = ~matrixData[rowId]; // inverting row data because an on LED means a LOW column

        uint8_t colId;
        for (colId = 0; colId < NB_COLS; colId++)
        {
            uint8_t bitPos = (NB_COLS - 1) - colId; // going through rowData in reverse because MSB will be associated with col 0
            registerData[rowId] |= (((invRowData & (1 << bitPos)) >> bitPos) << colPins[colId]);
        }
    }
}

static inline void displayRow()
{
    // sending the word for the current displayed row to the register and displaying it

    PORTB &= ~(1 << LATCH_PIN);

    // shift out, MSB first
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        uint8_t bitPos = 7 - i; // going MSB first
        if ((registerData[displayedRowId] & (1 << bitPos)) >> bitPos)
        {
            PORTB |= (1 << DATA_PIN);
        }
        else
        {
            PORTB &= ~(1 << DATA_PIN);
        }

        PORTB |= (1 << CLK_PIN);
        PORTB &= ~(1 << CLK_PIN);
    }

    PORTB |= (1 << LATCH_PIN);
}

ISR(TIMER0_COMPA_vect)
{
    // updating millisecond count and leds if necessary

    milliseconds += 2;

    // display next row
    displayRow();
    displayedRowId = (displayedRowId + 1) % NB_ROWS;

    // handling full minute
    if (milliseconds == 60000)
    {
        milliseconds = 0;
        addMinute();
        if (min1 == 0 && min0 == 0) addHour();
        updateMatrixData();
        updateRegisterData();
    }

    // handling hour button
    if (((PINB & (1 << BUTTON_HOURS_PIN)) >> BUTTON_HOURS_PIN) != buttonHoursPrevReading)
    {
        if (!buttonHoursWasPressed)
        {
            milliseconds = 0;
            addHour();
            updateMatrixData();
            updateRegisterData();
            buttonHoursWasPressed = 1;
        }
        else
        {
            buttonHoursWasPressed = 0;
        }
        buttonHoursPrevReading = (PINB & (1 << BUTTON_HOURS_PIN)) >> BUTTON_HOURS_PIN;
    }

    // handling minute button
    if (((PINB & (1 << BUTTON_MINUTES_PIN)) >> BUTTON_MINUTES_PIN) != buttonMinutesPrevReading)
    {
        if (!buttonMinutesWasPressed)
        {
            milliseconds = 0;
            addMinute();
            updateMatrixData();
            updateRegisterData();
            buttonMinutesWasPressed = 1;
        }
        else
        {
            buttonMinutesWasPressed = 0;
        }
        buttonMinutesPrevReading = (PINB & (1 << BUTTON_MINUTES_PIN)) >> BUTTON_MINUTES_PIN;
    }
}

int main(void)
{
    cli(); // disabling interrupts

    initPins();

    initTimer0();
    initTimer0Interrupt();

    sei(); // enabling interrupts

    // infinite loop
    for (;;)
    {
    }

    return 0;
}