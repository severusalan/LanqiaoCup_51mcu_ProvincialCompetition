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
idata unsigned char ucRtc[3]={13,03,05}; //初始时间 13:03:05

idata unsigned char Max_Time[3];

idata unsigned char Seg_Disp_Mode; //0-频率界面 1-参数界面 2-时间界面 3-回显界面
unsigned int Timer_1s; //NE555 1s计时器
long int Freq; //NE555频率

unsigned char Set_Mode; //参数界面 0-超限参数界面 1-校准值参数界面
unsigned char Re_Disp_Mode; //回显界面显示模式 0-回显频率 1-回显时间 
unsigned int CX_Num = 2000; //超限参数
int JZ_Num = 0; //校准值参数
int ZZ_Freq; //最终频率（Freq + Max_Freq）
unsigned int Max_Freq; //最大频率值（频率回显界面）
unsigned int dat;

unsigned int LED_Blink_200ms; //LED闪烁计数器
bit Led_Flag; //LED状态标志 0-灭 1-亮

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
        case 4:
            Seg_Disp_Mode = (Seg_Disp_Mode + 1) % 4;
        break;
        case 5:
            if(Seg_Disp_Mode == 1)
            {
                Set_Mode ^= 1;
            }
            else if(Seg_Disp_Mode == 3)
            {
                Re_Disp_Mode ^= 1;
            }
        break;
        case 8:
            if(Seg_Disp_Mode == 1)
            {
                if(Set_Mode == 0)
                {
                    CX_Num += 1000;
                    if(CX_Num >= 9000)
                    {
                        CX_Num = 9000;
                    }
                }
                else if(Set_Mode == 1)
                    {
                        JZ_Num += 100;
                        if(JZ_Num >= 900)
                        {
                            JZ_Num = 900;
                        }
                    }
            }
        break;
        case 9:
            if(Seg_Disp_Mode == 1)
            {
               CX_Num -= 1000;
                if(CX_Num <= 1000)
                {
                    CX_Num = 1000;
                }
            }
            else if(Set_Mode == 1)
                {
                    JZ_Num -= 100;
                    if(JZ_Num <= -900)
                    {
                        JZ_Num = -900;
                    }
                }
        break;
    }
}

