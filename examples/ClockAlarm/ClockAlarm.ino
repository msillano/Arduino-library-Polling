/*
This small class adds special timed tasks, called Alarms, to Polling task.
Alarms have a precision +/- 1 sec, and an Alarm is defined by 2 Timestamp:
start and adding.
Exemple: {"2014-01-01 00:01:30", "0-1-0 0:0:0"} => <the firsth of every
month, at 0h 1' 30">.
Like Polling tasks, the "add" Timestamp can be modified at runtime, to allow
complex beavoirs. This class uses directly Timestamp and millis (and not
tics and time) so it is simple and very compact.
It requires Polling library (a RTC is very usefull for precision, but not
required).
*/

#include <Bridge.h>
#include <Console.h>
#include <Polling.h>

// only if RTC clock is present:
// #include <Wire.h>
// #include <getDS1307ts.h> // or any library implementing a RTC getNowTimestamp().
// #include <I2Ctunnel.h>   // required if USE_I2C is defined (see getDS1307ts.h)
// ------------- end only if

#ifndef getDS1307timeStamp_h
#include <Process.h>
#endif

// class AlarmClock.h
#define MILLIS_PER_HOUR (3600000UL)
#define MILLIS_PER_DAY  (MILLIS_PER_HOUR * 24UL)
// leap year calulator
#define LEAP_YEAR(Y)     ( ((Y)>0) && !((Y)%4) && ( ((Y)%100) || !((Y)%400) ) )

#define FORMATTIME "+%F %T"
String getNowTimestamp(String sFormat = FORMATTIME); // see later

// definition of an Alarm using 2 Timestamp:
// runTimestamp: like "2014-03-12 12:00:30" => initial Alarm, then time of next
//     Alarm.
//     note: accepts also negative values: they are replaced from now:
//     "-1--1--1 -1:30:00" => the next therty past now.
// addTimestamp: the increment after run, like "0-0-0 1:0:0" => one hour increment
//     note: only addTimestamp can be changed at runtime

typedef struct {
  String runTimestamp;
  String addTimestamp;
} alarmTask, *alarmTaskptr;

// main class
class AlarmClock {
  public:
    AlarmClock(alarmTask& tstamp);
    void begin();
    long loopAlarm(task& aTask);
    void setNext(task& aTask);
    unsigned long getRun() {
      return run;         // counter of Alarm run
    };
    // for user convenience, getters for the Alarm time
    int getSecond() {
      return Second;
    };
    int getMinute() {
      return Minute;
    };
    int getHour() {
      return Hour;
    };
    int getDay() {
      return Day;
    };
    int getMonth() {
      return Month;
    };
    int getYear() {
      return Year;
    };

  private:
    int16_t Second;
    int16_t Minute;
    int16_t Hour;
    int16_t Day;
    int16_t Month;
    int16_t Year;
    uint32_t strMillis;
    alarmTaskptr alarm;
    unsigned long run;

    AlarmClock(String tstamp) ;
    String toTimestamp();
    void add(AlarmClock& y);
    void syncro(AlarmClock& nowt);
    long diffMillis(AlarmClock& nowt);
    void explode(String tstamp) ;
};


// class AlarmClock.cpp

/*
Constructor, ties the new AlarmClock to a given alarmTask,
and updates time fields from alarmTsk.runTimestamp.
*/
AlarmClock::AlarmClock(alarmTask& alarmTsk) {
  explode(alarmTsk.runTimestamp);
  alarm = &alarmTsk;
  run = 1;
}

/*
Initialize the Object:
- replace with now the negative time fields in runTimestamp.
- finds the next Alarm time, adding many times addTimestamp to runTimestamp
*/
void AlarmClock::begin() {
  // to explode now
  AlarmClock _now(getNowTimestamp());
  // replace negative values
  if (Second < 0) Second = _now.Second;
  if (Minute < 0) Minute = _now.Minute;
  if (Hour < 0) Hour = _now.Hour;
  if (Day < 0) Day = _now.Day;
  if (Month < 0) Month = _now.Month;
  if (Year < 0) Year = _now.Year;
  // find first future Alarm time
  syncro(_now);
}

