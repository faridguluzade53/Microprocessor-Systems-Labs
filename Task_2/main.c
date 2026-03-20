#include <Arduino.h>

uint8_t readU8() {
  while (!Serial.available()) {}
  long v = Serial.parseInt();
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  return (uint8_t)v;
}

void printU8(const char *label, uint8_t v) {
  Serial.print(label);
  Serial.print(v);
  Serial.print(" (0b");
  for (int8_t i = 7; i >= 0; --i) Serial.print((v >> i) & 1);
  Serial.println(")");
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  Serial.setTimeout(60000);
  Serial.println("Enter A (0..255), then B (0..255).");
}

void loop() {
  Serial.print("A: ");
  uint8_t a = readU8();
  Serial.println(a);

  Serial.print("B: ");
  uint8_t b = readU8();
  Serial.println(b);

  uint8_t sreg;

  asm volatile(
    "sub %[a], %[b]\n\t"
    "in  %[s], __SREG__\n\t"
    : [a] "+r" (a), [s] "=r" (sreg)
    : [b] "r" (b)
    : "cc"
  );

  uint8_t result = a;
  uint8_t C = (sreg >> 0) & 1;
  uint8_t Z = (sreg >> 1) & 1;

  Serial.println("----");
  printU8("A_in: ", (uint8_t)(result + b));
  printU8("B_in: ", b);
  printU8("Result (A-B): ", result);
  printU8("SREG: ", sreg);
  Serial.print("C (bit0): "); Serial.println(C);
  Serial.print("Z (bit1): "); Serial.println(Z);
  Serial.println("----\n");
}
