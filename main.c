// 1D Pong for ATtiny2313
// 2 buttons and 20 charlieplexed LEDs
// Code driven by interrupts
// Written by Tobias Balle-Petersen

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>


// Debounce delay.
#define DEBOUNCE 500


// How long is the play field?
#define FIELD_LENGTH 20  // Could be shorter than display

// No of LEDs to multiplex
#define NO_OF_LEDS 20

// Anode, cathode, status
uint8_t plex_pins[NO_OF_LEDS][2] = {
  {PB4, PB3},
  {PB3, PB4},
  {PB3, PB2},
  {PB2, PB3},
  {PB4, PB2},
  {PB2, PB4},
  
  {PB1, PB2},
  {PB2, PB1},
  {PB1, PB3},
  {PB3, PB1},
  {PB1, PB4},
  {PB4, PB1},

  {PB0, PB1},
  {PB1, PB0},
  {PB0, PB2},
  {PB2, PB0},
  {PB0, PB3},
  {PB3, PB0},
  {PB0, PB4},
  {PB4, PB0},

};

// Status of LEDs (ON/OFF)
volatile uint8_t led_status[NO_OF_LEDS];

volatile uint16_t debounce_p1;
volatile uint16_t debounce_p2;

// Number of the LED currently on the charlieplexing loop 
volatile uint8_t current_led;
volatile uint8_t prev_led;

volatile uint8_t ball_poss;

// Ball direction. 0 = left, 1 = right
volatile uint8_t ball_dir;

// Buttons where pressed
volatile uint8_t b1_pressed;
volatile uint8_t b2_pressed;

// Game speed
#define START_GAME_SPEED 20000

// Store changing speed here
uint16_t game_speed = START_GAME_SPEED;
 
// Players' scores
uint8_t p1_score;
uint8_t p2_score;

// Game ends when this score is reached
#define MAX_SCORE 3

#define MODE_ATTRACT 0
#define MODE_GAME 1
uint8_t game_mode;

// Set to true when game advances
volatile uint8_t tick;

// Button 1
ISR(INT0_vect) {
  if (debounce_p1 == 0) {
    b1_pressed = 1;
    debounce_p1 = DEBOUNCE;
  }
}

// Button 2
ISR(INT1_vect) {
  if (debounce_p2 == 0) {
    b2_pressed = 1;
    debounce_p2 = DEBOUNCE;
  }
}

// Multiplexing
ISR(TIMER0_COMPA_vect) {
  
  // Debounce
  if (debounce_p1 > 0)
    debounce_p1--;
    
  if (debounce_p2 > 0)
    debounce_p2--;
      
  // Turn prev LEDs anode off
  if (plex_pins[prev_led]) {
    PORTB &=  ~(1<<plex_pins[prev_led][0]);
    // Prev LEDs pins are inputs
    DDRB &= ~(1<<plex_pins[prev_led][0]);
    DDRB &= ~(1<<plex_pins[prev_led][1]);
  }
  
  // Turn on current LED
  if (led_status[current_led]) {
    
    // Both pins are outputs
    DDRB |=  1<<plex_pins[current_led][0];
    DDRB |=  1<<plex_pins[current_led][1];
    
    // Annode on
    PORTB |=  1<<plex_pins[current_led][0];
    // Cathode off
    //PORTB &=  ~(1<<plex_pins[current_led][1]);
    //delay_ms(40);
  }

  // Rember this LED no  
  prev_led = current_led;
  // Next LED no
  current_led++;  
  if (current_led >= NO_OF_LEDS)
    current_led = 0;
  
}

// Game tick
ISR(TIMER1_COMPA_vect) {
  tick = 1;
}

void update_ball() {
    
     
  // Calculate next ball position
    // Attract
  /*if (game_mode == MODE_ATTRACT) {
    // Do something in attract mode
  } else {*/
    // Turn off current ball position
    //plex_pins[ball_poss][2] = 0;
    led_status[ball_poss] = 0;

    // If out of field. 0-- = 255 remember.
    if (ball_poss >= FIELD_LENGTH) {
      ball_poss = 10;
      //ball_dir = ~ball_dir;
    }
    // Move ball
    if (ball_dir == 1) {
      // Out?
      ball_poss++;
    } else {
      // Out?
      ball_poss--;
    }

    // Turn on new ball position
    led_status[ball_poss] = 1;

  //}
  
}

void init_pins() {
  uint8_t i;
  
  // LED pins are outputs
  DDRD |= 1<<PD0;
  //PORTD |= 1<<PD0;
  
  // Button pins are inputs (Default I think)
  DDRD &= ~(1<<PD2);
  DDRD &= ~(1<<PD3);
  
  // Activate pull up resistors on button pins
  PORTD |= 1<<PD2; 
  PORTD |= 1<<PD3; 
}

