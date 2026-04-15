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
#include "math.h"

#define PARA_MIN 20
#define PARA_MAX 80

unsigned char KD,KU,KV,KO;
unsigned char KeySlow;
idata unsigned int SegSlow;

idata unsigned char Seg_Buf[8]={16,16,16,16,16,16,16,16};
idata unsigned char Seg_Pos;
idata unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
idata unsigned char ucRtc[3]={0,0,0};

idata unsigned char U_Rx[10];
idata unsigned char U_Flag;
idata unsigned char U_Tick;
idata unsigned char U_Index;

//温度
idata unsigned char Temperature_Value;
idata unsigned int Temperature_Slow_Down;
//光强等级
idata unsigned char Light_Level = 1;
idata unsigned char AD_Slow_Down;
//距离
idata unsigned char Motion_State; //运动状态 0 静止 1 徘徊 2 跑动
idata unsigned char Distance_Value;
idata unsigned char Distance_Value_Old; 
idata unsigned int Distance_Slow_Down;
idata bit State_Change_Flag; //状态改变标志
idata unsigned int Change_Time_3s; //3s计时器
//参数
idata unsigned char Temperature_Para = 30;
idata unsigned char Distance_Para = 30;

idata bit Hot_Flag; //高温判定
idata bit Close_Flag; //接近标志

idata bit Long_Press_Flag; //长按检测标志
idata unsigned int Long_Press_Time_2S; //2S计时器

idata bit Relay_Led_Lock; //状态锁定
idata unsigned int Relay_Count; //继电器吸合次数

idata bit First_Get_Distance; //第一次获取距离标志
idata bit Relay_Flag; //继电器标志位
//显示
idata unsigned char Seg_Show_Mode; //0 环境状态 1 运动监测 2 参数设置 3 统计数据
idata bit Para_Show_Mode; //0 温度参数 1 距离参数

idata bit Running_State; //跑动的状态标志
idata bit Stay_State; //静止状态
idata bit Paralysis_State; //徘徊状态
idata bit Led_Blink_Flag; //LED闪烁标志
idata unsigned char Blink_Time_100ms;

void Key_Proc(){
    if(KeySlow<10) return;
    KeySlow=0;

    KV=Key_Read();
    KD=KV&(KO^KV);
    KU=~KV&(KO^KV);
    KO=KV;

    /* 在这里写按键逻辑 */
    if(KD==4)
    {
        Relay_Led_Lock = (Seg_Show_Mode==1); //判断逻辑为S4按下 则在参数界面前 运动监测状态下进行逻辑判断
        Para_Show_Mode = 0; //每次切换到参数界面 默认为温度参数子界面
        Seg_Show_Mode = (++Seg_Show_Mode)%4; //切换显示模式
    }

    //如果处于参数界面
    if(Seg_Show_Mode==2)
    {
        if(KD==5)
        {
            Para_Show_Mode = ~Para_Show_Mode; //切换参数界面显示的参数
        }
        if(Para_Show_Mode == 0)
        {
            if(KD==8 && Temperature_Para < PARA_MAX)
            {
                Temperature_Para++; //温度参数加
            }
            if(KD==9 && Temperature_Para > PARA_MIN)
            {
                Temperature_Para--; //温度参数减
            }
        }
        else
        {
            if(KD==8 && Distance_Para < PARA_MAX)
            {
                Distance_Para = Distance_Para+5; //距离参数加
            }
            if(KD==9 && Distance_Para >PARA_MIN)
            {
                Distance_Para = Distance_Para-5; //距离参数减
            }
        }
    }
    if(Seg_Show_Mode == 3)
    {
        if(Long_Press_Time_2S>=2000) //长按时间保持两秒以上
        {
            Relay_Count = 0; //重置统计数据
        }
        if(KO == 89)
        {
            Long_Press_Flag = 1; //长按标志位置位
        }
        else
        {
            Long_Press_Flag = 0; //长按标志位清零
            Long_Press_Time_2S = 0;
        }
    }
}

void Seg_Proc(){
    if(SegSlow<200) return;
    SegSlow=0;
    switch(Seg_Show_Mode)
    {
        case 0: //环境状态
            Seg_Buf[0] = 12; //C
            Seg_Buf[1] = Temperature_Value/10%10;
            Seg_Buf[2] = Temperature_Value%10;
            Seg_Buf[3] = 16;
            Seg_Buf[4] = 16;
            Seg_Buf[5] = 16;
            Seg_Buf[6] = 17; //N
            Seg_Buf[7] = Light_Level; //光强等级
        break;
        case 1: //运动检测
            Seg_Buf[0] = 18; //L
            Seg_Buf[1] = Motion_State;
            Seg_Buf[2] = 16;
            Seg_Buf[3] = 16;
            Seg_Buf[4] = 16;
            Seg_Buf[5] = Distance_Value/100%10;
            Seg_Buf[6] = Distance_Value/10%10;
            Seg_Buf[7] = Distance_Value%10; 
        break;
        case 2: //参数设置界面
            Seg_Buf[0] = 19; //P
            Seg_Buf[2] = 16;
            Seg_Buf[3] = 16;
            Seg_Buf[4] = 16;
            Seg_Buf[5] = 16;
            switch((unsigned char)Para_Show_Mode)
            {
                case 0: //温度参数
                    Seg_Buf[1] = 12; //C
                    Seg_Buf[6] = Temperature_Para/10%10;
                    Seg_Buf[7] = Temperature_Para%10; 
                break;
                case 1: //距离参数
                    Seg_Buf[1] = 18; //L
                    Seg_Buf[6] = Distance_Para/10%10;
                    Seg_Buf[7] = Distance_Para%10; 
                break; 
            }
        break;
        case 3: //统计数据界面
            Seg_Buf[0] = 17; //N
            Seg_Buf[1] = 12; //C
            Seg_Buf[2] = 16;
            Seg_Buf[3] = 16;
            Seg_Buf[4] = (Relay_Count>=1000)?Relay_Count/1000%10:16;
            Seg_Buf[5] = (Relay_Count>=100)?Relay_Count/100%10:16;
            Seg_Buf[6] = (Relay_Count>=10)?Relay_Count/10%10:16;
            Seg_Buf[7] = Relay_Count%10;
        break;
    }

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
}

