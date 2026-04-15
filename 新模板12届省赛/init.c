#include "init.h"
#include "ch138.h"

void SysInit(){
    W_CH138(0x80,0xff);
    W_CH138(0xa0,0x00);
}