void init_interrupts() {
  
  // TIMERS
  
  
  // Timer 1 (Game)
  // set up timer with prescaler = 64 and CTC mode
  TCCR1B |= (1 << CS11); // 1/8
  //TCCR1B |= (1 << CS11)|(1 << CS10); // 1/64
  TCCR1B |= (1 << WGM12); //CTC
  
  // initialize counter
  TCNT1 = 0;
  
  // initialize compare value
  OCR1A = game_speed;
  
  // enable compare interrupt
  TIMSK |= (1 << OCIE1A);
    

  // TIMER0
  // Charlieplexing
    
  //TCCR0B |= 1<<CS00; // Internal clock, no prescale
  TCCR0B |= 1<<CS01; // Internal clock, 1/8 prescale
  //TCCR0B |= 1<<CS02; // Internal clock, no prescale
  //TCCR0B |= 1<<CS00 | 1<<CS02; // Internal clock, 1/1024 prescale
  
  
  //OCR0A  = 0x70;      // number to count up to (0x70 = 112)
  OCR0A  = 140;      // number to count up to (0x70 = 112)
  //OCR0A  = 0xFF;      // number to count up to (0x70 = 112)
  TCCR0A = 0x02;      // Clear Timer on Compare Match (CTC) mode
  TIFR |= 0x01;       // clear interrupt flag
  // Time Counter Interrupt Mask
  TIMSK |= 1<<OCIE0A;       // TC0 compare match A interrupt enable
  //TCCR0B = 0x05;      // clock source CLK/1024, start timer

  
  
  // EXTERNAL INTERRUPTS
  
  // Enable pin change interrupts on button pins
  PCMSK |= (1<<PD2);  
  PCMSK |= (1<<PD3);

  // Pin change interrupts triggered on falling edge
  // ISC = Interrupt Sense Control
  MCUCR |= (1<<ISC01); //INT0
  MCUCR |= (1<<ISC11); //INT1
  
  // Turn on these external interrupts
  GIMSK |= (1<<INT0);
  GIMSK |= (1<<INT1);

  // Global interrupt enable (SREG) 
  sei();
}

void reset_display() {
  uint8_t i;
  for (i=0; i < NO_OF_LEDS; i++) {
    led_status[i] = 0;
  }
  // Current LED in multiplex cycle
  current_led = 0;
}

void display_p1_score(uint8_t state) {
  uint8_t i;
  for (i=0;i<p1_score;i++) {
    led_status[i] = state;
  }
}

void display_p2_score(uint8_t state) {
  uint8_t i;
  for (i=0;i<p2_score;i++) {
    led_status[FIELD_LENGTH-1-i] = state;
  }
}

void init_buttons() {
  b1_pressed = 0;
  b2_pressed = 0;
  debounce_p1 = DEBOUNCE;
  debounce_p2 = DEBOUNCE;
}
void init_game() {
  ball_poss = 10;
  ball_dir = 1;
  p1_score = 0;
  p2_score = 0;
  // Use last bit of timer as random
  //ball_dir = (TCNT0 & (1<<0));
  // Turn on ball pos
  //plex_pins[ball_poss][2] = 1;
  led_status[ball_poss] = 1;
}

void delay_ms(uint8_t ms) {
    uint16_t delay_count = 1000000 / 17500;
    volatile uint16_t i;
    
    while (ms != 0) {
        for (i=0; i != delay_count; i++);
        ms--;
    }
}


int main(void) {
  
  // Attract
  //game_mode = MODE_ATTRACT;
  //game_mode = MODE_GAME;
  tick = 0;
  
  reset_display();
  
  init_pins();

  init_interrupts();
  
  init_game();
  init_buttons();
    
  // For loops
  //uint8_t i;

  //uint8_t *p_loosers_button;
  
  while(1) {

    // B1 pressed
    if (b1_pressed) {
      // Change dir if ball on player's end
      if (ball_poss == FIELD_LENGTH-1) {
        ball_dir = 0;
      }
      b1_pressed = 0;
    }
    // B2 pressed
    if (b2_pressed) {
      // Change dir if ball on player's end
      if (ball_poss == 0) {
        ball_dir = 1;
      }
      b2_pressed = 0;
    }
    
    
    // If time passed
    if (tick) {
      
      // Increase game speed
      if (game_speed > 600) {
        game_speed -= 50;
        OCR1A = game_speed;
      }
      
      if (ball_poss < FIELD_LENGTH) {

        // Turn off current ball pos LED
        // Has been on for a tick now
        led_status[ball_poss] = 0;

        // Advance ball
        if (ball_dir) {
          ball_poss++;
        } else {
          ball_poss--;
        }
      
        // Turn on new ball position if on field
        if (ball_poss < FIELD_LENGTH) {
          led_status[ball_poss] = 1;
        }
      } else {
        // Add point to winner's score
        if (ball_dir == 1) {
          p1_score++;
          ball_poss = (FIELD_LENGTH-1);
          //p_loosers_button = &b2_pressed;
        } else {
          p2_score++;
          ball_poss = 0;
          //p_loosers_button = &b1_pressed;
        }
        
        // Flip ball direction
        ball_dir = !ball_dir;
        
        // Reset speed
        game_speed = START_GAME_SPEED;
        
        // Display score
        display_p1_score(1);
        display_p2_score(1);
        // Wait for button
        while(b1_pressed == b2_pressed){}
        init_buttons();
        // Blank display
        reset_display();
        
        // Serve
        while(b1_pressed == b2_pressed){
          if (tick) {          
            led_status[ball_poss] = ~led_status[ball_poss];
            tick = 0;
          }
        }        
        init_buttons();
      }
      
      // Wait for next tick
      tick = 0;

    } // Tick
    
  }
  
  return 0;
}
