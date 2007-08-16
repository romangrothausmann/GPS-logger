#define ENC_1_PORT PIND
#define ENC_1_PIN PIND4 //push
#define ENC_2_PORT PIND
#define ENC_2_PIN PIND6
#define ENC_3_PORT PIND
#define ENC_3_PIN PIND5

volatile char inc_push= 0, pressed= 0; //, tov;
volatile int inc_lr= 0;

void dreh_init(void){
    TCCR2|= (3 << CS20); // clock/64, 256 is too rarely
    TIMSK|= (1 << TOIE2);
    }


ISR(TIMER2_OVF_vect){
    static uint8_t last_state = 0,last_cnt = 0, halbdreh=0;
    uint8_t new_state;

//add var released
//implement separate counter for pressed state

    new_state=0;
    if (!(ENC_2_PORT & (1<<ENC_2_PIN))) new_state |= (1<<ENC_2_PIN);
    if (!(ENC_3_PORT & (1<<ENC_3_PIN))) new_state |= (1<<ENC_3_PIN);
  

    if ((new_state ^ last_cnt)==(_BV(ENC_2_PIN) | _BV(ENC_3_PIN)) )
        {
        if ((new_state ^ last_state)==_BV(ENC_3_PIN)){
            if (halbdreh==1){
	  	inc_lr+=1;
		halbdreh=0;
		}
            else halbdreh=1;
            }
        else{
            if (halbdreh==1){
	  	inc_lr-=1;
		halbdreh=0;
		}
            else halbdreh=1;
            }
        last_cnt=new_state;
        }
    last_state=new_state;


    if (ENC_1_PORT & (1 << ENC_1_PIN)) 
        pressed=0;

    if (pressed==0){
        if (inc_push==0){
            if (!(ENC_1_PORT & (1 << ENC_1_PIN))){
                pressed=1;
                inc_push=1;
                }
            }
        }
    
    //tov= 1;
    
    }
