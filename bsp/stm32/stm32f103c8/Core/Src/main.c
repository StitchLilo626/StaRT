/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define THREAD_STACK_SIZE 512

#define MSG_QUEUE_SIZE 10
#define MSG_POOL_SIZE  START_MSGQ_POOL_SIZE(sizeof(msg_t),MSG_QUEUE_SIZE) // 10条消息，每条4字节
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
s_mutex mutex1;


s_thread thread1;
s_thread thread2;
s_thread thread3;
s_uint8_t i,j = 0,k = 10;

s_uint8_t thread1stack[THREAD_STACK_SIZE];
s_uint8_t thread2stack[THREAD_STACK_SIZE];
s_uint8_t thread3stack[THREAD_STACK_SIZE];

void thread1entry(void);
void thread2entry(void);
void thread3entry(void);

void s_putc(char c)
{
    // 通过串口发送字符
    HAL_UART_Transmit(&huart1, (uint8_t *)&c, 1, HAL_MAX_DELAY);
}
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
  /* USER CODE BEGIN 2 */
	s_start_init();

  s_mutex_init(&mutex1,START_IPC_FLAG_FIFO);

	s_thread_init(&thread1,
						thread1entry,
						thread1stack,
						THREAD_STACK_SIZE,
						10,
						10);
	s_thread_startup(&thread1);
	s_thread_init(&thread2,
						thread2entry,
						thread2stack,
						THREAD_STACK_SIZE,
					  12,
						10);
	s_thread_startup(&thread2);
  s_thread_init(&thread3,
            thread3entry,
            thread3stack,
            THREAD_STACK_SIZE,
            15,
            10);
  s_thread_startup(&thread3);

	s_sched_start();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
 while (1)
 {
   /* USER CODE END WHILE */

   /* USER CODE BEGIN 3 */
		k++;
		HAL_Delay(500);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void thread1entry() /* High priority (等待互斥量) */
{
    static int phase = 0;
    while (1)
    {
        if (phase == 0)
        {
            s_mdelay(100); /* 先让低优先级线程获取互斥量制造反转 */
            s_printf("HIGH : try take mutex\n");
            if (S_OK == s_mutex_take(&mutex1, START_WAITING_FOREVER))
            {
                s_printf("HIGH : got mutex (after inheritance) j=%d\n", j);
                s_mutex_release(&mutex1);
                s_printf("HIGH : released mutex\n");
                phase = 1;
            }
        }
        else
        {
            /* 后续简单演示重复获取/释放 */
            if (S_OK == s_mutex_take(&mutex1, START_WAITING_FOREVER))
            {
                s_mutex_release(&mutex1);
            }
            s_mdelay(600);
        }
        j++;
        if (j == 255) j = 0;
        s_mdelay(50);
    }
}

void thread2entry() /* Medium priority (制造 CPU 干扰) */
{
    while (1)
    {
        i++;
        if (i % 50 == 0)
            s_printf("MED  : running i=%d\n", i);
        if (i == 255) i = 0;
        /* 不使用互斥量，纯粹占用时间片 */
        s_mdelay(40);
    }
}

void thread3entry() /* Low priority (先获取互斥量并长时间占用) */
{
    static int once = 0;
    s_uint8_t base_prio_saved = 0;
    while (1)
    {
        if (once == 0)
        {
            if (S_OK == s_mutex_take(&mutex1, START_WAITING_FOREVER))
            {
                base_prio_saved = s_thread_get()->current_priority;
                s_printf("LOW  : took mutex, do long work (base prio=%d)\n",
                         base_prio_saved);

                /* 模拟长任务分多段，期间高优先级会等待，触发优先级继承 */
                for (int seg = 0; seg < 5; seg++)
                {
                    s_mdelay(120); /* 每段持有 */
                    s_thread *self = s_thread_get();
                    if (self)
                    {
                        if (self->current_priority != base_prio_saved)
                            s_printf("LOW  : inherited priority -> %d (seg=%d)\n",
                                     self->current_priority, seg);
                    }
                }

                s_printf("LOW  : releasing mutex\n");
                s_mutex_release(&mutex1);
                s_printf("LOW  : released mutex (should drop back to prio=%d)\n",
                         base_prio_saved);
                once = 1;
            }
        }
        else
        {
            /* 之后偶尔再占用一下，验证递归外正常路径 */
            if (S_OK == s_mutex_take(&mutex1, START_WAITING_FOREVER))
            {
                s_mdelay(30);
                s_mutex_release(&mutex1);
            }
            s_mdelay(200);
        }

        k++;
        if (k == 255) k = 0;
        s_mdelay(10);
    }
}
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
