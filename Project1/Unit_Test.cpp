#include <iostream>
using namespace std;
/********************************************************
 Datatypes
********************************************************/
#define TOTALTASKS   10  
typedef void (*fptr)();
/********************************************************
System queues
********************************************************/
struct Task{
	int ID;				//Task unique id
	int Priority;		//Task priority
	int  delay;			// Delay in number of ticks
	void (*pntr)();		// A pointer to task function
}; 
Task TasksQueue[TOTALTASKS];	//All tasks in the system
Task ReadyQueue[TOTALTASKS];	//Ready tasks
Task DelayQueue[TOTALTASKS];	//Delayed tasks
/********************************************************
 Global Variables
********************************************************/
int ReadyQueueTail_id;  // The ready queue tail (head is : tasks[0].next)
int DelayQueueHead_id;  // The delay queue head (tail i is detected by iterating the queue and finding that tasks[i].next = 0)
int RunningTask_id;     // The id for the task to be executed by RunTask
int NewDelay;            // ReRunMe delay that DoQueDelay put into delay element of the task
/********************************************************
 Local Functions Declarations (Scope of this file only)
********************************************************/
void Init();
void QueTask(void (*pntr)(),int);
void Dispatch();
void RunTask(void (*pntr)());
void QueDelay(void (*pntr)(), int);
fptr GetTaskParameters(void (*pntr)(), int*, int*);
void Task1();
void Task2();
void Task3();
/********************************************************
 Functions Definitions
********************************************************/
///////////////////////////////////////////////////////////
//This function is used to retrive tasks parameters such as ID and priority
fptr GetTaskParameters(void (*pntr)(), int *P, int *ID) {
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
void QueTask(void (*pntr)(),int P) 
{
	//uint8_t msg2[] = "QueTask!!\n";
	//sendUART(msg2, sizeof(msg2));
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
			for(int i = TOTALTASKS - 1; i >= InsertionLocation; i--)
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
		if(ReadyQueue[0].Priority == 1)
			cout<< "Task 1 inserted in ready queue\n";
		else if(ReadyQueue[0].Priority == 2)
			cout<< "Task 2 inserted in delay queue\n";
		else if(ReadyQueue[0].Priority == 3)
			cout<< "Task 3 inserted in delay queue\n";
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
		
		for(int i = TOTALTASKS - 1; i >= InsertionLocation; i--)
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
		if(ReadyQueue[InsertionLocation].ID == 0)
			cout<< "Task 1 inserted in ready queue\n";
		else if(ReadyQueue[InsertionLocation].ID == 1)
			cout<< "Task 2 inserted in delay queue\n";
		else if(ReadyQueue[InsertionLocation].ID == 2)
			cout<< "Task 3 inserted in delay queue\n";
		
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
		if(RunningTask_id == 0)
			cout<< "Dispatching Task 1\n";
		else if(RunningTask_id == 1)
			cout<< "Dispatching Task 2\n";
		else if(RunningTask_id == 2)
			cout<< "Dispatching Task 3\n";
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
//Helper function that receive pointer to the RunningTask_id to run it
void RunTask(void (*pntr)()) {
	pntr();     
}
///////////////////////////////////////////////////////////
/********************************************************
This function is used to insert a task to the delay queue 
by passing a pointer to the function and delay value
Delayed tasks are stored in descending order based on the delay ticks
********************************************************/
void QueDelay(void (*pntr)(), int delay) {
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
		if(TempID == 0)
			cout<< "Task 1 inserted in delay queue with delay of " << delay << " ticks\n";
		else if(TempID == 1)
			cout<< "Task 2 inserted in delay queue with delay of " << delay << " ticks\n";
		else if(TempID == 2)
			cout<< "Task 3 inserted in delay queue with delay of " << delay << " ticks\n";
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
				if(DelayQueue[i].ID == 0)
					cout<< "Inside Decrementdelay function remaining delay for Task 1 = " << DelayQueue[i].delay << " !!\n";
				else if(DelayQueue[i].ID == 1)
					cout<< "Inside Decrementdelay function remaining delay for Task 2 = " << DelayQueue[i].delay << " !!\n";
				else if(DelayQueue[i].ID == 2)
					cout<< "Inside Decrementdelay function remaining delay for Task 3 = " << DelayQueue[i].delay << " !!\n";
			}
		}
		//Loop on the exisiting tasks and move tasks that has 0 delay to ready queue.
		for(int j = TOTALTASKS - 1; j >= 0; j--)
		{
			if(DelayQueue[j].pntr != 0 && DelayQueue[j].delay == 0)
			{
				if(DelayQueue[j].ID == 0)
					cout<< "Inside Decrementdelay function Task 1 sent to ready queue !!\n";
				else if(DelayQueue[j].ID == 1)
					cout<< "Inside Decrementdelay function Task 2 sent to ready queue !!\n";
				else if(DelayQueue[j].ID == 2)
					cout<< "Inside Decrementdelay function Task 3 sent to ready queue  !!\n";
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
	if(RunningTask_id == 0)
		cout<< "Inside ReRunMe function called by Task1 !!\n";
	else if(RunningTask_id == 1)
		cout<< "Inside ReRunMe function called by Task2 !!\n";
	else if(RunningTask_id == 2)
		cout<< "Inside ReRunMe function called by Task3 !!\n";
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

////////////////////////////////////////////////////
int SwitchOn = 0;
void Task1(){
	cout<< "Inside Task 1 !!\n";
	if (SwitchOn) 
	{
		SwitchOn = 0;
		QueDelay(Task2, 8);
	}
}
/////////////////////////////////////////////////////////
void Task2() {
	cout<< "Inside Task 2 !!\n";
	if (!SwitchOn) 
	{
		SwitchOn = 1;
		QueDelay(Task1, 4);
	}
}
////////////////////////////////////////////////////
void Task3(void){
	cout<< "Inside Task 3 !!\n";
    ReRunMe(2);
}
///////////////////////////////////////////////////////////
void main()
{
	Init();
	QueTask(Task1, 1); 
	QueTask(Task2, 2); 
	QueTask(Task3, 3); 
	int count = 1;
	for(int i = 0; i < 10; i++)
	{
		cout<< "-------------------Trial# " << count++ << "-------------------\n";
		Dispatch();
		DecrementDelay();
	}
}