/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include "string.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "Queue.h"
/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )


/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/

TaskHandle_t xLEDHandle = NULL;
TaskHandle_t xButton1Handle = NULL;
TaskHandle_t xButton2Handle = NULL;
TaskHandle_t xTransmitterHandle = NULL;
TaskHandle_t xUart_ReceiverHandle = NULL;
TaskHandle_t xLoad_1_SimulationHandle = NULL;
TaskHandle_t xLoad_2_SimulationHandle = NULL;

void vLEDTask( void * pvParameters );
void vButton1Task( void * pvParameters );
void vButton2Task( void * pvParameters );
void vTransmitterTask( void * pvParameters );
void vUart_ReceiverTask( void * pvParameters );
void vLoad_1_SimulationTask( void * pvParameters );
void vLoad_2_SimulationTask( void * pvParameters );

/*timing variable of system Tasks*/
unsigned int Button_1_Monitor_start;
unsigned int Button_2_Monitor_start;
unsigned int Periodic_Transmitter_start;
unsigned int Uart_Receiver_start;
unsigned int Load_1_Simulation_start;
unsigned int Load_2_Simulation_start;
unsigned int Total_Execution_Time;
unsigned int CPU_Load;

pinState_t ButtonState;
QueueHandle_t xQueue;//for transmission task

QueueHandle_t xQueue1;//for buttons monitoring tasks

#define POSITIVE_EDGE_BUTTON1	"\nP1"
#define NEGATIVE_EDGE_BUTTON1	"\nN1"
#define POSITIVE_EDGE_BUTTON2	"\nP2"
#define NEGATIVE_EDGE_BUTTON2	"\nN2"
#define TRANSMISSION_TASK_MSG	"\nTransmission TASK"
#define QUEUE_SIZE 				100
#define MAX_MSG_SIZE			18
/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )

