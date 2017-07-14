/*
*********************************************************************************************************
*
*	模块名称 : 主程序模块。
*	文件名称 : main.c
*	版    本 : V1.0
*	说    明 : 按键检测例子
*	修改记录 :
*		版本号  日期       作者    说明
*		v1.0    2015-12-09 armfly  首发
*
*	Copyright (C), 2015-2020, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"			/* 底层硬件驱动 */

static void AD7606_Mak(void);
static void AD7606_Disp(void);

/*角度*/
float a[3],w[3],angle[3],T;
extern unsigned char Re_buf[11],temp_buf[11],counter;
extern unsigned char sign;
/*AD*/
static int16_t s_volt[8];
static int16_t s_dat[8];
static double s_force[8];

/*
*********************************************************************************************************
*	函 数 名: main
*	功能说明: c程序入口
*	形    参：无
*	返 回 值: 错误代码(无需处理)
*********************************************************************************************************
*/
int main(void)
{
	unsigned char Temp[11];
	uint8_t ucFifoMode;
	int16_t i;	
	int16_t iTemp;
	
	
	bsp_Init();		/* 硬件初始化 */

	ucFifoMode = 0;	 	/* AD7606进入普通工作模式 */

	bsp_InitAD7606();	/* 配置AD7606所用的GPIO */
	AD7606_StartRecord(1000);		/* 进入自动采集模式，采样频率1KHz，数据存放在全局FIFO */
	
	/*设置过采样和量程*/
	AD7606_SetOS(AD_OS_X64);		/* 过采样倍率6 */
	//AD7606_SetOS(AD_OS_NO);		/* 无过采样 */
	AD7606_SetInputRange(0);	/* 0表示输入量程为正负5V, 1表示正负10V */
	AD7606_StartConvst();		/* 启动1次转换 */

	bsp_StartAutoTimer(0, 500);	/* 启动1个200ms的自动重装的定时器 */

	while (1)
	{
//		printf("count = %6d \r\n", count);
//		/* 也可以用 sprintf先输出到一个buf，然后在发送到串口 */
//		{
//			char buf[64];
//			sprintf(buf, "count = %6d \r", count);
//			comSendBuf(COM1, (uint8_t *)buf, strlen(buf));
//		}
		
//		AD7606_ReadNowAdc();		/* 读取采样结果 */
//		printf("CH1 = %6d, CH2 = %6d, CH3 = %6d, CH4 = %6d, \r\n",
//		g_tAD7606.sNowAdc[0], g_tAD7606.sNowAdc[1], g_tAD7606.sNowAdc[2], g_tAD7606.sNowAdc[3]);

		/* 处理数据 */
		AD7606_Mak();
						 
		/* 打印ADC采样结果 */
//		AD7606_Disp(); 

		s_force[0]=80.0 / 1000 * (s_volt[0]+38);
		s_force[1]=80.0 / 1000 * (s_volt[1]+191);
		s_force[2]=160.0 / 1000 * (s_volt[2]-95);
		printf("%f,%f,%f,%f,%f,%f,%f,%f,%f", s_force[0],s_force[1],s_force[2],angle[0],angle[1],angle[2],a[0],a[1],a[2]);
//		printf("%6dmV,", s_volt[i]);
//		printf(" CH%d = %6d,0x%04X (-%d.%d%d%d V) \r\n", i+1, s_dat[i], (uint16_t)s_dat[i], iTemp /1000, (iTemp%1000)/100, (iTemp%100)/10,iTemp%10);
		printf("\r\n");
			
		

		if (ucFifoMode == 0)	/* AD7606 普通工作模式 */
		{
			if (bsp_CheckTimer(0))
			{
				/* 每隔500ms 进来一次. 由软件启动转换 */
				AD7606_ReadNowAdc();		/* 读取采样结果 */
				AD7606_StartConvst();		/* 启动下次转换 */
			}
		}
		if (ucFifoMode == 1)  /* AD7606进入FIFO工作模式 */
		{
			printf("\33[%dA", (int)1);  /* 光标上移n行 */	
			printf("AD7606进入FIFO工作模式 (200KHz 8通道同步采集)...\r\n");
			AD7606_StartRecord(200000);		/* 启动200kHz采样速率 */
		}		
		
		
		
		if(sign)
		{  
			memcpy(Temp,Re_buf,11);
			sign=0;
			if(Re_buf[0]==0x55)       //检查帧头
			{  
				switch(Re_buf[1])
				{
					case 0x51: //标识这个包是加速度包
					a[0] = ((short)(Temp[3]<<8 | Temp[2]))/32768.0*16;      //X轴加速度
					a[1] = ((short)(Temp[5]<<8 | Temp[4]))/32768.0*16;      //Y轴加速度
					a[2] = ((short)(Temp[7]<<8 | Temp[6]))/32768.0*16;      //Z轴加速度
					T    = ((short)(Temp[9]<<8 | Temp[8]))/340.0+36.25;      //温度
					break;
					case 0x52: //标识这个包是角速度包
					w[0] = ((short)(Temp[3]<<8| Temp[2]))/32768.0*2000;      //X轴角速度
					w[1] = ((short)(Temp[5]<<8| Temp[4]))/32768.0*2000;      //Y轴角速度
					w[2] = ((short)(Temp[7]<<8| Temp[6]))/32768.0*2000;      //Z轴角速度
					T    = ((short)(Temp[9]<<8| Temp[8]))/340.0+36.25;      //温度
					break;
					case 0x53: //标识这个包是角度包
					angle[0] = ((short)(Temp[3]<<8| Temp[2]))/32768.0*180;   //X轴滚转角（x 轴）
					angle[1] = ((short)(Temp[5]<<8| Temp[4]))/32768.0*180;   //Y轴俯仰角（y 轴）
					angle[2] = ((short)(Temp[7]<<8| Temp[6]))/32768.0*180;   //Z轴偏航角（z 轴）
					T        = ((short)(Temp[9]<<8| Temp[8]))/340.0+36.25;   //温度

					//printf("X轴角度：%.2f   Y轴角度：%.2f   Z轴角度：%.2f\r\n",angle[0],angle[1],angle[2]);
					break;
					default:  break;
				}
				//printf("X轴角度：%.2f   Y轴角度：%.2f   Z轴角度：%.2f\r\n",angle[0],angle[1],angle[2]);	
				//printf("X角度：%.2f  Y角度：%.2f  Z角度：%.2f  X速度：%.2f  Y速度：%.2f  Z速度：%.2f\r\n",angle[0],angle[1],angle[2],w[0],w[1],w[2]);
			}

		}
//		bsp_DelayMS(50);

	}
}


