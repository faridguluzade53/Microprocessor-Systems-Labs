volatile uint8_t current_digit = 0;
volatile uint8_t paused = 0;

const int digits[10][7] = {
  {1, 1, 1, 1, 1, 1, 0}, // 0
  {0, 1, 1, 0, 0, 0, 0}, // 1
  {1, 1, 0, 1, 1, 0, 1}, // 2
  {1, 1, 1, 1, 0, 0, 1}, // 3
  {0, 1, 1, 0, 0, 1, 1}, // 4
  {1, 0, 1, 1, 0, 1, 1}, // 5
  {1, 0, 1, 1, 1, 1, 1}, // 6
  {1, 1, 1, 0, 0, 0, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 1, 1, 0, 1, 1}  // 9
};

void setup() {
  Serial.begin(9600);
  DDRD &= ~(1 << PD2);
  PORTD |= (1 << PD2);

  EICRA |= (1 << ISC01);
  EICRA &= ~(1 << ISC00);

  EIMSK |= (1 << INT0);

  DDRD |= 0b11111000; // PD3–PD7
  DDRB |= (1 << PB0) | (1 << PB1);

  TCCR1A = 0x00;
  TCCR1B &= ~((1 << CS11) | (1 << CS10));
  TCCR1B = (1 << CS12);
  TCNT1 = 34286;

  TIMSK1 = (1 << TOIE1);

  sei();
}

ISR(INT0_vect) {
  paused ^= 1;

  if (paused) {
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
  } else {
    TCCR1B |= (1 << CS12);
  }
}

ISR(TIMER1_OVF_vect) {
  TCNT1 = 34286;

  current_digit = (current_digit + 1) % 10;
}

void displayDigit() {
  const int* volt_mapping = digits[current_digit];

  PORTD &= ~((1 << PD3) | (1 << PD4) | (1 << PD5) |
           (1 << PD6) | (1 << PD7));

  PORTD |= (volt_mapping[0] << PD3) | 
  (volt_mapping[1] << PD4) | 
  (volt_mapping[2] << PD5) | 
  (volt_mapping[3] << PD6) | 
  (volt_mapping[4] << PD7);

  PORTB &= ~((1 << PB0) | (1 << PB1));

  PORTB |= (volt_mapping[5] << PB0) |
  (volt_mapping[6] << PB1);
}

void loop() {
  displayDigit();
}
