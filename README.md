## freeRTOS EDF schudler

An implemenation of EDF schedular in freeRTOS and test it with 6 different periodic tasks

## Tasks:
* Task 1: "Button_1_Monitor", {Periodicity: 50, Deadline: 50}
This task will monitor rising and falling edge on button 1 and send this event to the consumer task.

* Task 2: "Button_2_Monitor", {Periodicity: 50, Deadline: 50}
This task will monitor rising and fallin edge on button 2 and send this event to the consumer task.

* Task 3: "Periodic_Transmitter", {Periodicity: 100, Deadline: 100}
This task will send preiodic string every 100ms to the consumer task

* Task 4: "Uart_Receiver", {Periodicity: 20, Deadline: 20}
This is the consumer task which will write on UART any received string from other tasks

* Task 5: "Load_1_Simulation", {Periodicity: 10, Deadline: 10}, Execution time: 5ms

* Task 6: "Load_2_Simulation", {Periodicity: 100, Deadline: 100}, Execution time: 12ms

# Used software tools:
* Keil is an IDE and simulation tool 
* Simso for simulate the tasks and check its schudelability

# Snap of simso results:
![simulation result](/simso.png)


**Project is implemented and tested on ARM architecutre**
