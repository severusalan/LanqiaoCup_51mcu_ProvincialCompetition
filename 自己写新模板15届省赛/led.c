#include "led.h"
#include "ch138.h"

unsigned char t0=0x00;
unsigned char t00=0xff;

void Led_Disp(unsigned char *ucLed){
    t0=(ucLed[0]<<0)|(ucLed[1]<<1)|(ucLed[2]<<2)|(ucLed[3]<<3)|(ucLed[4]<<4)|(ucLed[5]<<5)|(ucLed[6]<<6)|(ucLed[7]<<7);
    if(t0!=t00){
        W_CH138(0x80,~t0);
        t00=t0;
    }
}

void Led_Off(){
    W_CH138(0x80,0xff);
    t00=0x00;
}

unsigned char t1=0x00;
unsigned char t11=0xff;

void Beep(bit enable){
    if(enable) t1|=0x40;
    else t1&=~0x40;
    if(t1!=t11){
        W_CH138(0xa0,t1);
        t11=t1;
    }
}

void Relay(bit enable){
    if(enable) t1|=0x10;
    else t1&=~0x10;
    if(t1!=t11){
        W_CH138(0xa0,t1);
        t11=t1;
    }
}

void Motor(bit enable){
    if(enable) t1|=0x20;
    else t1&=~0x20;
    if(t1!=t11){
        W_CH138(0xa0,t1);
        t11=t1;
    }
}