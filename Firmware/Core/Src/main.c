/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define Pulsee 10752.0 // 16 vong/phut * 168 (ti so truyen 1:168) *  4 (2 canh xung A va B)
#define Ts 0.02 // 20ms
char u8_RxBuff[100]; //buffer luu chuoi nhan duoc
uint8_t u8_RxData;
uint8_t _RxIndex;
char u8_TxBuff[50];
char u8_Data_ReceFromGUI[100];
uint16_t Uart_Flag = 0;
uint16_t Tx_Flag = 0;
uint16_t Motor_Accept_Flag = 0;//C? cho phép d?ng co ch?y
float Check_Mode_Select = 0;//Ch?n Velo Hay Posi??
float SetPoint = 0,Kp_Gui = 0,Ki_Gui = 0,Kd_Gui = 0;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
int16_t check_xung = 0, speed = 0;
float output_pid, vantoc_real, vitri_real;
float e_vitri = 0, e_pre_vitri = 0;
float e_vantoc = 0, e_pre_vantoc = 0;
volatile short counter = 0;
//------------------------------
///cau truc khai bao PID
typedef struct {
	float P_part;
	float I_part;
	float D_part;
} PID_control;
PID_control PID_contr;
//-------------------------
typedef struct {
	int32_t position;
	int16_t speed_by_encoder;  // don vi: xung/(tgian ngat timer)
	int16_t pre_speed_by_encoder;
	int16_t rate;
	short pre_counter;
	int32_t velocity;    // vong/phut
	int32_t pre_velocity;

} instance_encoder;
instance_encoder instance_enc;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
typedef enum {
	RR_MOTOR,
	NRR_MOTOR,
	STOP_MOTOR
} motor_status;
motor_status motor;