void Seg_Proc(){
    unsigned char i = 3;
    if(SegSlow<80) return;
    SegSlow=0;

    Read_Rtc(ucRtc);
    ZZ_Freq = Freq + JZ_Num;

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
   switch(Seg_Disp_Mode)
    {
        case 0: //频率界面
            Seg_Buf[0] = 15; //F
            Seg_Buf[1] = 16;
            Seg_Buf[2] = 16;
            if(ZZ_Freq >= 0)
            {
                Seg_Buf[3] = ZZ_Freq / 10000 % 10;
                Seg_Buf[4] = ZZ_Freq / 1000 % 10;
                Seg_Buf[5] = ZZ_Freq / 100 % 10;
                Seg_Buf[6] = ZZ_Freq / 10 % 10;
                Seg_Buf[7] = ZZ_Freq % 10;
                while(Seg_Buf[i] == 0)
                {
                    Seg_Buf[i] = 16;
                    if(i++ == 7) break;
                }
            }
            else
            {
                Seg_Buf[6] = 20; //L
                Seg_Buf[7] = 20; //L
                while(Seg_Buf[i] == 0)
                {
                    Seg_Buf[i] = 16;
                    if(i++ == 5) break;
                }
            }
            break;
        case 1: //参数界面
            Seg_Buf[0] = 17; //P
            if(Set_Mode == 0)
            {
                Seg_Buf[1] = 1; //超限参数界面
                Seg_Buf[2] = 16;
                Seg_Buf[3] = 16;
                Seg_Buf[4] = CX_Num / 1000 % 10;
                Seg_Buf[5] = CX_Num / 100 % 10;
                Seg_Buf[6] = CX_Num / 10 % 10;
                Seg_Buf[7] = CX_Num % 10;
                if(Seg_Buf[4] == 0)
                {
                    Seg_Buf[4] = 16;
                    if(Seg_Buf[5] == 0)
                    {
                        Seg_Buf[5] = 16;
                        if(Seg_Buf[6] == 0)
                        {
                            Seg_Buf[6] = 16;
                        }
                    }
                }
            }
            else if(Set_Mode == 1) //校准值参数界面
            {
                unsigned int Temp_JZ_Num;
                Seg_Buf[1] = 2;
                Seg_Buf[2] = 16;
                Seg_Buf[3] = 16;
                Seg_Buf[4] = 16;
                if(JZ_Num >= 0)
                {
                    Temp_JZ_Num = JZ_Num;
                }
                else
                {
                    Temp_JZ_Num = -JZ_Num;
                }
                Seg_Buf[5] = Temp_JZ_Num / 100 % 10;
                Seg_Buf[6] = Temp_JZ_Num / 10 % 10;
                Seg_Buf[7] = Temp_JZ_Num % 10;
                if(Seg_Buf[5] == 0)
                {
                    Seg_Buf[5] = 16;
                    if(Seg_Buf[6] == 0)
                    {
                        Seg_Buf[6] = 16;
                    }
                }
                if(JZ_Num < 0)
                {
                    if(Seg_Buf[5] != 16)
                    {
                        Seg_Buf[4] = 18;
                    }
                    else if(Seg_Buf[6] != 16)
                    {
                        Seg_Buf[5] = 18;
                    }
                    else
                    {
                        Seg_Buf[6] = 18;
                    }
                }
            }
        break;
        case 2: //时间界面
            Seg_Buf[0] = ucRtc[0] / 10 % 10;
            Seg_Buf[1] = ucRtc[0] % 10;
            Seg_Buf[2] = 18;
            Seg_Buf[3] = ucRtc[1] / 10 % 10;
            Seg_Buf[4] = ucRtc[1] % 10;
            Seg_Buf[5] = 18;
            Seg_Buf[6] = ucRtc[2] / 10 % 10;
            Seg_Buf[7] = ucRtc[2] % 10;
        break;
        case 3: //回显界面
            Seg_Buf[0] = 19; //H
            if(Re_Disp_Mode == 0) //回显频率
            {
                Seg_Buf[1] = 15; //F
                Seg_Buf[2] = 16;
                Seg_Buf[3] = Max_Freq / 10000 % 10;
                Seg_Buf[4] = Max_Freq / 1000 % 10;
                Seg_Buf[5] = Max_Freq / 100 % 10;
                Seg_Buf[6] = Max_Freq / 10 % 10;
                Seg_Buf[7] = Max_Freq % 10;
            }
            else if(Re_Disp_Mode ==1) //回显时间
            {
                Seg_Buf[1] = 10; //A
                Seg_Buf[2] = Max_Time[0] / 10 % 10;
                Seg_Buf[3] = Max_Time[0] % 10;
                Seg_Buf[4] = Max_Time[1] / 10 % 10;
                Seg_Buf[5] = Max_Time[1] % 10;
                Seg_Buf[6] = Max_Time[2] / 10 % 10;
                Seg_Buf[7] = Max_Time[2] % 10;
            }
    }
}

void Led_Proc(){
    /* 在这里写 LED / 蜂鸣器 / 继电器逻辑 */
    
    if(Seg_Disp_Mode == 0)
    {
        ucLed[0] = Led_Flag;
    }
    else
    {
        ucLed[0] = 0;
    }

    if(ZZ_Freq < 0)
    {
        ucLed[1] = 1;
    }
    else if(Freq > CX_Num)
    {
        ucLed[1] = Led_Flag;
    }
    else
    {
        ucLed[1] = 0;
    }

    //Da输出
    if(Freq <= 500)
    {
        dat = 51;
    }
    else if(Freq >= CX_Num)
    {
        dat = 255;
    }
    else
    {
        dat = 51+(204.0*(Freq - 500))/(CX_Num - 500);
    }
    Da_Write(dat);
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

void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
    TMOD |= 0x05;
	TL0 = 0;				//设置定时初始值
	TH0 = 0;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
    EA = 1;                //打开总中断
}

void Timer1Isr() interrupt 3
{
    KeySlow++;
    SegSlow++;

    if(++Seg_Pos==8) Seg_Pos=0;

    if(Seg_Buf[Seg_Pos]>20) Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
    else Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);

    Led_Disp(ucLed);

    if(++Timer_1s == 1000)
    {
        Timer_1s = 0;
        Freq = TH0 << 8 | TL0;
        TH0 = 0;
        TL0 = 0;
    }
    if(LED_Blink_200ms++ == 200)
    {
        LED_Blink_200ms = 0;
        Led_Flag = ~Led_Flag;
    }
}

void main(){
    SysInit();
    Timer1Init();
    UartInit();
    Timer0_Init();
    while(1){
        Key_Proc();
        Seg_Proc();
        Led_Proc();
    }
}
