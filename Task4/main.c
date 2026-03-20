#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t countdown = 0;

// Timer1 Compare Match A — fires every 1ms
ISR(TIMER1_COMPA_vect) {
    if (countdown > 0) {
        countdown--;
        if (countdown == 0) {
            PORTB &= ~(1 << PB5);  // Set output LOW (pin 13)
            // Stop Timer1
            TCCR1B &= ~((1 << CS11) | (1 << CS10));
        }
    }
}

// External Interrupt 0 — fires on rising edge of D2
ISR(INT0_vect) {
    countdown = 10;                // 10 x 1ms = 10ms
    PORTB |= (1 << PB5);          // Set output HIGH (pin 13)

    // Reset and start Timer1 with prescaler 64
    TCNT1 = 0;
    TCCR1B |= (1 << CS11) | (1 << CS10);  // prescaler = 64
}

int main(void) {
    // === Output pin: PB5 (Arduino pin 13) ===
    DDRB |= (1 << PB5);           // Set PB5 as output
    PORTB &= ~(1 << PB5);         // Start LOW

    // === Input pin: PD2 (Arduino D2) with pull-up ===
    DDRD  &= ~(1 << PD2);         // PD2 as input
    PORTD |=  (1 << PD2);         // Enable internal pull-up

    // === External Interrupt INT0 on PD2: falling edge ===
    EICRA |=  (1 << ISC01);       // ISC01=1
    EICRA |=  (1 << ISC00);       // ISC00=1  → rising edge
    EIMSK |=  (1 << INT0);        // Enable INT0

    // === Timer1: CTC mode, prescaler 64, OCR1A=249 (1ms) ===
    TCCR1A = 0;                    // Normal port operation
    TCCR1B = (1 << WGM12);        // CTC mode (TOP = OCR1A)
                                   // No clock yet — started in ISR
    OCR1A  = 249;                  // 16MHz / 64 / 250 = 1000 Hz = 1ms
    TIMSK1 |= (1 << OCIE1A);      // Enable compare match interrupt

    sei();                         // Enable global interrupts

    while (1) {
        // Main loop intentionally empty
    }
}
