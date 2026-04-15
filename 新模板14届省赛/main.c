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
idata unsigned char ucRtc[3]={0,0,0};

idata unsigned char U_Rx[10];
idata unsigned char U_Flag;
idata unsigned char U_Tick;
idata unsigned char U_Index;

unsigned char ucRtc[3] = {13, 3, 5};
unsigned int time_1s; //1秒计时器
unsigned int time_3s; //3秒计时器

idata unsigned char Seg_Show_Mode; //0-时间界面 1-回显界面 2-参数界面
idata unsigned char Re_Show_Mode; //（回显状态） 0-温度 1-湿度 2-时间

/*数据*/
unsigned char Max_Temperature; //最大温度
unsigned int Aver_Temperature_10x; //平均温度的10倍，避免使用浮点数
unsigned char Max_Humidity; //最大湿度
unsigned int Aver_Humidity_10x; //平均湿度的10倍，避免使用浮点数
unsigned char Trigger_Count; //触发次数
unsigned char Trigger_Time[2]; //触发时间
unsigned char Para_Temperature; //温度参数
unsigned int freq; //NE555脉冲频率
unsigned char Old_Light_Value; //上一次的光照强度
bit Trigger_Flag; //触发标志

/*湿度计算函数（新增）*/
float rd_humidity(void)
{
    float humidity_value;
    if(freq<200||freq>2000)
        return 0;
    humidity_value = (freq - 200) * 2 / 45 + 10;
    return humidity_value;
}

void Key_Proc(){
    if(KeySlow<10) return;
    KeySlow=0;

    KV=Key_Read();
    KD=KV&(KO^KV);
    KU=~KV&(KO^KV);
    KO=KV;

    /* 在这里写按键逻辑 */
}

