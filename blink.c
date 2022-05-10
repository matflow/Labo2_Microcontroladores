#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Definición de estados de la FSM
#define LDPV 0
#define PV_blink 1
#define LDVD 2
#define LDPP 3
#define PP_blink 4
#define LDPD 5

int estado;
int nxt_state;
int valid;
int boton;
int curr_delay = 61;  // 1 seg
int count = 0; // contador para interrupciones timer0
int blinks;

void fsm();

int main(void)
{
  // CONFIGURACIÓN
  //DDRB |= (1 << PB3)|(1 << PB2)|(1 << PB1) |(1 << PB0);
  DDRB = 0xFB; // Todos los puertos como salidas excepto PB2

  // Interrupciones de delay config
  TCCR0A = 0x00; // modo normal de puerto y modo normal counter
  TCCR0B = 0x00; // modo normal de counter
  TCCR0B |= (1 << CS02)|(1 << CS00); // prescaler f/1024

  sei(); // habilitar interrupción global
  TCNT0 = 0;  // empezar counter0 en 0
  TIMSK |= (1 << TOIE0); // habilitar timer0 overflow interrupt

  // Interrupciones de pin change config
  GIMSK |= (1 << INT0)|(1 << INT1); // habilitar "external interrupt 0"
  //PCMSK |= (1 << PCINT0);  // cambio en pin PB0
  MCUCR = 0x02; // interrupt en flanco neg.

  // INICIALIZACIÓN
  PORTB = 0x00;
  boton = 0;
  valid = 0;
  blinks = 0;
  estado = LDPV;  // estado inicial es el paso de vehículos
  nxt_state = LDPV;
  curr_delay = 210;

  while(1){
      estado = nxt_state;
      fsm();
    
  }
}

void fsm(){
    switch (estado)
    {
    case LDPV:      // paso vehículos
        PORTB |= (1 << PB3);
        PORTB |= (1 << PB5)|(1 << PB7); // LDPD encendido
        PORTB &= ~(1 << PB1);  // LDVD apagado
        curr_delay = 610;
        if (boton == 1 && blinks > 0){
            nxt_state = PV_blink;
            count = 0;
            blinks = 0;
        } 
        break;
    case PV_blink:      // paso vehículos apunto de terminar
        curr_delay = 30;  // parpadeo intermitente
        boton = 0;
        if (valid){
            PORTB ^= (1 << PB3);
        }
        if (blinks == 6){       // al pasar 3 parpadeos (6 cambios) de luz verde
            nxt_state = LDVD;
            count = 0;
            blinks = 0;
        }
        break;
    case LDVD:      // vehículos detenidos
        curr_delay = 61;
        PORTB &= ~(1 << PB3);
        PORTB |= (1 << PB1);
        if (blinks == 1){
            count = 0;
            blinks = 0;
            nxt_state = LDPP;
        }
        break;
    case LDPP:      // paso peatones
        curr_delay = 610;
        PORTB |= (1 << PB4)|(1 << PB6);
        PORTB &= ~(1 << PB5);
        PORTB &= ~(1 << PB7);  // LDPD apagada
        if (blinks == 1){
            nxt_state = PP_blink;
            count = 0;
            blinks = 0;
        }
        break;
    case PP_blink:      // paso peatones apunto de terminar
        curr_delay = 30;
        if (valid){
            PORTB ^= (1 << PB4)|(1 << PB6);
            //PORTB ^= (1 << PB6);
        }
        if (blinks == 6){
            nxt_state = LDPD;
            count = 0;
            blinks = 0;
        }
        break;
    case LDPD:
        curr_delay = 61;
        PORTB &= ~(1 << PB4);
        PORTB &= ~(1 << PB6);
        PORTB |= (1 << PB5)|(1 << PB7); // luces peatones en rojo
        if (blinks == 1){
            nxt_state = LDPV;
            count = 0;
            blinks = 0;
        }
        break;
    default:
        break;
    }
}

ISR(TIMER0_OVF_vect){
    if(count == curr_delay){
        valid = 1;  // pasó tiempo necesario y se puede cambiar luz
        blinks++;   // cantidad de cambios de luz ++
        count = 0;
    } else{
        count++;
        valid = 0;  // no cambiar luz
    }
}


ISR(INT0_vect){
    boton = 1;
    //PORTB ^= (1 << PB1);
}

ISR(INT1_vect){
    boton = 1;
    //PORTB ^= (1 << PB1);
}