#define F_CPU 16000000UL // 16MHz
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Defines
    // System functions
    #define System_DDR DDRH
    #define System_PORT DDRH
    #define noodstop PD2
    #define buzzer PD3

    // Motor functions
    #define Motors_DDR DDRH
    #define Motors_Port PORTH
    #define Motor_Len1 PD4
    #define Motor_Len2 PD5
    #define Motor_Ren1 PD6
    #define Motor_Ren2 PD7


uint16_t rising, falling;

volatile uint32_t counts; //it protects the compiler from changing the value of the variable
volatile uint32_t dist;
uint16_t us_per_count; //this is the measurement cycle


void init_timer()
{
    DDRL |= (1<<2); // for the trigpin
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
    DDRL |= (1<<3); // for the trigpin
    TCCR1B |= (1<<WGM13);
    TCCR1B |= (1<<CS21) | (1<<CS20); //clkdiv 64
    TCCR1B |= (1 << ICES2); //rising edge capture
    TIMSK2 |= (1<<ICIE2) | (1<<OCIE2A);
    OCR1A = 8750; //output compare waarde
    us_per_count = 4;
    sei();
}

void init_timer3()
{
    DDRL |= (1<<4); // for the trigpin
    TCCR1C |= (1<<WGM14);
    TCCR1C |= (1<<CS31) | (1<<CS13); //clkdiv 64
    TCCR1C |= (1 << ICES3); //rising edge capture
    TIMSK3 |= (1<<ICIE3) | (1<<OCIE3A);
    OCR1A = 8750; //output compare waarde
    us_per_count = 4;
    sei();
}

ISR(TIMER1_CAPT_vect)
{
    if(TCCR1B & (1<<ICES1)) // trigger capture of rising edge
    {
        rising = ICR1; // Save to Input Capture Register
        TCCR1B &= ~(1<<ICES1); //detect the falling edge
    }
    else
    {
        falling = ICR1; //save input to capture register
        TCCR1B |= (1<<ICES1); //capture the falling edge time
        counts = (uint32_t)falling - (uint32_t)rising ; //difference in time between rising and falling edge
        dist = (uint32_t)us_per_count * counts / 58;  //in microseconds
    }
}

ISR(TIMER1_COMPA_vect)
{
    PORTL |= (1<<2); //trigger pin high
    _delay_us(10); // wait 10 microseconds
    PORTL &= ~(1<<2); // trigger pin low
}

ISR(TIMER2_CAPT_vect)
{
    if(TCCR2B & (1<<ICES2)) // trigger capture of rising edge
    {
        rising2 = ICR2; // Save to Input Capture Register
        TCCR1B &= ~(1<<ICES2); //detect the falling edge
    }
    else
    {
        falling2 = ICR2; //save input to capture register
        TCCR2B |= (1<<ICES2); //capture the falling edge time
        counts2 = (uint32_t)falling2 - (uint32_t)rising2 ; //difference in time between rising and falling edge
        dist2 = (uint32_t)us_per_count * counts / 58;  //in microseconds
    }
}

ISR(TIMER2_COMPA_vect)
{
    PORTL |= (1<<3); //trigger pin high
    _delay_us(10); // wait 10 microseconds
    PORTL &= ~(1<<3); // trigger pin low
}

ISR(TIMER3_CAPT_vect)
{
    if(TCCR3B & (1<<ICES3)) // trigger capture of rising edge
    {
        rising3 = ICR3; // Save to Input Capture Register
        TCCR3B &= ~(1<<ICES3); //detect the falling edge
    }
    else
    {
        falling3 = ICR3; //save input to capture register
        TCCR3B |= (1<<ICES3); //capture the falling edge time
        counts3 = (uint32_t)falling3 - (uint32_t)rising3 ; //difference in time between rising and falling edge
        dist3 = (uint32_t)us_per_count * counts / 58;  //in microseconds
    }
}

ISR(TIMER3_COMPA_vect)
{
    PORTL |= (1<<4); //trigger pin high
    _delay_us(10); // wait 10 microseconds
    PORTL &= ~(1<<4); // trigger pin low
}

