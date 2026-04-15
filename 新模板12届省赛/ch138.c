#include "ch138.h"

void W_CH138(unsigned char addr,unsigned char dat){
    unsigned char temp;
    
    P0=dat;

    temp=P2&0x1f;
    temp=temp|addr;
    P2=temp;
    temp=P2&0x1f;
    P2=temp;
}