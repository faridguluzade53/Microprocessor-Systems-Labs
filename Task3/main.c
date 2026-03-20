#include <Arduino.h>
#include <avr/io.h>

#define EE_LAST 0x00
#define EE_SET  0x01
#define EE_MODE 0x02

volatile uint8_t counter_sram = 0;
uint8_t mode = 0;

static inline uint8_t ee_read(uint16_t addr) {
  while (EECR & (1 << EEPE)) {}
  EEAR = addr;
  EECR |= (1 << EERE);
  return EEDR;
}

static inline void ee_write(uint16_t addr, uint8_t data) {
  while (EECR & (1 << EEPE)) {}
  EEAR = addr;
  EEDR = data;
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE);
}

static inline void ee_update(uint16_t addr, uint8_t data) {
  uint8_t cur = ee_read(addr);
  if (cur != data) ee_write(addr, data);
}

static inline uint8_t ee_read0xff_as0(uint16_t addr) {
  uint8_t v = ee_read(addr);
  if (v == 0xFF) v = 0;
  return v;
}

static inline void inc_r16_and_sync() {
  asm volatile(
    "mov r16, %0\n\t"
    "inc r16\n\t"
    "mov %0, r16\n\t"
    : "+r"(counter_sram)
    :
    : "r16"
  );
}

static inline void delay_1s_approx() {
  asm volatile(
    "ldi r18, 110\n\t"
    "L0:\n\t"
    "ldi r19, 220\n\t"
    "L1:\n\t"
    "ldi r20, 220\n\t"
    "L2:\n\t"
    "dec r20\n\t"
    "brne L2\n\t"
    "dec r19\n\t"
    "brne L1\n\t"
    "dec r18\n\t"
    "brne L0\n\t"
    :
    :
    : "r18", "r19", "r20"
  );
}

void setup() {
  Serial.begin(9600);
  delay(600);

  mode = ee_read0xff_as0(EE_MODE);
  if (mode != 1) mode = 0;

  uint8_t lastv = ee_read0xff_as0(EE_LAST);
  uint8_t setv  = ee_read0xff_as0(EE_SET);

  counter_sram = (mode == 1) ? setv : lastv;

  Serial.print("BOOT mode=");
  Serial.print(mode == 1 ? "SET" : "AUTO");
  Serial.print(" start=");
  Serial.println(counter_sram);

  Serial.println(counter_sram);
}

void loop() {
  delay_1s_approx();

  inc_r16_and_sync();

  if (mode == 0) {
    ee_update(EE_LAST, counter_sram);
  }

  Serial.println(counter_sram);

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n' || c == ' ') continue;

    if (c == 's' || c == 'S') {
      ee_update(EE_SET, counter_sram);
      ee_update(EE_MODE, 1);
      mode = 1;
      Serial.print("SET saved=");
      Serial.println(counter_sram);
    }

    if (c == 'r' || c == 'R') {
      counter_sram = 0;
      ee_update(EE_LAST, 0);
      ee_update(EE_SET, 0);
      ee_update(EE_MODE, 0);
      mode = 0;
      Serial.println("Reset to 0 (AUTO mode)");
      Serial.println(counter_sram);
    }
  }
}
