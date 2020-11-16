#include "stm32f4xx.h"
#include "system_stm32f4xx.h"
/////////////////////////////////////////////////////////
#define TOTALTASKS   10    // The maximum number of tasks (max value is 255 because all id's are chosen to be char variables)
typedef void (*fptr)(void);
static struct Task{
	int ID;				//Task unique id
	int Priority;		//Task priority
	int  delay;			// Delay in number of ticks
	void (*pntr)(void);		// A pointer to task function
}TasksQueue[TOTALTASKS], ReadyQueue[TOTALTASKS], DelayQueue[TOTALTASKS];	//Delayed tasks
/********************************************************
 Local Functions Declarations (Scope of this file only)
********************************************************/
char GetNewTask(void (*pntr)(void)); // Returns 0 if no task entry is available (As Ron Kreymborg in his "Possible Extensions" section)
void RunTask(void (*pntr)(void));
void Init(void);
void DecrementDelay(void);
void QueDelay(void (*pntr)(void), int delay);
void QueTask(void (*pntr)(void), int P);
void ReRunMe(int delay);
void Dispatch(void);
fptr GetTaskParameters(void (*pntr)(void), int*, int*);
void SysTick_Handler(void);
void USART2_IRQHandler(void);
void EXTI0_IRQHandler(void);
static void sendUART(uint8_t * data, uint32_t length);
static uint8_t receiveUART(void);
void Task1(void);
void Task2(void);
void Task3(void);
/********************************************************
 Global Variables
********************************************************/
static  int ReadyQueueTail_id;  // The ready queue tail (head is : tasks[0].next)
static int DelayQueueHead_id;  // The delay queue head (tail i is detected by iterating the queue and finding that tasks[i].next = 0)
static int RunningTask_id;     // The id for the task to be executed by RunTask
static char SwitchOn = 0;
static char timerFlag = 0;
static volatile uint8_t stopFlag = 0;
/********************************************************
 Functions Definitions
********************************************************/
void SysTick_Handler(void)  {
	timerFlag = 1;
	DecrementDelay();
}
///////////////////////////////////////////////////////////
void USART2_IRQHandler(void) {
	/* pause/resume UART messages */
	stopFlag = !stopFlag;
	
	/* dummy read */
	(void)receiveUART();
}
///////////////////////////////////////////////////////////
void EXTI0_IRQHandler(void) {
		/* Clear interrupt request */
		EXTI->PR |= 0x01;
		/* send msg indicating button state */
}
///////////////////////////////////////////////////////////
static void sendUART(uint8_t * data, uint32_t length){
	 for (uint32_t i=0; i<length; ++i){
      // add new data without messing up DR register
      uint32_t value = (USART2->DR & 0x00) | data[i];
		  // send data
			USART2->DR = value;
      // busy wait for transmit complete
      while(!(USART2->SR & (1 << 6)));
		  // delay
      for(uint32_t j=0; j<1000; ++j);
      }
}
///////////////////////////////////////////////////////////
static uint8_t receiveUART(){
	  // extract data
	  uint8_t data = USART2->DR & 0xFF;
	
	  return data;
}
///////////////////////////////////////////////////////////
static void gpioInit(){	
    // enable GPIOA clock, bit 0 on AHB1ENR
    RCC->AHB1ENR |= (1 << 0);

    // set pin modes as alternate mode 7 (pins 2 and 3)
    // USART2 TX and RX pins are PA2 and PA3 respectively
    GPIOA->MODER &= ~(0xFU << 4); // Reset bits 4:5 for PA2 and 6:7 for PA3
    GPIOA->MODER |=  (0xAU << 4); // Set   bits 4:5 for PA2 and 6:7 for PA3 to alternate mode (10)

    // set pin modes as high speed
    GPIOA->OSPEEDR |= 0x000000A0; // Set pin 2/3 to high speed mode (0b10)

    // choose AF7 for USART2 in Alternate Function registers
    GPIOA->AFR[0] |= (0x7 << 8); // for pin A2
    GPIOA->AFR[0] |= (0x7 << 12); // for pin A3
}
///////////////////////////////////////////////////////////
static void uartInit(){
	
    // enable USART2 clock, bit 17 on APB1ENR
    RCC->APB1ENR |= (1 << 17);
	
	  // USART2 TX enable, TE bit 3
    USART2->CR1 |= (1 << 3);

    // USART2 rx enable, RE bit 2
    USART2->CR1 |= (1 << 2);
	
	  // USART2 rx interrupt, RXNEIE bit 5
    USART2->CR1 |= (1 << 5);

    // baud rate = fCK / (8 * (2 - OVER8) * USARTDIV)
    //   for fCK = 16 Mhz, baud = 115200, OVER8 = 0
    //   USARTDIV = 16Mhz / 115200 / 16 = 8.6805
    // Fraction : 16*0.6805 = 11 (multiply fraction with 16)
    // Mantissa : 8
    // 12-bit mantissa and 4-bit fraction
    USART2->BRR |= (8 << 4);
    USART2->BRR |= 11;

    // enable usart2 - UE, bit 13
    USART2->CR1 |= (1 << 13);
}
/********************************************************
 Dispatcher Functions Definitions
********************************************************/
//This function is used to retrive tasks parameters such as ID and priority
fptr GetTaskParameters(void (*pntr)(void), int *P, int *ID) {
	if(pntr != 0)
	{
		for(int i = 0; i < TOTALTASKS; i++)
		{
			if(pntr == TasksQueue[i].pntr)
			{
				*P = TasksQueue[i].Priority;
				*ID = TasksQueue[i].ID;
			}
		}
	}
	else
	{
		for(int i = 0; i < TOTALTASKS; i++)
		{
			if(*ID == TasksQueue[i].ID && TasksQueue[i].pntr != 0)
			{
				*P = TasksQueue[i].Priority;
				pntr = TasksQueue[i].pntr;
			}
		}
	}
	return pntr;
}
///////////////////////////////////////////////////////////
//This function creates and initializes all needed data structures.
void Init() {
	for (int i = 0; i < TOTALTASKS; i++) 
	{
		TasksQueue[i].ID = 0;
		TasksQueue[i].Priority = 0;
		TasksQueue[i].delay = 0;
		TasksQueue[i].pntr = 0;
		ReadyQueue[i].ID = 0;
		ReadyQueue[i].Priority = 0;
		ReadyQueue[i].delay = 0;
		ReadyQueue[i].pntr = 0; 	
		DelayQueue[i].ID = 0;
		DelayQueue[i].Priority = 0;
		DelayQueue[i].delay = 0;
		DelayQueue[i].pntr = 0; 
	}
	ReadyQueueTail_id = 0; 	// This can tell if the ready queue is empty or not
	DelayQueueHead_id = 0; 	// This can tell if the delay queue is empty or not
	RunningTask_id = 0;
}
///////////////////////////////////////////////////////////
//This function is used to insert a task to the ready queue by passing a pointer to the function and priority
void QueTask(void (*pntr)(void),int P) {
	int TaskExist = 0;
	int TaskQueueIndex = 0;
	//Normalize Priority
	if(P < 1)
		P = 1;
	if(P > 8)
	P = 8;
	//Add task to tasks queue only once
	if (TasksQueue[0].pntr == 0) // If the ready queue is empty
	{
		TasksQueue[0].pntr = pntr;
		TasksQueue[0].Priority = P;
		TasksQueue[0].ID = 0;
		TaskQueueIndex = 0;
	}
	else
	{
		
		for(int j = 0; j < TOTALTASKS; j++)
		{
			if(pntr == TasksQueue[j].pntr)
			{
				TaskExist = 1;
				TaskQueueIndex = j;
				break;
			}
		}
		if(!TaskExist)
		{
			int InsertionLocation = -1;
			for(int j = 0; j < TOTALTASKS; j++)
			{
				if(TasksQueue[j].pntr != 0)
				{
					if(TasksQueue[j].Priority > P)
					{
						InsertionLocation = j;
						break;
					}
				}
				else
				{
					InsertionLocation = j;
					break;
				}
			}
			for(int i = TOTALTASKS - 2; i >= InsertionLocation; i--)
			{
				if(TasksQueue[i].pntr != 0)
				{
					TasksQueue[i + 1].Priority = TasksQueue[i].Priority;
					TasksQueue[i + 1].pntr = TasksQueue[i].pntr; 
					TasksQueue[i + 1].ID = i + 1;
					TasksQueue[i].pntr = 0;
					TasksQueue[i].Priority = 0;
				}
			}
			TasksQueue[InsertionLocation].pntr = pntr;
			TasksQueue[InsertionLocation].Priority = P;
			TasksQueue[InsertionLocation].ID = InsertionLocation;
			TaskQueueIndex = InsertionLocation;
		}
	}
	//Add task to ready queue
	if (ReadyQueue[0].pntr == 0) // If the ready queue is empty
	{
		ReadyQueue[0].pntr = pntr;
		ReadyQueue[0].Priority = P;
		if(TaskExist)
			ReadyQueue[0].ID = TasksQueue[TaskQueueIndex].ID;
		else
			ReadyQueue[0].ID = 0;
		ReadyQueueTail_id = 0;
	}
	else
	{
		int InsertionLocation = -1;
		for(int j = 0; j < TOTALTASKS; j++)
		{
			if(ReadyQueue[j].pntr != 0)
			{
				if(ReadyQueue[j].Priority > P)
				{
					InsertionLocation = j;
					break;
				}
			}
			else
			{
				InsertionLocation = j;
				break;
			}
		}
		
		for(int i = TOTALTASKS - 2; i >= InsertionLocation; i--)
		{
			if(ReadyQueue[i].pntr != 0)
			{
				ReadyQueue[i + 1].Priority = ReadyQueue[i].Priority;
				ReadyQueue[i + 1].pntr = ReadyQueue[i].pntr; 
				if(TaskExist)
					ReadyQueue[i + 1].ID = TasksQueue[TaskQueueIndex].ID;
				else
					ReadyQueue[i + 1].ID = i + 1;
				ReadyQueue[i].Priority = 0;
				ReadyQueue[i].pntr = 0;
			}
		}
		ReadyQueue[InsertionLocation].pntr = pntr;
		ReadyQueue[InsertionLocation].Priority = P;
		if(TaskExist)
			ReadyQueue[InsertionLocation].ID = TasksQueue[TaskQueueIndex].ID;
		else
			ReadyQueue[InsertionLocation].ID = TasksQueue[InsertionLocation].ID;
		ReadyQueueTail_id++;
		
	}
}
///////////////////////////////////////////////////////////
//This function is used to remove the highest priority task from the queue and run it
void Dispatch() {
	// If the ready queue is not empty
	if (ReadyQueue[0].pntr != 0)			
	{	
		
		void (*pntr)(void) = ReadyQueue[0].pntr;
		// Set the RunningTask_id to the ready queue head
		RunningTask_id = ReadyQueue[0].ID;	
		//Shift the remainig task in the ready queue after retriving the task that will be executed
		for(int i = 0; i < TOTALTASKS - 1; i++)
		{
			ReadyQueue[i].Priority = ReadyQueue[i + 1].Priority;
			ReadyQueue[i].pntr = ReadyQueue[i + 1].pntr; 
			ReadyQueue[i].ID = ReadyQueue[i + 1].ID; 
			ReadyQueue[i + 1].Priority = 0;
			ReadyQueue[i + 1].pntr = 0;
			ReadyQueue[i + 1].ID = 0;
		}
		if (ReadyQueue[0].ID == 0)  // Reset ready queue tail if ready queue was only one task
			ReadyQueueTail_id = 0;
		else
			ReadyQueueTail_id--;
		
		RunTask(pntr); 
	}
}
///////////////////////////////////////////////////////////
//This function is used to decrement the sleeping time of the tasks in the delayed queue by 1 every tick
void DecrementDelay() {
if (DelayQueue[0].pntr != 0) // If the delay queue is not empty
	{
		//Decrement the delay ticks of existing tasks
		for(int i = 0; i < TOTALTASKS; i++)
		{
			if (DelayQueue[i].pntr != 0)
			{
				DelayQueue[i].delay--; 
			}
		}
		//Loop on the exisiting tasks and move tasks that has 0 delay to ready queue.
		for(int j = TOTALTASKS - 1; j >= 0; j--)
		{
			if(DelayQueue[j].pntr != 0 && DelayQueue[j].delay == 0)
			{
				QueTask(DelayQueue[j].pntr,DelayQueue[j].Priority);
				DelayQueue[j].Priority = 0;
				DelayQueue[j].pntr = 0;
				DelayQueue[j].ID = 0;
				DelayQueue[j].delay = 0;
				
			}
		}
	}
}
///////////////////////////////////////////////////////////
//This function should only be called from the task that is supposed to rerun (re-execute)
void ReRunMe(int delay) {
	void (*pntr)(void) = 0;
	int TemP;
	//Retrive the running task information from task queue as we have only the task ID
	//This function retrives the pointer to the runnign task and its priority to save it
	//either in the ready queue or delay queue based on the delay ticks
	pntr = GetTaskParameters(0, &TemP, &RunningTask_id);
	if(delay == 0)
	{
		QueTask(pntr,TemP);
	}
	else
	{
		QueDelay(pntr,delay);
	}
}