typedef enum
{
	Select_Velo,
	Select_Posi,
}Select_Tune;
int32_t prev_count = 0;
//-----------------------------------------------------------
void PWM_control_vantoc(TIM_HandleTypeDef *htim, float duty) {
	if(duty > 0)
	{
		HAL_GPIO_WritePin(IN1_GPIO_Port,IN1_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(IN2_GPIO_Port,IN2_Pin,GPIO_PIN_RESET); //chieu thuan cung chieu kim dong ho
		TIM1->CCR3 =  duty*(TIM1->ARR)/100;;
	}
	else if(duty < 0)
	{
		HAL_GPIO_WritePin(IN1_GPIO_Port,IN1_Pin,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(IN2_GPIO_Port,IN2_Pin,GPIO_PIN_SET);
		TIM1->CCR3 =  (-duty)*(TIM1->ARR)/100; // nguoc chieu kim dong ho
	}
	else
	{
		HAL_GPIO_WritePin(IN1_GPIO_Port,IN1_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(IN2_GPIO_Port,IN2_Pin,GPIO_PIN_SET);
	}
}
//----------------------------------------------------------------------------------------------------------------------

void PWM_control_vitri(TIM_HandleTypeDef *htim, float duty)
{
	if(duty > 0)
	{
		HAL_GPIO_WritePin(IN1_GPIO_Port,IN1_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(IN2_GPIO_Port,IN2_Pin,GPIO_PIN_RESET); //cung chieu kim dong ho
		htim1.Instance->CCR3 =  duty*(htim1.Instance->ARR)/100;;
//		htim1.Instance->CCR3 =  duty*900/100;
	}
	else if(duty < 0) {
		HAL_GPIO_WritePin(IN1_GPIO_Port,IN1_Pin,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(IN2_GPIO_Port,IN2_Pin,GPIO_PIN_SET);
		htim1.Instance->CCR3 =  (-duty)*(htim1.Instance->ARR)/100; // nguoc chieu kim dong ho
//		htim1.Instance->CCR3 =  -duty*900/100;
	}
	else {
		HAL_GPIO_WritePin(IN1_GPIO_Port,IN1_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(IN2_GPIO_Port,IN2_Pin,GPIO_PIN_SET);
	}
}
//
void Send_Data_To_Gui()
{
	if(Motor_Accept_Flag == 1 && Check_Mode_Select == 1.000000)
	{
		sprintf(u8_TxBuff,"11|%f \r\n",vantoc_real);
		HAL_UART_Transmit(&huart1,(uint8_t*)u8_TxBuff,sizeof(u8_TxBuff),20);
	}
	else if(Motor_Accept_Flag == 1 && Check_Mode_Select == 2.000000)
	{
		sprintf(u8_TxBuff,"11|%f \r\n",vitri_real);
		HAL_UART_Transmit(&huart1,(uint8_t*)u8_TxBuff,sizeof(u8_TxBuff),20);
	}
}
//------------------------------------------------------------------------------------------------------------------------

void encoder()
{
	//instance_enc.speed_by_encoder = TIM2->CNT;
//	htim2.Instance->CNT = 0;
	if(u8_Data_ReceFromGUI[0] == 'R')
	{
		htim2.Instance->CNT = 0;
		instance_enc.speed_by_encoder = 0;
		instance_enc.pre_speed_by_encoder = 0;
	}
	else
	{
		instance_enc.rate = htim2.Instance->CNT - instance_enc.pre_speed_by_encoder;
	    instance_enc.pre_speed_by_encoder = htim2.Instance->CNT;
        //instance_enc.speed_by_encoder = htim2.Instance->CNT;
	    instance_enc.position += instance_enc.rate;
	}
}
//-----------------------------------------------------------------------------------------------------------------------
//////////
void control_PID_Velocity(PID_control *pid_tune, float setpoint_vantoc, float Kp, float Ki, float Kd)    // moi chi dieu khien duoc toc do dong co
{   //velocity vong/phut
	vantoc_real =  (float)instance_enc.rate*60.0f/(Ts*2688*4);
	e_vantoc = setpoint_vantoc - (vantoc_real);
	instance_enc.velocity = vantoc_real;
	pid_tune->P_part = e_vantoc;
	pid_tune->I_part += e_vantoc*Ts;
	pid_tune->D_part = (e_vantoc-e_pre_vantoc)/Ts;
	output_pid = Kp*(pid_tune->P_part) + Ki*(pid_tune->I_part) + Kd*(pid_tune->D_part);
	if(output_pid > 95.0)
	{
		output_pid = 95.0;
	}
	else if(output_pid < -95.0)
	{
		output_pid = -95.0;
	}
	e_pre_vantoc = e_vantoc;

}
//--------------------------------------------------------------------------------------------------------------------

void control_PID_Position(PID_control *pid_tune, float setpoint_vitri, float Kp, float Ki, float Kd)
{
	vitri_real =  (float)instance_enc.position*360/10752;
	e_vitri = setpoint_vitri - (vitri_real);

	pid_tune->P_part = e_vitri;
	pid_tune->I_part += e_vitri*Ts;
	pid_tune->D_part = (e_vitri-e_pre_vitri)/Ts;
	output_pid = Kp*(pid_tune->P_part) + Ki*(pid_tune->I_part) + Kd*(pid_tune->D_part);
	if(output_pid > 95.0)
	{
		output_pid = 95.0;
	}
	else if(output_pid < -95.0)
	{
		output_pid = -95.0;
	}
	e_pre_vitri = e_vitri;
}
void select_mode(Select_Tune select)
{
//	select_tunning select;
	switch(select)
	{
		case Select_Posi:
		          /*control_PID_Position(&PID_contr, 180, 2.5, 0.0075, 0.01);
              PWM_control_vitri(&htim1, output_pid);
			        sprintf(buffer, "%f\n",vitri_real);
	            HAL_UART_Transmit(&huart1,(uint8_t*)buffer,strlen(buffer),HAL_MAX_DELAY);*/
//			sprintf(buffer, "Position is: %.3f", now_position);
//			HAL_UART_Receive_IT(&huart1, buffer, strlen(buffer));
		break;
		case Select_Velo:
			/*control_PID_Velocity(&PID_contr, 20, 2, 0.0001, 0.00005);
			PWM_control_vantoc(&htim1, output_pid);
		  sprintf(buffer, "%f\n",vantoc_real);
	    HAL_UART_Transmit(&huart1,(uint8_t*)buffer,strlen(buffer),HAL_MAX_DELAY);*/
		break;
		default:
		break;
	}
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM3)
	{
		encoder();
	}
	if(Motor_Accept_Flag == 1 && Check_Mode_Select == 1.000000) //Van toc
	{
		control_PID_Velocity(&PID_contr, SetPoint, Kp_Gui, Ki_Gui, Kd_Gui);
		PWM_control_vantoc(&htim1, output_pid);
	}
	else if(Motor_Accept_Flag == 1 && Check_Mode_Select == 2.000000) //Vi tri
	{
		control_PID_Position(&PID_contr, SetPoint, Kp_Gui, Ki_Gui, Kd_Gui);
	    PWM_control_vitri(&htim1, output_pid);
	    //sprintf(u8_TxBuff,"%f\n",vitri_real);
	    //HAL_UART_Transmit(&huart1,(uint8_t*)u8_TxBuff,sizeof(u8_TxBuff),100);
	}
	Send_Data_To_Gui();
}
//-------------------------------------------------
//Ham ngat uart de nhan du lieu truyen xuong
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	 UNUSED(huart);
	if(huart->Instance == USART1)
	{
		if(u8_RxData != ';')
		{
			u8_RxBuff[_RxIndex++] = u8_RxData;
		}
		else if(u8_RxData == ';')
		{
			_RxIndex = 0;
			Uart_Flag = 1; //chuoi nhan ve tu Gui da truyen xong
		}
		HAL_UART_Receive_IT(&huart1,&u8_RxData,1);
	}
}
//------------------------------------------------------------------
//Ham con c?t chu?i nh?n du?c xu?t ra giá tr? SetPoint,Kp,Ki,Kd
void string_cut(char *input, char *key, float *value)
{
    char *ptr = strstr(input,key);
    if(ptr != NULL)
    {
        ptr += strlen(key);
        *value = atof(ptr);
    }
}
//Hàm con g?i data lên CSharp

//------------------------------------------------------------------
//------------------------------------------------------------------
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_UART_Receive_IT(&huart1,&u8_RxData,1);
  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if(Uart_Flag)
	  {
		  memset(u8_Data_ReceFromGUI,0,sizeof(u8_Data_ReceFromGUI));
		  strcpy(u8_Data_ReceFromGUI,u8_RxBuff);
		  if(u8_Data_ReceFromGUI[0] == 'S')
		  {
			  string_cut(u8_Data_ReceFromGUI,"Selectmode=",&Check_Mode_Select);
		  }
		  else if(u8_Data_ReceFromGUI[0] == 'C')
		  {
		  	  Motor_Accept_Flag = 1;
		  }
		  else if(u8_Data_ReceFromGUI[0] == 'K')
		  {
			  string_cut(u8_Data_ReceFromGUI,"Ksetpoint=",&SetPoint);
			  string_cut(u8_Data_ReceFromGUI,"Kp=",&Kp_Gui);
			  string_cut(u8_Data_ReceFromGUI,"Ki=",&Ki_Gui);
			  string_cut(u8_Data_ReceFromGUI,"Kd=",&Kd_Gui);
		  }
		  else if(u8_Data_ReceFromGUI[0] == 'R')
		  {
			  //Them ham xu ly Reset
			  Kp_Gui = Ki_Gui = Kd_Gui =0;
			  SetPoint = 0;
			  htim2.Instance->CNT = 0;
			  instance_enc.position = 0;
			  instance_enc.speed_by_encoder = 0;

			  if(Check_Mode_Select == 1.000000) //Van toc
			  {
				 control_PID_Velocity(&PID_contr, SetPoint, Kp_Gui, Ki_Gui, Kd_Gui);
			  }
			  else if(Check_Mode_Select == 2.000000) //Vi tri
			  {
			  	 control_PID_Position(&PID_contr, SetPoint, Kp_Gui, Ki_Gui, Kd_Gui);
			  }
			  output_pid = 0;
			  HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_RESET);
			  HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);
			  Check_Mode_Select = 0;
			  Motor_Accept_Flag = 0;
			  //NVIC_SystemReset();
		  }
		  Uart_Flag = 0;
		  memset(u8_RxBuff,0,sizeof(u8_RxBuff));
	  }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 63;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 639;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, IN1_Pin|IN2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : IN1_Pin IN2_Pin */
  GPIO_InitStruct.Pin = IN1_Pin|IN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
