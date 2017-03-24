# Arduino-library-Polling
The Polling library schedules ripetitive activities using 1 ms precision.

An activity is defined filling a "task" variable. Example:
        task task1;
        task1.startTime = 10000;      // initial wait time in ms.
        task1.period    =  4000;      // repetion time in ms.
        task1.fLoop     = loop1;      // function executing the activity
        task1.status    = RUNNING;    
         // one of RUNNING | WAITING | WORKTIME | ONESHOOT | DELETED
  or, more simple:
        task task1		= {10000, 4000, loop2, RUNNING}; 
                            // every 4 seconds does loop2()
  In general a task is called at time = startTime + k * period
