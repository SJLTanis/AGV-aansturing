#define F_CPU 16000000UL // 16MHz
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Defines
    // System functions
    #define System_DDR DDRD
    #define System_PORT DDRD
    #define noodstop PD2
    #define buzzer PD3
    #define SystemOn PC4

    // Motor functions
    #define Motors_DDR DDRD
    #define Motors_Port PORTD
    #define Motor_Len1 PD4
    #define Motor_Len2 PD5
    #define Motor_Ren1 PD6
    #define Motor_Ren2 PD7


uint16_t rising, rising2, rising3, falling, falling2, falling3;

volatile uint32_t counts, counts2, counts3; //it protects the compiler from changing the value of the variable
volatile uint32_t dist, dist2, dist3;
uint16_t us_per_count; //this is the measurement cycle


void init_timer()
{
    DDRB |= (1<<2); // for the trigpin
    TCCR1A |= (1<<WGM12);
    TCCR1A |= (1<<CS11) | (1<<CS10); //clkdiv 64
    TCCR1A |= (1 << ICES1); //rising edge capture
    TIMSK1 |= (1<<ICIE1) | (1<<OCIE1A);
    OCR1A = 8750; //output compare waarde
    us_per_count = 4;
    sei();
}

void init_timer2()
{
    DDRB |= (1<<3); // for the trigpin
    TCCR1B |= (1<<WGM13);
    TCCR1B |= (1<<CS21) | (1<<CS20); //clkdiv 64
    TCCR1B |= (1 << ICES1); //rising edge capture
    TIMSK2 |= (1<<ICIE1) | (1<<OCIE1A);
    OCR1A = 8750; //output compare waarde
    us_per_count = 4;
    sei();
}

void init_timer3()
{
    DDRB |= (1<<4); // for the trigpin
    TCCR1C |= (1<<WGM11);
    TCCR1C |= (1<<CS11) | (1<<CS10); //clkdiv 64
    TCCR1C |= (1 << ICES1); //rising edge capture
    TIMSK0 |= (1<<ICIE1) | (1<<OCIE1A);
    OCR1A = 8750; //output compare waarde
    us_per_count = 4;
    sei();
}

ISR(TIMER1_CAPT_vect)
{
    if(TCCR1A & (1<<ICES1)) // trigger capture of rising edge
    {
        rising = ICR1; // Save to Input Capture Register
        TCCR1A &= ~(1<<ICES1); //detect the falling edge
    }
    else
    {
        falling = ICR1; //save input to capture register
        TCCR1A |= (1<<ICES1); //capture the falling edge time
        counts = (uint32_t)falling - (uint32_t)rising ; //difference in time between rising and falling edge
        dist = (uint32_t)us_per_count * counts / 58;  //in microseconds
    }

    if(TCCR1B & (1<<ICES1)) // trigger capture of rising edge
    {
        rising2 = ICR1; // Save to Input Capture Register
        TCCR1B &= ~(1<<ICES1); //detect the falling edge
    }
    else
    {
        falling2 = ICR1; //save input to capture register
        TCCR1B |= (1<<ICES1); //capture the falling edge time
        counts2 = (uint32_t)falling2 - (uint32_t)rising2 ; //difference in time between rising and falling edge
        dist2 = (uint32_t)us_per_count * counts / 58;  //in microseconds
    }

    if(TCCR1C & (1<<ICES1)) // trigger capture of rising edge
    {
        rising3 = ICR1; // Save to Input Capture Register
        TCCR1C &= ~(1<<ICES1); //detect the falling edge
    }
    else
    {
        falling3 = ICR1; //save input to capture register
        TCCR1C |= (1<<ICES1); //capture the falling edge time
        counts3 = (uint32_t)falling3 - (uint32_t)rising3 ; //difference in time between rising and falling edge
        dist3 = (uint32_t)us_per_count * counts / 58;  //in microseconds
    }
}

ISR(TIMER0_COMPA_vect)
{
    PORTB |= (1<<2); //trigger pin high
    _delay_us(10); // wait 10 microseconds
    PORTB &= ~(1<<2); // trigger pin low
}

ISR(TIMER1_COMPA_vect)
{
    PORTB |= (1<<3); //trigger pin high
    _delay_us(10); // wait 10 microseconds
    PORTB &= ~(1<<3); // trigger pin low
}

ISR(TIMER2_COMPA_vect)
{
    PORTB |= (1<<4); //trigger pin high
    _delay_us(10); // wait 10 microseconds
    PORTB &= ~(1<<4); // trigger pin low
}


void Forward(){
    PORTD |= (1<<Motor_Len1);
    PORTD &= ~(1<<Motor_Len2);
    PORTD |= (1<<Motor_Ren1);
    PORTD &= ~(1<<Motor_Ren2);
}