void Seg_Proc()
{
    unsigned int Temp_Temperature_10x;
    unsigned int Temp_Humidity_10x;
    unsigned char Temp_Light;

    if(SegSlow<200) return;
    SegSlow=0;

   Old_Light_Value = Ad_Read(0x01); //读取光照强度
   if(Old_Light_Value>50 && Temp_Light<20)
    {
        Trigger_Flag = 1;
        else if(Trigger_Flag == 1 && time_3s >= 3000)//被触发且达到三秒
        {
            Trigger_Flag = 0;
            time_3s = 0;u
        }
        if(Trigger_Flag)
            Seg_Buf[0] = 14; //E
            Temp_Temperature_10x = rd_temperature() * 10;
            Temp_Humidity_10x = rd_humidity() * 10;
            Seg_Buf[1] = 16;
            Seg_Buf[2] = 16;
            Seg_Buf[3] = Temp_Temperature_10x / 10 % 10;
            Seg_Buf[4] = Temp_Temperature_10x % 10;
            Seg_Buf[5] = 17;
            if(Temp_Humidity_10x == 0) //采到无效数据
            {
                Seg_Buf[6] = 10; //A
                Seg_Buf[7] = 10; //A
            }
            else //采到有效数据
            {
                Seg_Buf[6] = Temp_Humidity_10x / 10 % 10;
                Seg_Buf[7] = Temp_Humidity_10x % 10;
                if(++Trigger_Count == 100) Trigger_Count = 99;
                Max_Temperature = (Max_Temperature > Temp_Temperature_10x) ? Max_Temperature : Temp_Temperature_10x;
                Max_Humidity = (Max_Humidity > Temp_Humidity_10x) ? Max_Humidity : Temp_Humidity_10x;
                Aver_Temperature_10x = (Aver_Temperature_10x * (Trigger_Count - 1) + Temp_Temperature_10x) / Trigger_Count;
                Aver_Humidity_10x = (Aver_Humidity_10x * (Trigger_Count - 1) + Temp_Humidity_10x) / Trigger_Count;
            }
        }
        else
        {
                    switch(Seg_Show_Mode)
                {
                case 0: //时间界面
                    Seg_Buf[0] = ucRtc[0] / 10 % 10;
                    Seg_Buf[1] = ucRtc[0] % 10;
                    Seg_Buf[2] = 17;
                    Seg_Buf[3] = ucRtc[1] / 10 % 10;
                    Seg_Buf[4] = ucRtc[1] % 10;
                    Seg_Buf[5] = 17;
                    Seg_Buf[6] = ucRtc[2] / 10 % 10;
                    Seg_Buf[7] = ucRtc[2] % 10;
                break;
                case 1: //回显界面
                    switch(Re_Show_Mode)
                    {
                        case 0: //温度
                            Seg_Buf[0] = 12; //C
                            Seg_Buf[1] = 16;
                            if(Trigger_Count==0){
                                Seg_Buf[2] = 16;
                                Seg_Buf[3] = 16;
                                Seg_Buf[4] = 16;
                                Seg_Buf[5] = 16;
                                Seg_Buf[6] = 16;
                                Seg_Buf[7] = 16;
                            }
                            else{
                                Seg_Buf[2] = Max_Temperature / 10 % 10;
                                Seg_Buf[3] = Max_Temperature % 10;
                                Seg_Buf[4] = 17;
                                Seg_Buf[5] = Aver_Temperature_10x / 100 % 10;
                                Seg_Buf[6] = Aver_Temperature_10x / 10 % 10 + ',';
                                Seg_Buf[7] = Aver_Temperature_10x % 10;
                            }
                        break;
                        case 1: //湿度
                            Seg_Buf[0] = 18; //H
                            Seg_Buf[1] = 16;
                            if(Trigger_Count==0){
                                Seg_Buf[2] = 16;
                                Seg_Buf[3] = 16;
                                Seg_Buf[4] = 16;
                                Seg_Buf[5] = 16;
                                Seg_Buf[6] = 16;
                                Seg_Buf[7] = 16;
                            }
                            else{
                            Seg_Buf[2] = Max_Humidity / 10 % 10;
                            Seg_Buf[3] = Max_Humidity % 10;
                            Seg_Buf[4] = 17;
                            Seg_Buf[5] = Aver_Humidity_10x / 100 % 10;
                            Seg_Buf[6] = Aver_Humidity_10x / 10 % 10 + ',';
                            Seg_Buf[7] = Aver_Humidity_10x % 10;
                            }
                        break;
                        case 2: //时间
                            Seg_Buf[0] = 15; //F
                            Seg_Buf[1] = Trigger_Count / 10 % 10;
                            Seg_Buf[2] = Trigger_Count % 10;
                            Seg_Buf[3] = Trigger_Count == 0 ? 16 : Trigger_Time[0] / 10 % 10;
                            Seg_Buf[4] = Trigger_Count == 0 ? 16 : Trigger_Time[0] % 10;
                            Seg_Buf[5] = Trigger_Count == 0 ? 16 : 17;
                            Seg_Buf[6] = Trigger_Count == 0 ? 16 : Trigger_Time[1] / 10 % 10;
                            Seg_Buf[7] = Trigger_Count == 0 ? 16 : Trigger_Time[1] % 10;
                    }
                break;
                case 2: //参数界面
                    Seg_Buf[0] = 19; //P
                    Seg_Buf[1] = seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = seg_Buf[5] = 16;
                    Seg_Buf[6] = Para_Temperature / 10 % 10;
                    Seg_Buf[7] = Para_Temperature % 10;
                break;
                }
            }
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
 
    Read_Rtc(ucRtc);
}

void Led_Proc(){
    /* 在这里写 LED / 蜂鸣器 / 继电器逻辑 */
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

    if(U_Flag==1){
        U_Tick++;
    }

    if(++time_1s == 1000)
    {
        time_1s = 0;
        freq = TH0 << 8 | TL0; //读取频率计数器的值
        TH0 = 0;
        TL0 = 0;
    }
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
    }
}