void Led_Proc(){
    /* 在这里写 LED / 蜂鸣器 / 继电器逻辑 */
    bit Temp_Relay_Flag;
    //继电器和LED没有锁定时
    if(!Relay_Led_Lock)
    {
        ucLed[0] = (Light_Level>=1);
        ucLed[1] = (Light_Level>=2);
        ucLed[2] = (Light_Level>=3);
        ucLed[3] = (Light_Level>=4);
        Temp_Relay_Flag = (Close_Flag&&Hot_Flag);
        if((Relay_Flag == 0)&&(Temp_Relay_Flag == 1)) Relay_Count++; //继电器吸合次数统计
        Relay_Flag = Temp_Relay_Flag;
        Relay(Relay_Flag); //当接近且高温时吸合继电器
    }
    if(Stay_State) ucLed[7] = 0;
    else if(Paralysis_State) ucLed[7] = 1;
    else if(Running_State) ucLed[7] = Led_Blink_Flag;
    Led_Disp(ucLed);
}

void Uart_Proc(){
    if(U_Index==0) return;
    if(U_Tick<10) return;

    U_Flag=0;
    U_Tick=0;

    /* 在这里写串口协议处理逻辑 */

    U_Index=0;
}

void Uart1Isr() interrupt 4
{
    if(RI==1){
        U_Flag=1;
        U_Tick=0;
        U_Rx[U_Index]=SBUF;
        RI=0;

        if(++U_Index>=10){
            U_Index=0;
        }
    }
}

void Get_AD() //获取光强等级
{
    unsigned char temp;
    if(AD_Slow_Down<100) return;
    AD_Slow_Down = 0;
    temp = Ad_Read(0x01);
    if(temp>=3*51)Light_Level = 1;
    else if(temp>=2*51)Light_Level = 2;
    else if(temp>=0.5*51)Light_Level = 3;
    else Light_Level = 4;
}

void Get_Temperature() //获取温度值
{
    float temp;
    if(Temperature_Slow_Down<300) return;
    Temperature_Slow_Down = 0;
    temp = rd_temperature();
    Temperature_Value = temp;
    Hot_Flag = (temp > Temperature_Para);
}

void Get_Distance() //移动检测
{
    unsigned char D_Distance; //两次测量的差值
    unsigned char Temp_Motion_State; //临时运动状态
    if(Distance_Slow_Down<999) return;
    Distance_Slow_Down = 0;
    //并不是第一次获取
    if(First_Get_Distance == 1)
    {
        Distance_Value = Ut_Data();
        D_Distance = abs(Distance_Value_Old - Distance_Value);
        Distance_Value_Old = Distance_Value;
        Close_Flag = (Distance_Value < Distance_Para); //如果距离小于设定的距离参数 则认为接近
        //状态未锁定
        if(!State_Change_Flag)
        {
            if(D_Distance < 5)Temp_Motion_State = 0;
            else if(D_Distance < 10)Temp_Motion_State = 1;
            else Temp_Motion_State = 2;
            State_Change_Flag = (Temp_Motion_State!=Motion_State); //如果状态改变则状态改变标志位置位
            Motion_State = Temp_Motion_State; //更新运动状态
        }
        //如果LED和继电器没有锁定
        if(!Relay_Led_Lock)
        {
            Running_State = (Motion_State==2); //如果处于跑动状态 则跑动状态标志位置位 否则清零
            Stay_State = (Motion_State==0); //如果处于静止状态 则静止状态标志位置位 否则清零
            Paralysis_State = (Motion_State==1); //如果处于徘徊状态 则徘徊状态标志位置位 否则清零
        }
    }
    else //第一次获取
    {
        First_Get_Distance = 1;
        Distance_Value = Ut_Data();
        Distance_Value_Old = Distance_Value;
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
    AD_Slow_Down++;
    Temperature_Slow_Down++;
    Distance_Slow_Down++;

    if(++Seg_Pos==8) Seg_Pos=0;

    if(Seg_Buf[Seg_Pos]>20) Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
    else Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);

    Led_Disp(ucLed);

    if(U_Flag==1){
        U_Tick++;
    }

    if(Long_Press_Flag) //如果处于长按
    {
        if(++Long_Press_Time_2S>2000)
        Long_Press_Time_2S = 2001;
    }
    else //如果没有长按
    {
        Long_Press_Time_2S = 0;
    }

    //如果处于跑动状态
    if(Running_State)
    {
        if(++Blink_Time_100ms>=100) //每100ms切换一次LED亮灭状态
        {
            Blink_Time_100ms = 0;
            Led_Blink_Flag ^= 1;
        }
    }
    else
    {
        Blink_Time_100ms = 0;
        Led_Blink_Flag = 0;
    }

    //如果状态锁定
    if(State_Change_Flag)
    {
        if(++Change_Time_3s==3000)
        {
            Change_Time_3s = 0;
            State_Change_Flag = 0; //状态锁定时间到，清除状态改变标志位
        }
    }
    else Change_Time_3s = 0;
}

void main(){
    SysInit();
    Timer1Init();
    UartInit();

    while(1){
        Key_Proc();
        Seg_Proc();
        Led_Proc();
        Uart_Proc();
        Get_Temperature();
        Get_AD();
        Get_Distance();
    }
}
