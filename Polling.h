/*
  Polling.h -  Library for time scheduled Arduino applications.
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

#ifndef Polling_h
#define Polling_h

#include "Arduino.h"
#include <inttypes.h>


// Task status
typedef enum {WAITING, RUNNING, ONTIME, ONESHOOT, DELETED
}  taskStatus_t ;


// adds the extra function getMinGap() for debug
// #define POLL_DEBUG

// Type for 'void loop()' functions
typedef  void (*loopFunction)();

// Type for task definitions
typedef  struct task {
  unsigned long startTime;
  unsigned long period;
   loopFunction fLoop;
   taskStatus_t status;
};

// Internal used task pointer type
typedef task* _task_ptr;


class Polling {
  public:
    Polling (const byte nTask );
    void begin(); 
    void add(task& aTask);
    long loopPolling();
	
 inline int getSize(){ return(_size);}          // return number of tasks

#ifdef POLL_DEBUG
    long getMinGap();
#endif	

  private:
    _task_ptr* _tasks;
    byte  _size ;
    byte  _max;
#ifdef POLL_DEBUG
    long  _minGap;
#endif	

    static int _compareTask(const void* a, const void* b);
    void _adjustTime(task& aTask);
};


#endif