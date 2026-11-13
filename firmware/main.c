#include <REGX52.H>

/* ===================== 引脚定义 ===================== */

/* PCF8591 I2C */
sbit I2C_SCL = P0^6;
sbit I2C_SDA = P0^7;

/* 步进电机 P0.0~P0.3 */
sbit M1 = P0^0;
sbit M2 = P0^1;
sbit M3 = P0^2;
sbit M4 = P0^3;

/* RGB：共阴极，高电平亮 */
sbit RGB_R = P3^5;
sbit RGB_G = P3^6;
sbit RGB_B = P3^7;

/* 按键 */
sbit KEY_RGB = P3^2;

/* PCF8591地址 */
#define PCF8591_WRITE 0x90
#define PCF8591_READ  0x91

/* ===================== 全局变量 ===================== */

unsigned char rgb_color = 0;
unsigned char uart_mode = 0;

unsigned char light_value = 0;

unsigned char flow_led = 0x01;

unsigned char traffic_state = 0;
unsigned int traffic_time = 0;

unsigned int flow_time = 0;
unsigned int motor_time = 0;
unsigned int light_time = 0;

unsigned char motor_step = 0;

unsigned char motor_table[4] = {
    0x01,
    0x02,
    0x04,
    0x08
};

/* ===================== 延时 ===================== */

void DelayUs(unsigned int t)
{
    while(t--);
}

void DelayMs(unsigned int ms)
{
    unsigned int i, j;

    for(i = 0; i < ms; i++)
    {
        for(j = 0; j < 120; j++);
    }
}

/* ===================== 串口 ===================== */

void UART_Init(void)
{
    SCON = 0x50;      // 串口方式1，允许接收

    TMOD &= 0x0F;
    TMOD |= 0x20;     // 定时器1方式2，给串口提供波特率

    TH1 = 0xFD;       // 11.0592MHz，9600bps
    TL1 = 0xFD;

    TR1 = 1;
}

void UART_SendByte(unsigned char dat)
{
    SBUF = dat;
    while(TI == 0);
    TI = 0;
}

void UART_SendString(char *s)
{
    while(*s)
    {
        UART_SendByte(*s++);
    }
}

void UART_SendNumber(unsigned int num)
{
    UART_SendByte(num / 100 + '0');
    UART_SendByte(num % 100 / 10 + '0');
    UART_SendByte(num % 10 + '0');
}

void UART_Check(void)
{
    if(RI == 1)
    {
        RI = 0;

        if(SBUF >= '0' && SBUF <= '3')
        {
            uart_mode = SBUF - '0';

            UART_SendString("Mode=");
            UART_SendByte(SBUF);
            UART_SendString("\r\n");
        }
    }
}

/* ===================== I2C ===================== */

void I2C_Start(void)
{
    I2C_SDA = 1;
    I2C_SCL = 1;
    DelayUs(5);

    I2C_SDA = 0;
    DelayUs(5);

    I2C_SCL = 0;
}

void I2C_Stop(void)
{
    I2C_SDA = 0;
    I2C_SCL = 1;
    DelayUs(5);

    I2C_SDA = 1;
    DelayUs(5);
}

void I2C_SendByte(unsigned char dat)
{
    unsigned char i;

    for(i = 0; i < 8; i++)
    {
        I2C_SDA = (dat & 0x80) ? 1 : 0;

        I2C_SCL = 1;
        DelayUs(5);

        I2C_SCL = 0;
        DelayUs(5);

        dat <<= 1;
    }

    I2C_SDA = 1;
    I2C_SCL = 1;
    DelayUs(5);
    I2C_SCL = 0;
}

unsigned char I2C_ReadByte(unsigned char ack)
{
    unsigned char i;
    unsigned char dat = 0;

    I2C_SDA = 1;

    for(i = 0; i < 8; i++)
    {
        dat <<= 1;

        I2C_SCL = 1;
        DelayUs(5);

        if(I2C_SDA)
            dat |= 0x01;

        I2C_SCL = 0;
        DelayUs(5);
    }

    I2C_SDA = ack ? 0 : 1;

    I2C_SCL = 1;
    DelayUs(5);

    I2C_SCL = 0;
    I2C_SDA = 1;

    return dat;
}

unsigned char PCF8591_ReadAIN0(void)
{
    unsigned char dat;

    I2C_Start();
    I2C_SendByte(PCF8591_WRITE);
    I2C_SendByte(0x40);
    I2C_Stop();

    I2C_Start();
    I2C_SendByte(PCF8591_READ);

    I2C_ReadByte(1);       // 第一次旧数据，丢弃
    dat = I2C_ReadByte(0); // 第二次当前数据

    I2C_Stop();

    return dat;
}

/* ===================== 交通灯 ===================== */
/*
P1.0 东西绿
P1.1 东西黄
P1.2 东西红
P1.3 南北绿
P1.4 南北黄
P1.5 南北红
高电平亮
*/

void Traffic_Show(unsigned char state)
{
    P1 &= 0xC0;   // 清P1.0~P1.5，保留P1.6、P1.7

    switch(state)
    {
        case 0:
            P1 |= 0x21;   // 东西绿 + 南北红
            break;

        case 1:
            P1 |= 0x22;   // 东西黄 + 南北红
            break;

        case 2:
            P1 |= 0x0C;   // 东西红 + 南北绿
            break;

        case 3:
            P1 |= 0x14;   // 东西红 + 南北黄
            break;
    }
}