///////////////////////////////////////////////////////////
//Helper function that receive pointer to the RunningTask_id to run it
void RunTask(void (*pntr)(void)) {
	//uint8_t msg2[] = "RunTask !!\n";
	//sendUART(msg2, sizeof(msg2));
	pntr();     
}
///////////////////////////////////////////////////////////
/********************************************************
This function is used to insert a task to the delay queue 
by passing a pointer to the function and delay value
Delayed tasks are stored in descending order based on the delay ticks
********************************************************/
void QueDelay(void (*pntr)(void), int delay) {
	//Get task parameters
	int TempPriority = 0;
	int TempID = 0;
	//Retrive task parameters such as ID and priority to save it in the 
	GetTaskParameters(pntr,&TempPriority,&TempID);
	if(delay == 0)
	{
		//if delay = 0
		QueTask(*pntr,TempPriority);
	}
	else
	{
		if (DelayQueue[0].pntr == 0) // If the delay queue is empty
		{
			DelayQueue[0].pntr = pntr;
			DelayQueue[0].Priority = TempPriority;
			DelayQueue[0].delay = delay;
			DelayQueue[0].ID = TempID;
			DelayQueueHead_id = 0; 
		}
		else
		{
			int InsertionLocation = -1;
			for(int j = 0; j < TOTALTASKS; j++)
			{
				if(DelayQueue[j].pntr != 0)
				{
					if(DelayQueue[j].delay > delay)
						continue;
					else if(DelayQueue[j].delay == delay)
					{
						if(DelayQueue[j].Priority > TempPriority)
							continue;
						else
						{
							InsertionLocation = j;
							break;
						}
					}
					else
					{
						InsertionLocation = j;
						break;
					}
				}
				else
				{
					InsertionLocation = j;
					break;
				}
			}
			for(int i = TOTALTASKS - 2; i >= InsertionLocation; i--)
			{
				if(DelayQueue[i].pntr != 0)
				{
					DelayQueue[i + 1].Priority = DelayQueue[i].Priority;
					DelayQueue[i + 1].pntr = DelayQueue[i].pntr; 
					DelayQueue[i + 1].ID = DelayQueue[i].ID ; 
					DelayQueue[i + 1].delay = DelayQueue[i].delay;
					DelayQueue[i].Priority = 0;
					DelayQueue[i].pntr = 0;
					DelayQueue[i].ID = 0;
					DelayQueue[i].delay = 0;
				}
			}
			DelayQueue[InsertionLocation].pntr = pntr;
			DelayQueue[InsertionLocation].Priority = TempPriority;
			DelayQueue[InsertionLocation].ID = TempID;
			DelayQueue[InsertionLocation].delay = delay;
			DelayQueueHead_id = 0;
		}
	}
}

