#include <STC15F2K60S2.H>
#include "init.h"
#include "key.h"
#include "led.h"
#include "seg.h"
#include "iic.h"
#include "onewire.h"
#include "ds1302.h"
#include "uart.h"
#include "ultrawave.h"

unsigned char KD,KU,KV,KO;
unsigned char KeySlow;
idata unsigned int SegSlow;

idata unsigned char Seg_Buf[8]={16,16,16,16,16,16,16,16};
idata unsigned char Seg_Pos;
idata unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
idata unsigned char ucRtc[3]={23,25,0};

idata unsigned char Seg_Proc_Mode; //0-温度显示 1-时间显示 2-参数设置
float Temperature; //实时温度值
unsigned char Para_Temperature = 23; //温度参数值

unsigned int Led_5s_Count = 0; //LED5s计数器
unsigned int Led_01s_Count = 0; //LED0.1s计数器
unsigned int JDQ_5s_Count = 0; //继电器5s计数器

bit Work_Mode; //0-温度控制 1-时间控制
bit Time_Mode; //0-显示时分 1-显示分秒
bit WDJDQ_Mode; //0-继电器断开 1-继电器闭合
bit SJJDQ_Mode; //0-继电器断开 1-继电器闭合
bit Led1_Mode; //0-LED1灭 1-LED1亮
bit Led3_Mode; //0-LED3灭 1-LED3亮

void Key_Proc(){
    if(KeySlow<10) return;
    KeySlow=0;

    KV=Key_Read();
    KD=KV&(KO^KV);
    KU=~KV&(KO^KV);
    KO=KV;

    /* 在这里写按键逻辑 */
    switch(KD)
    {
        case 12:
            Seg_Proc_Mode++;
            if(Seg_Proc_Mode>2) Seg_Proc_Mode=0;
            break;
        case 13:
            Work_Mode=!Work_Mode;
            break;
        case 16:
            if(Seg_Proc_Mode==2)
            {
                Para_Temperature++;
                if(Para_Temperature > 99)Para_Temperature=99;
            }
            break;
        case 17:
            if(Seg_Proc_Mode==2)
            {
                Para_Temperature--;
                if(Para_Temperature < 10)Para_Temperature=10;
            }
            if(Seg_Proc_Mode==1)
            {
                Time_Mode=1;
            }
            break;
    }
    if(Seg_Proc_Mode==1 && KU==17)
    {
        Time_Mode=0;
    }
}

void Seg_Proc(){
    if(SegSlow<200) return;
    SegSlow=0;

    Read_Rtc(ucRtc);
    Temperature = rd_temperature();

    /* 在这里写数码管缓冲区赋值逻辑 */
    /*
    Seg_Buf[0]=16;
    Seg_Buf[1]=16;
    Seg_Buf[2]=16;
    Seg_Buf[3]=16;
    Seg_Buf[4]=16;
    Seg_Buf[5]=16;
    Seg_Buf[6]=16;
    Seg_Buf[7]=16;
    */
   switch(Seg_Proc_Mode){
       case 0:
           /* 温度显示 */
            Seg_Buf[0] = 17;
            Seg_Buf[1] = 1;
            Seg_Buf[2] = 16;
            Seg_Buf[3] = 16;
            Seg_Buf[4] = 16;
            Seg_Buf[5] = (unsigned char)Temperature/10%10;
            Seg_Buf[6] = (unsigned char)Temperature%10 + ',';
            Seg_Buf[7] = (unsigned int)(Temperature * 10)%10;
           break;
       case 1:
           /* 时间显示 */
           if(Time_Mode==0) //显示时分
           {
               Seg_Buf[0] = 17;
               Seg_Buf[1] = 2;
               Seg_Buf[2] = 16;
               Seg_Buf[3] = ucRtc[0]/10%10;
               Seg_Buf[4] = ucRtc[0]%10;
               Seg_Buf[5] = 18;
               Seg_Buf[6] = ucRtc[1]/10%10;
               Seg_Buf[7] = ucRtc[1]%10;
           }
           else //显示分秒
           {
           Seg_Buf[0] = 17;
           Seg_Buf[1] = 2;
           Seg_Buf[2] = 16;
           Seg_Buf[3] = ucRtc[1]/10%10;
           Seg_Buf[4] = ucRtc[1]%10;
           Seg_Buf[5] = 18;
           Seg_Buf[6] = ucRtc[2]/10%10;
           Seg_Buf[7] = ucRtc[2]%10;
           }
           break;
       case 2:
           /* 参数设置 */
           Seg_Buf[0] = 17;
           Seg_Buf[1] = 3;
           Seg_Buf[2] = 16;
           Seg_Buf[3] = 16;
           Seg_Buf[4] = 16;
           Seg_Buf[5] = 16;
           Seg_Buf[6] = Para_Temperature/10%10;
           Seg_Buf[7] = Para_Temperature%10;
           break;
   }
}

void Led_Proc(){
    /* 在这里写 LED / 蜂鸣器 / 继电器逻辑 */
    if(Temperature > Para_Temperature)
    {
        WDJDQ_Mode = 1;
        Relay(WDJDQ_Mode);
    }
    else
    {
        WDJDQ_Mode = 0;
        Relay(WDJDQ_Mode);
    }

    if(ucRtc[1] == 0 && ucRtc[2] == 0)
    {
        SJJDQ_Mode = 1;
        Relay(SJJDQ_Mode);
    }

    if(ucRtc[1] == 0 && ucRtc[2] == 0)
    {
        Led1_Mode = 1;
        ucLed[0] = Led1_Mode;
    }

    if(Work_Mode == 0)
    {
        ucLed[1] = 1;
    }
    else
    {
        ucLed[1] = 0;
    }

    if(WDJDQ_Mode == 1 || SJJDQ_Mode == 1)
    {
        Led3_Mode = 1;
        ucLed[2] = Led3_Mode;
    }
}

void Timer1Init(void)		//1毫秒@12.000MHz
{
    AUXR &= 0xBF;
    TMOD &= 0x0F;
    TL1 = 0x18;
    TH1 = 0xFC;
    TF1 = 0;
    TR1 = 1;
    ET1 = 1;
    EA = 1;
}

void Timer1Isr() interrupt 3
{
    KeySlow++;
    SegSlow++;

    if(++Seg_Pos==8) Seg_Pos=0;

    if(Seg_Buf[Seg_Pos]>20) Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
    else Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);

    Led_Disp(ucLed);

    if(SJJDQ_Mode == 1)
    {
        JDQ_5s_Count++;
        if(JDQ_5s_Count >= 5000)
        {
            SJJDQ_Mode = 0;
            JDQ_5s_Count = 0;
        }
    }
    
    if(Led1_Mode == 1)
    {
        Led_5s_Count++;
        if(Led_5s_Count >= 5000)
        {
            Led1_Mode = 0;
            Led_5s_Count = 0;
        }
    }
    if(Led3_Mode == 1)
    {
        Led_01s_Count++;
        if(Led_01s_Count >= 100)
        {
            Led3_Mode = 0;
            Led_01s_Count = 0;
        }
    }
}

void Delay750ms(void)	//@12.000MHz 延时
{
	unsigned char data i, j, k;

	i = 35;
	j = 51;
	k = 182;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}


void main(){
    rd_temperature();
    Delay750ms();
    SysInit();
    Timer1Init();
    Set_Rtc(ucRtc);

    while(1){
        Key_Proc();
        Seg_Proc();
        Led_Proc();
    }
}
