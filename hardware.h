extern volatile unsigned int ticks;

#define LED PORTD,3
#define UART_TX PORTD,1

#define NO_OF_POLES 4

#define SDA PORTC,4
#define SCL PORTC,5

#define P_UP PORTB,4
#define P_UN PORTB,3 // OC2
#define P_VP PORTB,0
#define P_VN PORTB,2 // OC1B
#define P_WP PORTB,7
#define P_WN PORTB,1 // OB1A

#define PHASE_U_IN PORTC,4
#define PHASE_V_IN PORTC,3
#define PHASE_W_IN PORTC,2

// funcs
#define BTN_PRESSED() (IO_IS_LOW(P_BTN))
#define BTN2_PRESSED() (IO_IS_LOW(P_BTN2))

#define ENABLE_POWER() { IO_PUSH_PULL(P_ON); IO_LOW(P_ON); }
#define DISABLE_POWER() { IO_INPUT(P_ON); IO_LOW(P_ON); }

#define ENABLE_UN TCCR2 |= (_BV(COM21)/* | _BV(COM20)*/)
#define DISABLE_UN TCCR2 &= ~(_BV(COM21)/* | _BV(COM20)*/)

#define ENABLE_UP IO_HIGH(P_UP)
#define DISABLE_UP IO_LOW(P_UP)

#define ENABLE_VN TCCR1A |= (_BV(COM1B1)/* | _BV(COM1B0)*/)
#define DISABLE_VN TCCR1A &= ~(_BV(COM1B1)/* | _BV(COM1B0)*/)

#define ENABLE_VP IO_HIGH(P_VP)
#define DISABLE_VP IO_LOW(P_VP)

#define ENABLE_WN TCCR1A |= (_BV(COM1A1)/* | _BV(COM1A0)*/)
#define DISABLE_WN TCCR1A &= ~(_BV(COM1A1)/* | _BV(COM1A0)*/)

#define ENABLE_WP IO_HIGH(P_WP)
#define DISABLE_WP IO_LOW(P_WP)

#define DISABLE_ALL() { DISABLE_UP; DISABLE_UN; DISABLE_VP; DISABLE_VN; DISABLE_WP; DISABLE_WN; }