void turnRight(){
    PORTD &= ~(1<<Motor_Len1);
    PORTD |= (1<<Motor_Len2);
    PORTD |= (1<<Motor_Ren1);
    PORTD &= ~(1<<Motor_Ren2);
}

void turnLeft(){
    PORTD |= (1<<Motor_Len1);
    PORTD &= ~(1<<Motor_Len2);
    PORTD &= ~(1<<Motor_Ren1);
    PORTD |= (1<<Motor_Ren2);
}

void Stop(){
    PORTD &= ~(1<<Motor_Len1);
    PORTD &= ~(1<<Motor_Len2);
    PORTD &= ~(1<<Motor_Ren1);
    PORTD &= ~(1<<Motor_Ren2);
}

int main(void)
{
    // Open zetten van de system components
    DDRD |= (1<<buzzer);
    DDRD &= ~(1<<noodstop);
    DDRC &= ~(1<<SystemOn);

    // Open zetten van de motor enables
    DDRD |= (1<<Motor_Len1);
    DDRD |= (1<<Motor_Len2);
    DDRD |= (1<<Motor_Ren1);
    DDRD |= (1<<Motor_Ren2);

    // Inits
    // Gemeten afstanden vanaf de 3 ultrasonische sensoren
    int dist; // Gemeten afstand vanaf de voorste ultrasonische sensor
    int dist2; // Gemeten afstand vanaf de linker ultrasonische sensor
    int dist3; // Gemeten afstand vanaf de rechter ultrasonische sensor

    int phase = 1; // Fase om te kunnen schakelen in welke modus de AGV zich bevindt, start in fase 1
    int pos;

    init_timer(); // Timer
    init_timer2(); // Timer 2
    init_timer3(); // Timer 3

    while (1)
    {
        if((1<<SystemOn) && (noodstop == 0)){ // Check of het systeem aanstaat en de noodstop niet ge-activeerd is
            _delay_ms(5000); // Wait for placement or adjustments

            if(phase == 0){
                Stop();
            }

            if(phase == 1){ // Iniatie oriëntatie fase
                if(dist2 < 5){ // Linker sensor voor oriëntatie
                    pos = 1;
                    phase = 2;
                }
                if(dist2 < 47){
                    pos = 2;
                    phase = 2;
                }
                if(dist2 < 74){
                    pos = 3;
                    phase = 2;
                }
                if(dist2 <100){
                    pos = 4;
                    phase = 2;
                }
            }

            if(phase == 2){ // Rijden en detecteren
                if(pos == 1){
                    if(dist > 5 && dist3 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 3;
                        }
                    }
                }
                if(pos == 2){
                    if(dist > 5 && dist2 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 3;
                        }
                    }
                }
                if(pos == 3){
                    if(dist > 5 && dist3 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 3;
                        }
                    }
                }
                if(pos == 4){
                    if(dist > 5 && dist2 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 3;
                        }
                    }
                }
            }

            if(phase == 3){ // Eerste 90 graden
                if(pos == 1){
                    while(dist3 > 200 || dist < 100){
                        turnRight();
                    }
                    if(dist < 100 && dist2 < 5 && dist3 > 200){
                        phase = 4;
                        Forward();
                    }
                }
                if(pos == 2){
                    while(dist3 > 200 || dist < 80){
                        turnRight();
                    }
                    if(dist < 80 && dist2 < 5 && dist3 > 200){
                        phase = 4;
                        Forward();
                    }
                }
                if(pos == 3){
                    while(dist2 > 200 || dist < 80){
                        turnLeft();
                    }
                    if(dist < 80 && dist2 > 200 && dist3 < 5){
                        phase = 4;
                        Forward();
                    }
                }
                if(pos == 4){
                    while(dist2 > 200 || dist < 100){
                        turnLeft();
                    }
                    if(dist < 100 && dist2 > 200 && dist3 < 5){
                        phase = 4;
                        Forward();
                    }
                }
            }

            if(phase == 4){ // Rijden naar de juiste positie
                if(pos == 1){
                    if(dist > 74){
                        Forward();
                    } else {
                        phase = 5;
                    }
                }
                if(pos == 2){
                    if(dist > 47){
                        Forward();
                    } else {
                        phase = 5;
                    }
                }
                if(pos == 3){
                    if(dist > 47){
                        Forward();
                    } else {
                        phase = 5;
                    }
                }
                if(pos == 4){
                    if(dist > 74){
                        Forward();
                    } else {
                        phase = 5;
                    }
                }
            }

            if(phase == 5){ // Tweede 90 graden
                if(pos == 1){
                    while(dist2 > 40 || dist3 < 70){
                        turnRight();
                    }
                    if(dist > 200 && dist2 > 40 && dist3 < 70){
                        phase = 6;
                        Forward();
                    }
                }
                if(pos == 2){
                    while(dist2 > 70 || dist3 < 40){
                        turnRight();
                    }
                    if(dist > 200 && dist2 > 70 && dist3 < 40){
                        phase = 6;
                        Forward();
                    }
                }
                if(pos == 3){
                    while(dist2 < 70 || dist3 > 40){
                        turnLeft();
                    }
                    if(dist > 200 && dist2 < 70 && dist3 > 40){
                        phase = 6;
                        Forward();
                    }
                }
                if(pos == 4){
                    while(dist2 > 40 || dist3 < 70){
                        turnRight();
                    }
                    if(dist > 200 && dist2 > 40 && dist3 < 70){
                        phase = 6;
                        Forward();
                    }
                }
            }

            if(phase == 6){ // Rijden en bomen detecteren
                if(pos == 1){
                    if(dist > 5 && dist2 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 7;
                        }
                    }
                }
                if(pos == 2){
                    if(dist > 5 && dist2 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 7;
                        }
                    }
                }
                if(pos == 3){
                    if(dist > 5 && dist2 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 7;
                        }
                    }
                }
                if(pos == 4){
                    if(dist > 5 && dist2 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 7;
                        }
                    }
                }
            }

            if(phase == 7){ // Derde 90 graden
                if(pos == 1){
                    while(dist2 < 200 || dist < 70){
                        turnLeft();
                    }
                    if(dist > 70 && dist2 < 200 && dist3 < 5){
                        phase = 8;
                        Forward();
                    }
                }
                if(pos == 2){
                    while(dist2 < 200 || dist < 40){
                        turnLeft();
                    }
                    if(dist > 40 && dist2 < 200 && dist3 < 5){
                        phase = 8;
                        Forward();
                    }
                }
                if(pos == 3){
                    while(dist3 < 200 || dist < 40){
                        turnRight();
                    }
                    if(dist < 40 && dist2 < 5 && dist3 < 200){
                        phase = 8;
                        Forward();
                    }
                }
                if(pos == 4){
                    while(dist3 < 200 || dist < 70){
                        turnRight();
                    }
                    if(dist < 70 && dist2 < 5 && dist3 < 200){
                        phase = 8;
                        Forward();
                    }
                }
            }

            if(phase == 8){ // Rijden naar de juiste positie
                if(pos == 1){
                    if(dist > 47){
                        Forward();
                    } else {
                        phase = 9;
                    }
                }
                if(pos == 2){
                    if(dist > 5){
                        Forward();
                    } else {
                        phase = 9;
                    }
                }
                if(pos == 3){
                    if(dist > 5){
                        Forward();
                    } else {
                        phase = 9;
                    }
                }
                if(pos == 4){
                    if(dist > 5){
                        Forward();
                    } else {
                        phase = 9;
                    }
                }
            }

            if(phase == 9){ // Vierde 90 graden
                if(pos == 1){
                    while(dist2 < 80 || dist3 < 50){
                        turnLeft();
                    }
                    if(dist > 200 && dist2 < 80 && dist3 < 50){
                        phase = 10;
                        Forward();
                    }
                }
                if(pos == 2){
                    while(dist2 < 100 || dist < 200){
                        turnLeft();
                    }
                    if(dist > 200 && dist2 > 100 && dist3 < 5){
                        phase = 10;
                        Forward();
                    }
                }
                if(pos == 3){
                    while(dist3 > 100 || dist < 200){
                        turnRight();
                    }
                    if(dist > 200 && dist2 > 100 && dist3 < 5){
                        phase = 10;
                        Forward();
                    }
                }
                if(pos == 4){
                    while(dist3 > 100 || dist < 200){
                        turnRight();
                    }
                    if(dist > 200 && dist2 < 5 && dist3 < 100){
                        phase = 10;
                        Forward();
                    }
                }
            }

            if(phase == 10){ // Rij uitrijden en bomen detecteren
                if(pos == 1){
                    if(dist > 5 && dist3 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 0;
                        }
                    }
                }
                if(pos == 2){
                    if(dist > 5 && dist2 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 0;
                        }
                    }
                }
                if(pos == 3){
                    if(dist > 5 && dist3 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 0;
                        }
                    }
                }
                if(pos == 4){
                        if(dist > 5 && dist3 > 10){
                        Forward();
                    } else {
                        Stop();
                        if(dist < 5){
                        phase = 0;
                        }
                    }
                }
            }
        } else {
            Stop();
        }
    }
}