/*
*********************************************************************************************************
*	函 数 名: AD7606_Mak
*	功能说明: 处理采样后的数据
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void AD7606_Mak(void)
{
	uint8_t i;
	int16_t value;  //有符号位
	int32_t volt;
	
	for (i = 0; i < 8; i++)
	{		
	/* 
		32767 = 5V , 这是理论值，实际可以根据5V基准的实际值进行公式矫正 
		volt[i] = ((int16_t)dat[i] * 5000) / 32767;	计算实际电压值（近似估算的），如需准确，请进行校准            
		volt[i] = dat[i] * 0.3051850947599719
	*/	
		s_dat[i] = g_tAD7606.sNowAdc[i]& 0x0000FFFF;  //相与之后为无符号位，赋值给value之后为有符号位。
		if (g_tAD7606.ucRange == 0)
		{
			s_volt[i] = (s_dat[i] * 5000) / 32767;
		}
		else
		{
			s_volt[i] = (s_dat[i] * 10000) / 32767;
		}
	}
}
 
/*
*********************************************************************************************************
*	函 数 名: AD7606_Disp
*	功能说明: 显示采样后的数据
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void AD7606_Disp(void)
{
	int16_t i;	
	int16_t iTemp;

	/* 打印采集数据 */
//	printf(" OS  =  %d \r\n", g_tAD7606.ucOS);
	
	for (i = 0; i < 4; i++)
	{                
   		iTemp = s_volt[i];	/* uV  */
		
		if (s_dat[i] < 0)
		{
			iTemp = -iTemp;
//			s_volt[0]=s_volt[0];
//			s_volt[1]=s_volt[1];
//			s_volt[2]=s_volt[2];
			printf("%6dmV  ", s_volt[i]);
			//printf(" CH%d = %6d,0x%04X (-%d.%d%d%d V) \r\n", i+1, s_dat[i], (uint16_t)s_dat[i], iTemp /1000, (iTemp%1000)/100, (iTemp%100)/10,iTemp%10);
		}
		else
		{
			printf("%6dmV  ", s_volt[i]);
      //printf(" CH%d = %6d,0x%04X ( %d.%d%d%d V) \r\n", i+1, s_dat[i], (uint16_t)s_dat[i] , iTemp /1000, (iTemp%1000)/100, (iTemp%100)/10,iTemp%10);                    
		}
	}
	printf("\r\n");
//	printf("\33[%dA", (int)9);  /* 光标上移n行 */		
}