/////////////////////////////////////////////////////////
void Task1(){
  if (SwitchOn) 
	{
		SwitchOn = 0;
		uint8_t msg[] = "Task 1 !!\n";
		sendUART(msg, sizeof(msg));
		QueDelay(Task2, 8);
  }
}
/////////////////////////////////////////////////////////
void Task2() {
  if (!SwitchOn) 
	{
		SwitchOn = 1;
		uint8_t msg[] = "Task 2 !!\n";
		sendUART(msg, sizeof(msg));
		QueDelay(Task1, 4);
   }
}
////////////////////////////////////////////////////
void Task3(void){
		uint8_t msg[] = "Task3 3 !!\n";
		sendUART(msg, sizeof(msg));
    ReRunMe(2);
}
/////////////////////////////////////////////////////////
int main()
{	
	  /* startup code initialization */
	  SystemInit();
	  SystemCoreClockUpdate();
	  /* intialize UART */
	  gpioInit();
		/* intialize UART */
	  uartInit();
	  /* enable SysTick timer to interrupt system every second */
	  SysTick_Config(SystemCoreClock);
	  /* enable interrupt controller for USART2 external interrupt */
		NVIC_EnableIRQ(USART2_IRQn);
		/* Unmask External interrupt 0 */
		EXTI->IMR |= 0x0001;
	  /* Enable rising and falling edge triggering for External interrupt 0 */
		EXTI->RTSR |= 0x0001;
		EXTI->FTSR |= 0x0001;
	  /* enable interrupt controller for External interrupt 0 */
		NVIC_EnableIRQ(EXTI0_IRQn);
		
	  /* USER CODE BEGIN 2 */
		Init();
		QueTask(Task1, 1); 
		QueTask(Task2, 2); 
		QueTask(Task3, 3); 
		/* USER CODE END 2 */
	  while(1)
		{
			Dispatch();
				if(timerFlag && !stopFlag)
				{
					timerFlag = 0;
				}
		}
}