/*
called when Polling  gives the control.
It compares now() with runTimestamp:
 - if still now > runTimestamp: one more
     polling interval.
 - if now <= runTimestamp: adjust and return (to do Alarm)
 return: (runTimestamp - now) as millis
*/
long AlarmClock::loopAlarm(task& aTask) {
  AlarmClock _now(getNowTimestamp()); // to explode now
  long _delta = diffMillis(_now);  // compare runTimestamp with now
  if (_delta > 0) {                //  runTimestamp > now, so wait more
    aTask.period =  _delta ;
  } else {                         //  runTimestamp <= now
    aTask.startTime += ( _delta / 200) * 100; // reduces error
  }
  return (_delta) ;
}

/*
If loopAlarm is ok to run the Alarm,
programmer must call setNext() to calculate next polling time.
(after user actions: the addTimestamp string can be changed)
It updates runTimestamp and aTask.period.
*/
void AlarmClock::setNext(task& aTask) {
  AlarmClock toNext(alarm->addTimestamp);   // only to explode addTimestamp
  add(toNext);                              // add Y, M, D, h, m, s
  alarm->runTimestamp = toTimestamp();      // update runTimestamp: next Alarm
  aTask.period = ((toNext.Hour * 60L + toNext.Minute) * 60L + toNext.Second) * 1000L;
  run++;
}                                         // and next period in millis (for Polling)


// ----------------------------- private

// this Constructor only explodes Timestamp
AlarmClock::AlarmClock(String tstamp) {
  explode(tstamp);
  alarm = NULL;
  run = 1;
}

// find first future Alarm
void AlarmClock::syncro(AlarmClock& nowt) {
  AlarmClock toNext(alarm->addTimestamp);
  while (
    ((Year == nowt.Year) && (Month == nowt.Month) && (Day == nowt.Day) && (Hour == nowt.Hour) && (Minute == nowt.Minute) && (Second < nowt.Second)) ||
    ((Year == nowt.Year) && (Month == nowt.Month) && (Day == nowt.Day) && (Hour == nowt.Hour) && (Minute < nowt.Minute)) ||
    ((Year == nowt.Year) && (Month == nowt.Month) && (Day == nowt.Day) && (Hour < nowt.Hour)) ||
    ((Year == nowt.Year) && (Month == nowt.Month) && (Day < nowt.Day)) ||
    ((Year == nowt.Year) && (Month < nowt.Month)) ||
    (Year < nowt.Year) ) {
    add(toNext);
  }
  alarm->runTimestamp = toTimestamp();
}

