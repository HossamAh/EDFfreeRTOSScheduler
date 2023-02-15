/*
 * -----------------------------------------------------------
 * The Idle task.
 * ----------------------------------------------------------
 *
 * The portTASK_FUNCTION() macro is used to allow port/compiler specific
 * language extensions.  The equivalent prototype for this function is:
 *
 * void prvIdleTask( void *pvParameters );
 *
 */
static portTASK_FUNCTION( prvIdleTask, pvParameters )
{
    /* Stop warnings. */
    ( void ) pvParameters;

    /** THIS IS THE RTOS IDLE TASK - WHICH IS CREATED AUTOMATICALLY WHEN THE
     * SCHEDULER IS STARTED. **/

    /* In case a task that has a secure context deletes itself, in which case
     * the idle task is responsible for deleting the task's secure context, if
     * any. */
    portALLOCATE_SECURE_CONTEXT( configMINIMAL_SECURE_STACK_SIZE );

    for( ; ; )
    {
        /* See if any tasks have deleted themselves - if so then the idle task
         * is responsible for freeing the deleted task's TCB and stack. */
        prvCheckTasksWaitingTermination();
			
			#if ( configUSE_PREEMPTION == 0 )
            {
                /* If we are not using preemption we keep forcing a task switch to
                 * see if any other task has become available.  If we are using
                 * preemption we don't need to do this as any task becoming available
                 * will automatically get the processor anyway. */
                taskYIELD();
            }
        #endif /* configUSE_PREEMPTION */

        #if ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) )
            {
                /* When using preemption tasks of equal priority will be
                 * timesliced.  If a task that is sharing the idle priority is ready
                 * to run then the idle task should yield before the end of the
                 * timeslice.
                 *
                 * A critical region is not required here as we are just reading from
                 * the list, and an occasional incorrect value will not matter.  If
                 * the ready list at the idle priority contains more than one task
                 * then a task other than the idle task is ready to execute. */
                if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > ( UBaseType_t ) 1 )
                {
                    taskYIELD();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        #endif /* ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) ) */

        #if ( configUSE_IDLE_HOOK == 1 )
            {
                extern void vApplicationIdleHook( void );

                /* Call the user defined function from within the idle task.  This
                 * allows the application designer to add background functionality
                 * without the overhead of a separate task.
                 * NOTE: vApplicationIdleHook() MUST NOT, UNDER ANY CIRCUMSTANCES,
                 * CALL A FUNCTION THAT MIGHT BLOCK. */
                vApplicationIdleHook();
            }
        #endif /* configUSE_IDLE_HOOK */

        /* This conditional compilation should use inequality to 0, not equality
         * to 1.  This is to ensure portSUPPRESS_TICKS_AND_SLEEP() is called when
         * user defined low power mode  implementations require
         * configUSE_TICKLESS_IDLE to be set to a value other than 1. */
        #if ( configUSE_TICKLESS_IDLE != 0 )
            {
                TickType_t xExpectedIdleTime;

                /* It is not desirable to suspend then resume the scheduler on
                 * each iteration of the idle task.  Therefore, a preliminary
                 * test of the expected idle time is performed without the
                 * scheduler suspended.  The result here is not necessarily
                 * valid. */
                xExpectedIdleTime = prvGetExpectedIdleTime();

                if( xExpectedIdleTime >= configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
                {
                    vTaskSuspendAll();
                    {
                        /* Now the scheduler is suspended, the expected idle
                         * time can be sampled again, and this time its value can
                         * be used. */
                        configASSERT( xNextTaskUnblockTime >= xTickCount );
                        xExpectedIdleTime = prvGetExpectedIdleTime();

                        /* Define the following macro to set xExpectedIdleTime to 0
                         * if the application does not want
                         * portSUPPRESS_TICKS_AND_SLEEP() to be called. */
                        configPRE_SUPPRESS_TICKS_AND_SLEEP_PROCESSING( xExpectedIdleTime );

                        if( xExpectedIdleTime >= configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
                        {
                            traceLOW_POWER_IDLE_BEGIN();
                            portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime );
                            traceLOW_POWER_IDLE_END();
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    ( void ) xTaskResumeAll();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        #endif /* configUSE_TICKLESS_IDLE */
        /*TODO*/
        #if (configUSE_EDF_SCHEDULER == 1)
        {
        /*update the idle task deadline each call*/
        TickType_t currentTick = xTaskGetTickCount();
        TickType_t initIDLEPeriod = 200;
        listSET_LIST_ITEM_VALUE( &( ( pxCurrentTCB )->xStateListItem ), initIDLEPeriod + xTickCount);

        }
        #endif     
    }
}
/***************************************************************/
BaseType_t xTaskIncrementTick( void )
{
    TCB_t * pxTCB;
    TickType_t xItemValue;
    BaseType_t xSwitchRequired = pdFALSE;

    /* Called by the portable layer each time a tick interrupt occurs.
     * Increments the tick then checks to see if the new tick value will cause any
     * tasks to be unblocked. */
    traceTASK_INCREMENT_TICK( xTickCount );

    if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
    {
        /* Minor optimisation.  The tick count cannot change in this
         * block. */
        const TickType_t xConstTickCount = xTickCount + ( TickType_t ) 1;

        /* Increment the RTOS tick, switching the delayed and overflowed
         * delayed lists if it wraps to 0. */
        xTickCount = xConstTickCount;

        if( xConstTickCount == ( TickType_t ) 0U ) /*lint !e774 'if' does not always evaluate to false as it is looking for an overflow. */
        {
            taskSWITCH_DELAYED_LISTS();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        /* See if this tick has made a timeout expire.  Tasks are stored in
         * the  queue in the order of their wake time - meaning once one task
         * has been found whose block time has not expired there is no need to
         * look any further down the list. */
        if( xConstTickCount >= xNextTaskUnblockTime )
        {
            for( ; ; )
            {
                if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
                {
                    /* The delayed list is empty.  Set xNextTaskUnblockTime
                     * to the maximum possible value so it is extremely
                     * unlikely that the
                     * if( xTickCount >= xNextTaskUnblockTime ) test will pass
                     * next time through. */
                    xNextTaskUnblockTime = portMAX_DELAY; /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
                    break;
                }
                else
                {
                    /* The delayed list is not empty, get the value of the
                     * item at the head of the delayed list.  This is the time
                     * at which the task at the head of the delayed list must
                     * be removed from the Blocked state. */
                    pxTCB = listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList ); /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */
                    xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );

                    if( xConstTickCount < xItemValue )
                    {
                        /* It is not time to unblock this item yet, but the
                         * item value is the time at which the task at the head
                         * of the blocked list must be removed from the Blocked
                         * state -  so record the item value in
                         * xNextTaskUnblockTime. */
                        xNextTaskUnblockTime = xItemValue;
                        break; /*lint !e9011 Code structure here is deemed easier to understand with multiple breaks. */
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* It is time to remove the item from the Blocked state. */
                    listREMOVE_ITEM( &( pxTCB->xStateListItem ) );

                    /* Is the task waiting on an event also?  If so remove
                     * it from the event list. */
                    if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
                    {
                        listREMOVE_ITEM( &( pxTCB->xEventListItem ) );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    /* TODO: after removing the task from delayed list Calculate new deadline for the task and insert it in ready list  */
					#if ( configUSE_EDF_SCHEDULER == 1)
							{
								listSET_LIST_ITEM_VALUE( &( ( pxTCB )->xStateListItem ), ( ( pxTCB )->xTaskPeriod )+ xTickCount );
							}
					#endif
                    /* Place the unblocked task into the appropriate ready
                     * list. */
                    prvAddTaskToReadyList( pxTCB );
                    
                    /* A task being unblocked cannot cause an immediate
                     * context switch if preemption is turned off. */
                    #if ( configUSE_PREEMPTION == 1 )
                        {
                            #if(configUSE_EDF_SCHEDULER==0)
                                {
									/* Preemption is on, but a context switch should
									 * only be performed if the unblocked task has a
									 * priority that is equal to or higher than the
									 * currently executing task. */
									if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority )
									{
										xSwitchRequired = pdTRUE;
									}
									else
									{
										mtCOVERAGE_TEST_MARKER();
									}
                                }
                                #else
                                {
                                    /*TODO*/
                                    /*preempre accorrding to new deadlines*/									
                                    /* Preemption is on, but a context switch should
									 * only be performed if the unblocked task has a
									 * deadline that is equal to or lower than the
									 * currently executing task. */
									if( pxCurrentTCB->xStateListItem.xItemValue >= pxTCB->xStateListItem.xItemValue )
									{
										xSwitchRequired = pdTRUE;
									}
									else
									{
										mtCOVERAGE_TEST_MARKER();
									}
                                }
                                #endif
                        }
                    #endif /* configUSE_PREEMPTION */
                }
            }
        }

        /* Tasks of equal priority to the currently running task will share
         * processing time (time slice) if preemption is on, and the application
         * writer has not explicitly turned time slicing off. */
        #if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) )
            {
                if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ pxCurrentTCB->uxPriority ] ) ) > ( UBaseType_t ) 1 )
                {
                    xSwitchRequired = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        #endif /* ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) ) */

        #if ( configUSE_TICK_HOOK == 1 )
            {
                /* Guard against the tick hook being called when the pended tick
                 * count is being unwound (when the scheduler is being unlocked). */
                if( xPendedTicks == ( TickType_t ) 0 )
                {
                    vApplicationTickHook();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        #endif /* configUSE_TICK_HOOK */

        #if ( configUSE_PREEMPTION == 1 )
            {
                if( xYieldPending != pdFALSE )
                {
                    xSwitchRequired = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        #endif /* configUSE_PREEMPTION */
    }
    else
    {
        ++xPendedTicks;

        /* The tick hook gets called at regular intervals, even if the
         * scheduler is locked. */
        #if ( configUSE_TICK_HOOK == 1 )
            {
                vApplicationTickHook();
            }
        #endif
    }

    return xSwitchRequired;
}