{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	xQueue = xQueueCreate( QUEUE_SIZE, sizeof(unsigned char *));
	xQueue1 = xQueueCreate( QUEUE_SIZE, sizeof(unsigned char *));
	/* Create Tasks here */
		xTaskPeriodicCreate(
                    vButton1Task,       /* Function that implements the task. */
                    "Button_1_Monitor",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    2,/* Priority at which the task is created. */
                    &xButton1Handle,50 );      /* Used to pass out the created task's handle. */

		xTaskPeriodicCreate(
                    vButton2Task,       /* Function that implements the task. */
                    "Button_2_Monitor",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    2,/* Priority at which the task is created. */
                    &xButton2Handle,50 );      /* Used to pass out the created task's handle. */
		xTaskPeriodicCreate(
                    vTransmitterTask,       /* Function that implements the task. */
                    "Periodic_Transmitter",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    2,/* Priority at which the task is created. */
                    &xTransmitterHandle,100 );      /* Used to pass out the created task's handle. */
		xTaskPeriodicCreate(
                    vUart_ReceiverTask,       /* Function that implements the task. */
                    "Uart_Receiver",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    2,/* Priority at which the task is created. */
                    &xUart_ReceiverHandle,20 );      /* Used to pass out the created task's handle. */
										
		xTaskPeriodicCreate(
                    vLoad_1_SimulationTask,       /* Function that implements the task. */
                    "Load_1",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    2,/* Priority at which the task is created. */
                    &xLoad_1_SimulationHandle,10 );      /* Used to pass out the created task's handle. */
		xTaskPeriodicCreate(
                    vLoad_2_SimulationTask,       /* Function that implements the task. */
                    "Load_2",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    2,/* Priority at which the task is created. */
                    &xLoad_2_SimulationHandle,100 );      /* Used to pass out the created task's handle. */
	
	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/
/* Implement tick hook */
void vApplicationTickHook(void)
{
	GPIO_write(PORT_0, PIN9, PIN_IS_HIGH);
	GPIO_write(PORT_0, PIN9, PIN_IS_LOW);
}

/* LED Task to be created. */
void vLEDTask( void * pvParameters )
{
    for( ;; )
    {
			if(ButtonState == PIN_IS_HIGH)
			{
				GPIO_write(PORT_0,PIN0,PIN_IS_HIGH);
			}
			else if(ButtonState == PIN_IS_LOW)
			{
				GPIO_write(PORT_0,PIN0,PIN_IS_LOW);
			}
			vTaskDelay(80);
        /* Task code goes here. */
    }
}

/* Button Task to be created. */
void vButton1Task( void * pvParameters )
{
	pinState_t prevState,currentState;
	unsigned char msg[MAX_MSG_SIZE];
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = 50;
	unsigned char i;
	prevState = GPIO_read(PORT_0,PIN0);

    for( ;; )
    {
			currentState = GPIO_read(PORT_0,PIN0);
			/*Negative edge detected*/
			if((currentState == PIN_IS_LOW) && (prevState == PIN_IS_HIGH))
			{
				strcpy(msg,NEGATIVE_EDGE_BUTTON1);
				/*send to consumer task via message queue*/
				for(i=0;i<4;i++)
					xQueueSend( xQueue1, &msg[i], 10);
				xQueueReset(xQueue1);
			}
			/*Positive edge*/
			else if((currentState == PIN_IS_HIGH) && (prevState == PIN_IS_LOW))
			{
				strcpy(msg,POSITIVE_EDGE_BUTTON1);
				/*send to consumer task via message queue*/
				for(i=0;i<4;i++)
					xQueueSend( xQueue1, &msg[i], 10);
				xQueueReset(xQueue1);
			}
			else{}
			prevState = currentState;
			vTaskDelayUntil( &xLastWakeTime, xFrequency );
    }
}
/* Button Task to be created. */
void vButton2Task( void * pvParameters )
{
	pinState_t prevState,currentState;
	unsigned char msg[MAX_MSG_SIZE];
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = 50;
	unsigned char i;
	prevState = GPIO_read(PORT_0,PIN1);
	
  for( ;; )
    {
			currentState = GPIO_read(PORT_0,PIN1);
			/*Negative edge detected*/
			if((currentState == PIN_IS_LOW) && (prevState == PIN_IS_HIGH))
			{
				strcpy(msg,NEGATIVE_EDGE_BUTTON2);
				/*send to consumer task via message queue*/
				for(i=0;i<4;i++)
					xQueueSend( xQueue1, &msg[i], 10);
				xQueueReset(xQueue1);
			}
			/*Positive edge*/
			else if((currentState == PIN_IS_HIGH) && (prevState == PIN_IS_LOW))
			{
				strcpy(msg,POSITIVE_EDGE_BUTTON2);
				/*send to consumer task via message queue*/
				for(i=0;i<4;i++)
					xQueueSend( xQueue1, &msg[i], 10);
				xQueueReset(xQueue1);
				
			}
			else{}
			prevState = currentState;
			vTaskDelayUntil( &xLastWakeTime, xFrequency );
    }
}

void vTransmitterTask( void * pvParameters )
{
	unsigned char msg[] = TRANSMISSION_TASK_MSG;
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = 100;
	const char newline = '\n';
	unsigned int i;
	for(;;)
	{
		/*send to consumer task via message queue*/
		for(i=0;i<MAX_MSG_SIZE;i++)
			xQueueSend( xQueue, &msg[i], 10);
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}
}
void vUart_ReceiverTask( void * pvParameters )
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = 20;
	signed char msgchar;
	signed char msg[MAX_MSG_SIZE];
	unsigned char counter=0;
	for(;;)
	{
		if( uxQueueMessagesWaiting(xQueue) != 0)
		{
			for( counter=0 ; counter<MAX_MSG_SIZE+1; counter++)
			{
				xQueueReceive( xQueue, (signed char*)&msg[counter] , 0);
			}
			vSerialPutString( (signed char *) msg, strlen(msg));
			xQueueReset(xQueue);
		}
		if( uxQueueMessagesWaiting(xQueue1) != 0)
		{
			for( counter=0 ; counter<4; counter++)
			{
				xQueueReceive( xQueue1, (signed char*)&msg[counter] , 0);
			}
			vSerialPutString( (signed char *) msg, strlen(msg));
			xQueueReset(xQueue1);
		}
		// if(xQueueReceive(xQueue,(signed char*)&msgchar,0)==pdTRUE)
		// {
		// 	xSerialPutChar(msgchar);
		// }
		// if(strlen(msg)>0)
		// {
		// 	for(counter=0;counter<strlen(msg);counter++)
		// 	{
		// 		xSerialPutChar(msg[counter]);
		// 	}
		// }
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}
}
/* 1 ms equal 1200 cycle */
void vLoad_1_SimulationTask( void * pvParameters )
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = 10;
	unsigned int counter=0;
	unsigned int LimitValue=12000*5; /*5 ms*/
	for(;;)
	{
		for(counter=0;counter<LimitValue;counter++);
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}
}
void vLoad_2_SimulationTask( void * pvParameters )
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = 100;
	unsigned int counter=0;
	unsigned int LimitValue=12000 * 12; /*12 ms*/
	for(;;)
	{
		for(counter=0;counter<=LimitValue;counter++);
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}
}