// explode Timestamp in elements as int.
void AlarmClock::explode(String tstamp) {
  sscanf(tstamp.c_str(), "%d-%d-%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second );
}


// opposite to explode, regenerates Timestamp (after add, etc.)
String AlarmClock::toTimestamp() {
  char sData [27];  // 26?
  sprintf(sData, "%04u-%02hu-%02hu %02hu:%02hu:%02hu", Year, Month, Day, Hour, Minute, Second);
  return String(sData);
}

// get (this - tnow) as millis (same day)
// or millis to next day 00:00:00
long AlarmClock::diffMillis(AlarmClock& tnow) {
  long tmp;
  if ((Year == tnow.Year) && (Month == tnow.Month) && (Day == tnow.Day)) {
    tmp = ((((Hour - tnow.Hour) * 60L) + (Minute - tnow.Minute)) * 60L + (Second - tnow.Second)) * 1000L;
  }  else {
    tmp = (((((24L - tnow.Hour) * 60L) - tnow.Minute) * 60L - tnow.Second) * 1000L) % MILLIS_PER_DAY;
  }
  return (tmp);
}

static  const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// sum 2 exploded Timestamp, taking care of month and leap year
void AlarmClock::add(AlarmClock& y) {
  byte carry = 0;
  Second += y.Second;
  carry = Second / 60;
  Second %= 60;
  Minute += (y.Minute + carry);
  carry = Minute / 60;
  Minute %= 60;
  Hour += (y.Hour + carry);
  carry = Hour / 24;
  Hour %= 24;
  Day += (y.Day + carry);
  Day--;
  if ((Month == 2) && LEAP_YEAR(Year))
  {
    carry = Day / 29;
    Day %= 29;
  } else {
    carry = Day / monthDays[Month - 1];
    Day %= monthDays[Month - 1];
  }
  Day++;
  Month += (y.Month + carry);
  Month--;
  carry = Month / 12;
  Month %= 12;
  Month++;
  Year += (y.Year + carry);
}

// end AlarmClock

// example of ClockAlarm use ---------------------------------------------------

// Polling step 0, define tasks
// tasks definitions:  starTime, period, function, status (one of WAITING RUNNING ONESHOT DELETED)

task task1 = {     800,  6000, loop1, RUNNING};  // every 6 seconds
task task2 = {    9000, 30000, loop2, RUNNING};  // every 30 seconds

// alarm step 0: Alarm structure:
// "runTimestamp, addTimestamp" :
alarmTask testA = {"-1--1--1 -1:-1:15", "0-0-0 00:01:00"};

//  associated task
task alarm1 = {  5400, 2000, loop3, RUNNING};  // every 2 seconds (initial)

// alarm step 1: create an AlarmClock for every alarmTask.
AlarmClock myAlarm(testA);

// Polling step 1, create Polling instance (max 3 tasks)
Polling myTasks(3);

// standard setup
void setup() {
  Bridge.begin();
  Console.begin();
#ifdef getDS1307timeStamp_h
  Wire.begin();
#endif

  // Wait for the Console port to connect
  while (!Console);

#ifdef getDS1307timeStamp_h
  Console.println(F("ALARM CLOCK DEMO TEST USING RTC"));
#else
  Console.println(F("ALARM CLOCK DEMO TEST USING DATE PROCESS"));
#endif

  // Polling step 2, add tasks
  myTasks.add(alarm1);
  myTasks.add(task1);
  myTasks.add(task2);

  Console.print(F("  *****  startime: "));
  Console.println(millis() + " : " + getNowTimestamp() );

  // Alarm, step 2: start  engine
  myAlarm.begin();
  // Polling step 3, start engine
  myTasks.begin();
  Console.println("  *****  first Alarm: " + testA.runTimestamp);
}

void loop() {
  // Polling loop:
  long free = myTasks.loopPolling();
  if ( free > 500) {
    // 500 ms free time: enough to do more checks asyncronously
    // e.g. processing BRIDGE requests
  }
}

//--------------------------------------------------

// short task
void loop1() {
  // echo for test
  unsigned long t = millis();
  Console.print( F("loop1: " ));                  // only print millis
  Console.print( String(t) + " ...                   " );     // same length as loop2
  String duration = " [" + String(millis() - t) + " ms]";
  Console.println( duration );                 // and duration
}

// same as loop1, but uses also getNowTimestamp()
void loop2() {
  unsigned long t = millis();
  Console.print(F( "    loop2: "));
  Console.print( String( t ) + " : " + getNowTimestamp() );   // get and print timestamp
  String duration = " [" + String(millis() - t) + " ms]";
  Console.println( duration );               // and duration
}

// Alarm task
void loop3() {
  unsigned long t = millis();
  long err = myAlarm.loopAlarm(alarm1) ;
  // special for Alarm:
  if ( err <= 0 ) {      // test
    //  ************** here Alarm actions ***************
    // It is also possible to change here the "addTimestamp" string
    // example: alternate delays of 1' and 1'30"
    if (myAlarm.getRun() % 2) {
      testA.addTimestamp = "0-0-0 0:1:0";   // odd
    } else {
      testA.addTimestamp = "0-0-0 0:1:30";  // even
    }
    // note: short delays are useful only for test.
    // For delays not related to absolute time, better to use Polling alone.
    // ************************************************
    Console.print(  F( "        loop3: " ));
    Console.print(t);
    Console.print(" : ");
    Console.print( getNowTimestamp());
    Console.println( " [err: " + String(err) + " ms]") ; // print loopAlarm() result
    myAlarm.setNext(alarm1);                 // update for next Alarm
    Console.print("        *** Alarm ****  next: " + testA.runTimestamp);
  } else {
    // no run, only more delay
    Console.print( F("        loop3: "));
    Console.print(t);
    Console.println(" : " + getNowTimestamp() );    // get and print timestamp
    Console.print(  "         updated millis to: " + String(alarm1.startTime + alarm1.period));
  }
  String duration = " [" + String(millis() - t) + " ms]";
  Console.println( duration );               // and duration
  Console.flush();
}

// alternatives about getNowTimestamp()

#ifdef getDS1307timeStamp_h
// uses getDS1307timeStamp library
// get now Timestamp from RTC DS1307 (about 13 ms, but 650 byte more)
String getNowTimestamp(String) {
  return ( getDS1307timeStamp());
}
#endif


#ifndef getDS1307timeStamp_h
// --------------------------------------------------------
// get now timestamp from OpenWrt-Yùn (about 1100 ms)
String getNowTimestamp(String sFormat) {
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
#endif


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




