/*
  PollingTest.ino - Test sketch for Polling library.
  Copyright (c) 2014 Marco Sillano.  All right reserved.

This example shows how to use the Polling library.
It uses 2 tasks: loop1() and loop2(), the first every 4/5 sec, the second every 20 sec.

USE
Compile this sketch and start Console. Requires a standard Arduino Yùn.
*/

#include <Console.h>
#include <YunServer.h>
#include <YunClient.h>
#include <Polling.h>

// to get Timestamp from OpenWrt-Yùn
#include <Process.h>
#define FORMATTIME "+%F %T"

String getTimestamp(String sFormat = FORMATTIME);

YunServer server;

// Polling step 0, define tasks
// tasks definitions:  starTime, period, function, status (one of WAITING RUNNING ONESHOT DELETED)

task task1 = {     800,  4000, loop1, RUNNING};  // every 4 seconds
task task2 = {    9000, 20000, loop2, RUNNING};  // every 20 seconds

// task1 = 800 + k* 4000:  800, 4800, 8800, 12800, 16800 ... ms.
// task2 = 9000 + k* 20000: 9000, 29000, 49000, 69000 ... ms.

// task2:           YY                  YY                  YY
// task1:  X   X   X   X   X   X   X   X   X   X   X   X   X
// sec     012345678901234567890123456789012345678901234567890

// Polling step 1, create a Pollng instance (max 2 tasks)
Polling myTasks(2);

long run = 0;

void setup() {
  Bridge.begin();
  Console.begin();
  // Wait for the Console port to connect
  while (!Console);
  Console.println(F("POLLING LIBRARY DEMO TEST"));
  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();

  // Polling step 2, add task
  myTasks.add(task1);
  myTasks.add(task2);

  Console.print(F("  *****  startime: "));
  Console.println(millis());

  // Polling step 3, start engine
  myTasks.begin();
}

void loop() {
  // Polling loop:
  long free = myTasks.loopPolling();

  if ( free > 1000) {
    // 1000 ms free: enough time to do more tasks asyncronously
    // e.g. processing BRIDGE requests
    YunClient client = server.accept();
    // There is a new client?
    if (client) {
      // read the command, do only echo on Console
      String command = client.readString();
      command.trim();               //kill whitespaces
      Console.println(command);
      //
      client.stop();
    }// if client
  }
}

//--------------------------------------------------

// short task
void loop1() {
  // echo for test
  unsigned long t = millis();
  Console.print(F( "loop1: "));                  // only print                  
  Console.print( t );
  String duration = String(" [") + (millis() - t) + String(" ms]");
  Console.println( duration );                 // and duration
}

// more long task
void loop2() {
  long t = millis();
  Console.print(F("    loop2: "));
  Console.print(t);
  Console.print("   " + getTimestamp() );    // get and print timestamp
  String duration = " [" + String(millis() - t) + " ms]";
  Console.println( duration );               // and duration

  // we can modify the task's fields to do special beavoir:
  run++;
  switch (run % 10) {
    case 2:
      Console.println(F(">>> suspend loop1 for 20 seconds."));
      // set status to WAITING for a suspended task.
      task1.status = WAITING;
      break;
    case 3:
      Console.println(F(">>> restart loop1"));
      // to set status to RUNNING restarts a suspended task.
      task1.status = RUNNING;
      break;
    case 4:
      Console.println(F(">>> loop1, single run after loop2 (2 times)"));
       // set status to ONESHOT for a single run: after status will be WAITING.
      task1.status = ONESHOOT;
      break;
    case 5:                  
      // set status to ONESHOT for a single run: after status will be WAITING.
      task1.status = ONESHOOT;
      break;
    case 6:
      Console.println(F(">>> now loop1 every 5 sec"));
      // to set status to RUNNING  restarts a suspended task.
      task1.status = RUNNING;
      // it is possible to change also period (never change startTime!)
      task1.period = 5000;
      break;
    case 7:
      Console.println( F(">>> now loop1 deleted"));
      // the task will be deleted from the quee later, at next loop (0-5 sec)
      task1.status = DELETED;
      break;
    case 8:
      Console.print(F(">>> running tasks = "));
      Console.println(myTasks.getSize());
      Console.println(F(">>> loop1 restored and added again"));
      // restoring task1
      task1.period = 4000;
      task1.status = RUNNING;
      // add() can be done any time, before and after begin()
      // but take care to max task number declared at start
      myTasks.add(task1);
      Console.print(F(">>> running tasks = "));
      Console.println(myTasks.getSize());
      break;

#ifdef POLL_DEBUG
    case 9:
      Console.print( F("  *****  DEBUG: in this test cyle minGap = "));
      Console.println( String(myTasks.getMinGap()) + " ms");
#endif
  }
}

// --------------------------------------------------------
// get timestamp from OpenWrt-Yùn
// note: for a faster timestamp from a RTC see  getDS1307ts library and
// RTC_timestamp.ino example.

String getTimestamp(String sFormat) {
  Process date;
  date.begin("date");
  date.addParameter(sFormat);
  date.run();
  if (date.available() > 0) {
    // get the result of the date process (should be YYYY-MM-DD hh:mm:ss):
    String now =  date.readString();
    now.trim();
    return (now);
  }
  return ("error");
}
/*
  This program is free software; you can redistribute it and/or
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
