/*
  Polling.cpp - Library for time scheduled Arduino applications.
  Copyright (c) 2014 Marco Sillano.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/* 
  The Polling library schedules ripetitive activities using 1 ms precision.
  An activity is defined filling a "task" variable. Example:
        task task1;
        task1.startTime = 10000;      // initial wait time in ms.
        task1.period    =  4000;      // repetion time in ms.
        task1.fLoop     = loop1;      // function executing the activity
        task1.status    = RUNNING;    
         // one of RUNNING | WAITING | WORKTIME | ONESHOOT | DELETED
  or, more simple:
        task task1		= {10000, 4000, loop1, RUNNING}; 
                            // every 20 seconds does loop2()
  In general a task is called at time = startTime + k * period
  note: as in http://arduino.cc/en/Reference/Millis the max time for Polling 
        is 50 days before overflow.
  note: if a task will complete too late, next task can start delayed. 
        (see POOL_DEBUG)
  note: The code is optimized for smallest footprint.

  To use Polling library see PollingTest.ino:
      0)  Create global vars for required tasks as see before.
      1)  Create a global Polling instance using constructor:
                Polling  myPolling(<max_num_task>);
	    2)  In Setup, add to myPolling all tasks:		
				        myPolling.add(task1);
      3)  In Setup, start myPolling using:
                myPolling.begin();
      4)  In Loop call  (and not use delay() in main loop()function):  
	              myPolling.loopPolling()
				
  DEBUG
     If POOL_DEBUG is defined (see Polling.h), it is avaiable the function 
     getMinGap(), to get the smallest inter-task delay in ms. If it is 0 
     or less, some task is too long, and next task can be delayed.
	   Calling this function will restart the MinGap check.
  */

#include "Polling.h"

// Constructor
// first step: initializes Polling object
// nTask = max number of queed tasks in sketch
Polling::Polling(byte nTask ) {
    _size = 0;
    _max = nTask;
    // using dynamic array constructor 
    _tasks = ( _task_ptr*) malloc(sizeof(_task_ptr) * nTask);
#ifdef POLL_DEBUG
    _minGap = -100000L;
#endif	

}

// second step: adds tasks to Polling quee
// this can be done also after begin().
void Polling::add(task& aTask) {
    if (_size < _max) {
        _tasks[_size++] =  &aTask;
        _adjustTime(aTask); 
   }
}

// third step: starting polling engine
void Polling::begin() {
  //  for (byte i = 0 ;  i < _size; i++) {
  //      _adjustTime(* _tasks[i]); 
  //   }
    qsort( _tasks, _size, sizeof(_task_ptr), _compareTask);
}

// This must be called in loop() function.
// If it is the rigth time this function executes a task. 
// Returns the number of milliseconds to next task: if
// negative, next task is delayed.
long  Polling::loopPolling() {
    long tnext;
    if (_tasks[0]->startTime <= millis() ) {      // time to do something?
        switch ( _tasks[0]->status) {             // status controlled actions
            case ONESHOOT:{
                 _tasks[0]->status = WAITING;      // intentional missed break
                 }
            case RUNNING:                         // do the task
            case ONTIME: {                         // same as RUNNING, do the task
                 (*(_tasks[0]->fLoop))();          // intentional missed break
                 }
            case WAITING: {
   // updates for next run, after doing fLoop() so it can change task values
                _adjustTime(* _tasks[0]); 
                break;
				}
            case DELETED:{
                _size--;                // now safe to remove deleted task
	            _tasks[0] = _tasks[_size];
				}
            default:
        /* nothing to do */ ;
        }
//	if (log){
// 	 strcpy(log,  _tasks[0]->id);
//	}
    qsort( _tasks, _size, sizeof(_task_ptr), _compareTask);  // next task
#ifdef POLL_DEBUG
    tnext = ( _tasks[0]->startTime > millis()) ? _tasks[0]->startTime - millis(): -long( millis()-_tasks[0]->startTime);
    _minGap = (_minGap == -100000L) ? tnext: ((tnext < _minGap) ? tnext: _minGap);
#endif
    }
    tnext = ( _tasks[0]->startTime > millis()) ? _tasks[0]->startTime - millis(): -long( millis()-_tasks[0]->startTime);
    return (tnext);                       // ms to next task
}

#ifdef POLL_DEBUG
    long Polling::getMinGap(){           // min time between tasks
       long g = _minGap;
       _minGap = -100000L;                  // initialize
       return (g);
    }
#endif	

// private, used by qsort(), ascending startTime order
int Polling::_compareTask(const void* a, const void* b) {
     return ((*(_task_ptr*)a)->startTime > (*(_task_ptr*)b)->startTime)? 1: -1;
 }

// private, updates startTime
void Polling::_adjustTime(task& aTask) {
        while (aTask.startTime <= millis() ){
            aTask.startTime +=  aTask.period;
	    }
 }
  
  