int main(void)
{
    // Open zetten van de system components
    DDRH |= (1<<buzzer);
    DDRH &= ~(1<<noodstop);

    // Open zetten van de motor enables
    DDRH |= (1<<Motor_Len1);
    DDRH |= (1<<Motor_Len2);
    DDRH |= (1<<Motor_Ren1);
    DDRH |= (1<<Motor_Ren2);

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
        if((1<<System_On) && (Noodstop == 0)){ // Check of het systeem aanstaat en de noodstop niet ge-activeerd is
            _delay_ms(5000); // Wait for placement or adjustments

            if(phase == 0){
                stop();
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
                        forward();
                    } else {
                        stop();
                        if(dist < 5){
                        phase = 3;
                        }
                    }
                }
                if(pos == 2){
                    if(dist > 5 && dist2 > 10){
                        forward();
                    } else {
                        stop();
                        if(dist < 5){
                        phase = 3;
                        }
                    }
                }
                if(pos == 3){
                    if(dist > 5 && dist3 > 10){
                        forward();
                    } else {
                        stop();
                        if(dist < 5){
                        phase = 3;
                        }
                    }
                }
                if(pos == 4){
                    if(dist > 5 && dist2 > 10){
                        forward();
                    } else {
                        stop();
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
                        forward();
                    }
                }
                if(pos == 2){
                    while(dist3 > 200 || dist < 80){
                        turnRight();
                    }
                    if(dist < 80 && dist2 < 5 && dist3 > 200){
                        phase = 4;
                        forward();
                    }
                }
                if(pos == 3){
                    while(dist2 > 200 || dist < 80){
                        turnLeft();
                    }
                    if(dist < 80 && dist2 > 200 && dist3 < 5){
                        phase = 4;
                        forward();
                    }
                }
                if(pos == 4){
                    while(dist2 > 200 || dist < 100){
                        turnLeft();
                    }
                    if(dist < 100 && dist2 > 200 && dist3 < 5){
                        phase = 4;
                        forward();
                    }
                }
            }

            if(phase == 4){ // Rijden naar de juiste positie
                if(pos == 1){
                    if(dist > 74){
                        forward();
                    } else {
                        phase = 5;
                    }
                }
                if(pos == 2){
                    if(dist > 47){
                        forward();
                    } else {
                        phase = 5;
                    }
                }
                if(pos == 3){
                    if(dist > 47){
                        forward();
                    } else {
                        phase = 5;
                    }
                }
                if(pos == 4){
                    if(dist > 74){
                        forward();
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
                        forward();
                    }
                }
                if(pos == 2){
                    while(dist2 > 70 || dist3 < 40){
                        turnRight();
                    }
                    if(dist > 200 && dist2 > 70 && dist3 < 40){
                        phase = 6;
                        forward();
                    }
                }
                if(pos == 3){
                    while(dist2 < 70 || dist3 > 40){
                        turnLeft();
                    }
                    if(dist > 200 && dist2 < 70 && dist3 > 40){
                        phase = 6;
                        forward();
                    }
                }
                if(pos == 4){
                    while(dist2 > 40 || dist3 < 70){
                        turnRight();
                    }
                    if(dist > 200 && dist2 > 40 && dist3 < 70){
                        phase = 6;
                        forward();
                    }
                }
            }

            if(phase == 6){ // Rijden en bomen detecteren
                if(post == 1){
                    if(dist > 5 && dist2 > 10){
                        forward();
                    } else {
                        stop();
                        if(dist < 5){
                        phase = 7;
                        }
                    }
                }
                if(pos == 2){
                    if(dist > 5 && dist2 > 10){
                        forward();
                    } else {
                        stop();
                        if(dist < 5){
                        phase = 7;
                        }
                    }
                }
                if(pos == 3){
                    if(dist > 5 && dist2 > 10){
                        forward();
                    } else {
                        stop();
                        if(dist < 5){
                        phase = 7;
                        }
                    }
                }
                if(pos == 4){
                    if(dist > 5 && dist2 > 10){
                        forward();
                    } else {
                        stop();
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
                        forward();
                    }
                }
                if(pos == 2){
                    while(dist2 < 200 || dist < 40){
                        turnLeft();
                    }
                    if(dist > 40 && dist2 < 200 && dist3 < 5){
                        phase = 8;
                        forward();
                    }
                }
                if(pos == 3){
                    while(dist3 < 200 || dist < 40){
                        turnRight();
                    }
                    if(dist < 40 && dist2 < 5 && dist3 < 200){
                        phase = 8;
                        forward();
                    }
                }
                if(pos == 4){
                    while(dist3 < 200 || dist < 70){
                        turnRight();
                    }
                    if(dist < 70 && dist2 < 5 && dist3 < 200){
                        phase = 8;
                        forward();
                    }
                }
            }

            if(phase == 8){ // Rijden naar de juiste positie
                if(pos == 1){
                    if(dist > 47){
                        forward();
                    } else {
                        phase = 9;
                    }
                }
                if(pos == 2){
                    if(dist > 5){
                        forward();
                    } else {
                        phase = 9;
                    }
                }
                if(pos == 3){
                    if(dist > 5){
                        forward();
                    } else {
                        phase = 9;
                    }
                }
                if(pos == 4){
                    if(dist > 5){
                        forward();
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
                        forward();
                    }
                }
                if(pos == 2){
                    while(dist2 < 100 || dist < 200){
                        turnLeft();
                    }
                    if(dist > 200 && dist2 > 100 && dist3 < 5){
                        phase = 10;
                        forward();
                    }
                }
                if(pos == 3){
                    while(dist3 > 100 || dist < 200){
                        turnRight();
                    }
                    if(dist > 200 && dist2 > 100 && dist3 < 5){
                        phase = 10;
                        forward();
                    }
                }
                if(pos == 4){
                    while(dist3 > 100 || dist < 200){
                        turnRight();
                    }
                    if(dist > 200 && dist2 < 5 && dist3 < 100){
                        phase = 10;
                        forward();
                    }
                }
            }

            if(phase == 10){ // Rij uitrijden en bomen detecteren
                if(post == 1){
                    if(dist > 5 && dist3 > 10){
                        forward();
                    } else {
                        stop();
                        if(dist < 5){
                        phase = 0;
                        }
                    }
                }
                if(pos == 2){
                    if(dist > 5 && dist2 > 10){
                        forward();
                    } else {
                        stop();
                        if(dist < 5){
                        phase = 0;
                        }
                    }
                }
                if(pos == 3){
                    if(dist > 5 && dist3 > 10){
                        forward();
                    } else {
                        stop();
                        if(dist < 5){
                        phase = 0;
                        }
                    }
                }
                if(pos == 4){
                        if(dist > 5 && dist3 > 10){
                        forward();
                    } else {
                        stop();
                        if(dist < 5){
                        phase = 0;
                        }
                    }
                }
            }
        } else {
            stop();
        }
    }
}

int forward(void){
    PORTH |= (1<<Motor_Len1);
    PORTH &= ~(1<<Motor_Len2);
    PORTH |= (1<<Motor_Ren1);
    PORTH &= ~(1<<Motor_Ren2);
}

int turnRight(void){
    PORTH &= ~(1<<Motor_Len1);
    PORTH |= (1<<Motor_Len2);
    PORTH |= (1<<Motor_Ren1);
    PORTH &= ~(1<<Motor_Ren2);
}

int turnLeft(void){
    PORTH |= (1<<Motor_Len1);
    PORTH &= ~(1<<Motor_Len2);
    PORTH &= ~(1<<Motor_Ren1);
    PORTH |= (1<<Motor_Ren2);
}

int stop(void){
    PORTH &= ~(1<<Motor_Len1);
    PORTH &= ~(1<<Motor_Len2);
    PORTH &= ~(1<<Motor_Ren1);
    PORTH &= ~(1<<Motor_Ren2);
}
