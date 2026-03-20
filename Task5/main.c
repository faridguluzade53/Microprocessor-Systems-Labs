#pragma GCC optimize ("no-lto")

#include <avr/io.h>
#include <util/delay.h>

// ── pins ───────────────────────────────────────────────────────
#define LED_BIT  PB5
#define BTN_BIT  PB0

static inline void led_on()  { PORTB |=  (1 << LED_BIT); }
static inline void led_off() { PORTB &= ~(1 << LED_BIT); }

static void btn_wait_release(void) {
    while (!(PINB & (1 << BTN_BIT)));
    _delay_ms(30);
}

static uint8_t btn_pressed(void) {
    return !(PINB & (1 << BTN_BIT));
}

// ── global action index ────────────────────────────────────────
volatile uint8_t g_action = 0;

// ================================================================
//  PATTERN FUNCTIONS — forward declare all first
// ================================================================
void pattern_slow(void);
void pattern_double(void);
void pattern_strobe(void);

void pattern_slow(void) {
    led_on();  _delay_ms(500);
    led_off(); _delay_ms(500);
}

void pattern_double(void) {
    led_on();  _delay_ms(120);
    led_off(); _delay_ms(120);
    led_on();  _delay_ms(120);
    led_off(); _delay_ms(600);
}

void pattern_strobe(void) {
    for (uint8_t i = 0; i < 6; i++) {
        led_on();  _delay_ms(50);
        led_off(); _delay_ms(50);
    }
    _delay_ms(400);
}

// ================================================================
//  ACTION TABLES — raw function pointer arrays, no typedef
//  void(*table[])(void) = array of pointers to void functions
// ================================================================

static void (*table_A[3])(void) = {
    pattern_slow, pattern_double, pattern_strobe
};

static void (*table_B[3])(void) = {
    pattern_double, pattern_slow, pattern_strobe
};

static void (*table_C[3])(void) = {
    pattern_strobe, pattern_slow, pattern_double
};

// ── IJMP dispatch (Phase 5C) ───────────────────────────────────
static void dispatch(void (**table)(void)) {
    uint16_t addr = (uint16_t)(table[g_action % 3]);

    asm volatile (
        "movw r30, %0  \n\t"    // Z (r31:r30) ← handler address
        "ijmp          \n\t"    // PC ← Z  *** IJMP (Phase 5C) ***
        :
        : "r" (addr)
        : "r30", "r31"
    );
}

// ── check button and advance action ───────────────────────────
static void check_btn_advance(void) {
    if (btn_pressed()) {
        _delay_ms(30);
        btn_wait_release();
        g_action = (g_action + 1) % 3;
    }
}

// ================================================================
//  MODE ENTRY FUNCTIONS
// ================================================================
void modeA_entry(void);
void modeB_entry(void);
void modeC_entry(void);

void modeA_entry(void) {
    g_action = 0;
    while (1) {
        dispatch(table_A);
        check_btn_advance();
    }
}

void modeB_entry(void) {
    g_action = 0;
    while (1) {
        dispatch(table_B);
        check_btn_advance();
    }
}

void modeC_entry(void) {
    g_action = 0;
    while (1) {
        dispatch(table_C);
        check_btn_advance();
    }
}

// ── Phase 5B: JMP into chosen mode (never returns) ────────────
static void enter_mode(uint8_t mode) {
    uint16_t addr;

    if      (mode == 1) addr = (uint16_t)modeA_entry;
    else if (mode == 2) addr = (uint16_t)modeB_entry;
    else                addr = (uint16_t)modeC_entry;

    asm volatile (
        "movw r30, %0  \n\t"    // Z ← mode entry address
        "ijmp          \n\t"    // *** JMP to mode (Phase 5B) ***
        :
        : "r" (addr)
        : "r30", "r31"
    );

    __builtin_unreachable();
}

// ================================================================
//  SETUP & LOOP
// ================================================================

void setup(void) {
    DDRB  |=  (1 << LED_BIT);   // LED output
    DDRB  &= ~(1 << BTN_BIT);   // button input
    PORTB |=  (1 << BTN_BIT);   // internal pull-up ON
    led_off();
}

void loop(void) {

    // ── PHASE 5A: RJMP boot-wait loop ─────────────────────────
    asm volatile (
        "wait_btn:           \n\t"
        "  sbic %0, %1       \n\t"  // skip if PB0==0 (pressed)
        "  rjmp wait_btn     \n\t"  // PB0==1 → not pressed → loop
        :
        : "I" (_SFR_IO_ADDR(PINB)),
          "I" (BTN_BIT)
    );

    // Debounce + 200ms confirm flash
    _delay_ms(30);
    btn_wait_release();
    led_on();  _delay_ms(200);
    led_off(); _delay_ms(200);

    // ── PHASE 5B: count presses in 2-second window ────────────
    uint8_t  presses = 0;
    uint16_t elapsed = 0;

    while (elapsed < 2000) {
        if (btn_pressed()) {
            _delay_ms(30);
            presses++;
            btn_wait_release();
            led_on();  _delay_ms(80);
            led_off(); _delay_ms(80);
        }
        _delay_ms(10);
        elapsed += 10;
    }

    if (presses < 1) presses = 1;
    if (presses > 3) presses = 3;

    // Commit to mode via JMP — never returns
    enter_mode(presses);

    while(1); // unreachable
}