void Traffic_Task(void)
{
    if(traffic_state == 0 && traffic_time >= 5000)
    {
        traffic_time = 0;
        traffic_state = 1;
        Traffic_Show(traffic_state);
    }
    else if(traffic_state == 1 && traffic_time >= 2000)
    {
        traffic_time = 0;
        traffic_state = 2;
        Traffic_Show(traffic_state);
    }
    else if(traffic_state == 2 && traffic_time >= 5000)
    {
        traffic_time = 0;
        traffic_state = 3;
        Traffic_Show(traffic_state);
    }
    else if(traffic_state == 3 && traffic_time >= 2000)
    {
        traffic_time = 0;
        traffic_state = 0;
        Traffic_Show(traffic_state);
    }
}

/* ===================== RGB ===================== */

void RGB_Show(unsigned char color)
{
    RGB_R = 0;
    RGB_G = 0;
    RGB_B = 0;

    switch(color)
    {
        case 0: RGB_R = 0; RGB_G = 0; RGB_B = 0; break; // 灭
        case 1: RGB_R = 1; RGB_G = 0; RGB_B = 0; break; // 红
        case 2: RGB_R = 0; RGB_G = 1; RGB_B = 0; break; // 绿
        case 3: RGB_R = 0; RGB_G = 0; RGB_B = 1; break; // 蓝
        case 4: RGB_R = 1; RGB_G = 1; RGB_B = 0; break; // 黄
        case 5: RGB_R = 1; RGB_G = 0; RGB_B = 1; break; // 紫
        case 6: RGB_R = 0; RGB_G = 1; RGB_B = 1; break; // 青
        case 7: RGB_R = 1; RGB_G = 1; RGB_B = 1; break; // 白
    }
}

void Key_Task(void)
{
    if(KEY_RGB == 0)
    {
        DelayMs(20);

        if(KEY_RGB == 0)
        {
            rgb_color++;

            if(rgb_color > 7)
                rgb_color = 0;

            RGB_Show(rgb_color);

            while(KEY_RGB == 0);
        }
    }
}

/* ===================== 流水灯 ===================== */
/*
P2.0~P2.7：流水灯
0：正向慢速
1：正向快速
2：反向慢速
3：反向快速
*/

void Flow_Task(void)
{
    unsigned int speed;

    if(uart_mode == 0 || uart_mode == 2)
        speed = 400;
    else
        speed = 120;

    if(flow_time >= speed)
    {
        flow_time = 0;

        P2 = flow_led;

        if(uart_mode == 0 || uart_mode == 1)
        {
            flow_led <<= 1;

            if(flow_led == 0x00)
                flow_led = 0x01;
        }
        else
        {
            flow_led >>= 1;

            if(flow_led == 0x00)
                flow_led = 0x80;
        }
    }
}

/* ===================== 步进电机 ===================== */

void Motor_Output(unsigned char dat)
{
    M1 = (dat & 0x01) ? 1 : 0;
    M2 = (dat & 0x02) ? 1 : 0;
    M3 = (dat & 0x04) ? 1 : 0;
    M4 = (dat & 0x08) ? 1 : 0;
}

void Motor_Forward(void)
{
    Motor_Output(motor_table[motor_step]);

    motor_step++;

    if(motor_step >= 4)
        motor_step = 0;
}

void Motor_Reverse(void)
{
    Motor_Output(motor_table[motor_step]);

    if(motor_step == 0)
        motor_step = 3;
    else
        motor_step--;
}

void Motor_Task(void)
{
    if(motor_time >= 5)
    {
        motor_time = 0;

        if(light_value < 128)
            Motor_Forward();
        else
            Motor_Reverse();
    }
}

/* ===================== 亮度采集与显示 ===================== */

void Light_Task(void)
{
    if(light_time >= 500)
    {
        light_time = 0;

        light_value = PCF8591_ReadAIN0();

        UART_SendString("湾大电子, love Fry Tgq Lyc eve\r\n");
        UART_SendString("Light=");
        UART_SendNumber(light_value);

        if(light_value < 128)
            UART_SendString(" Forward\r\n\r\n");
        else
            UART_SendString(" Reverse\r\n\r\n");
    }
}

/* ===================== 定时器0 ===================== */

void Timer0_Init(void)
{
    TMOD &= 0xF0;
    TMOD |= 0x01;     // 定时器0方式1

    TH0 = 0xFC;
    TL0 = 0x66;       // 约1ms

    ET0 = 1;
    TR0 = 1;
    EA = 1;
}

void Timer0_ISR(void) interrupt 1
{
    TH0 = 0xFC;
    TL0 = 0x66;

    traffic_time++;
    flow_time++;
    motor_time++;
    light_time++;
}

/* ===================== 主函数 ===================== */

void main(void)
{
    UART_Init();
    Timer0_Init();

    RGB_Show(0);
    Traffic_Show(0);

    P2 = 0x01;

    UART_SendString("APDS-9002 Light System\r\n");
    UART_SendString("Input 0~3 to change LED flow\r\n\r\n");

    while(1)
    {
        UART_Check();
        Key_Task();
        Traffic_Task();
        Flow_Task();
        Light_Task();
        Motor_Task();
    }
}