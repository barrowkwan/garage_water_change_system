/*
 * TimeAlarmExample.pde
 *
 * This example calls alarm functions at 8:30 am and at 5:45 pm (17:45)
 * and simulates turning lights on at night and off in the morning
 * A weekly timer is set for Saturdays at 8:30:30
 *
 * A timer is called every 15 seconds
 * Another timer is called once only after 10 seconds
 *
 * At startup the time is set to Jan 1 2011  8:29 am
 */
 
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DS1307RTC.h>
#include "DHT.h"
#include <Twitter.h>
#include <Time.h>
#include <TimeAlarms.h>


#define DS1307_I2C_ADDRESS 0x68  // This is the I2C address
// Arduino version compatibility Pre-Compiler Directives
#if defined(ARDUINO) && ARDUINO >= 100   // Arduino v1.0 and newer
  #define I2C_WRITE Wire.write 
  #define I2C_READ Wire.read
#else                                   // Arduino Prior to v1.0 
  #define I2C_WRITE Wire.send 
  #define I2C_READ Wire.receive
#endif


//DHT dht(2, 22);
//LiquidCrystal_I2C lcd(0x27,16,2);
int needFilterWaterMonitor = 1;


const int relayPin1 = 4;
const int relayPin2 = 5;
const int relayPin3 = 6;
const int relayPin4 = 7;
const int relayPin5 = 8;
const int floatValveHighPin = 3;
const int floatValveLowPin = 9;
const char  *Mon[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};


String getDateTime(int withDate=1){
  /*
  char  *Mon[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  char timestring[22];
  snprintf(timestring,sizeof(timestring),"%02d:%02d:%02d %02d %s %4d",hour(),minute(),second(),day(),Mon[month()],year());
  return String(timestring);
  */
  String timestring = "";
  int sec, min, hr, dayOfMonth, mon, yr;

  sec     = second();
  min     = minute();
  hr       = hour();  // Need to change this if 12 hour am/pm
  dayOfMonth = day();
  mon      = month();
  yr       = year();
  
  
    if (hr < 10)
    timestring += "0";
  timestring += hr;
  timestring += ":";
  if (min < 10)
    timestring += "0";
  timestring += min;
  timestring += ":";
  if (sec < 10)
    timestring += "0";
  timestring += sec;
  if (withDate == 1){
    timestring += " ";
    timestring += dayOfMonth;
    timestring += " ";
    timestring += Mon[mon];
    timestring += " ";
    timestring += yr;
  }
  return timestring;
}

void setup()
{
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x45, 0x1D };
  byte ip[] = { 192, 168, 210, 52 };
  //byte twitter_server[] = { 128, 121, 146, 100 };

  Serial.begin(57600);
  Wire.begin();
  setSyncProvider(RTC.get);
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time");      

  Ethernet.begin(mac, ip);
  
  pinMode(floatValveHighPin, INPUT);
  pinMode(floatValveLowPin, INPUT);
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(relayPin3, OUTPUT);
  pinMode(relayPin4, OUTPUT);
  pinMode(relayPin5, OUTPUT);
  digitalWrite(relayPin1, 0);
  digitalWrite(relayPin2, 1);
  digitalWrite(relayPin3, 1);
  digitalWrite(relayPin4, 0);
  digitalWrite(relayPin5, 1);


/*
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Start...");
*/
  
  //Serial.println(String(RTC.get));
  Alarm.timerRepeat(600,PostTwitterStatus);

  // create the alarms 
  Alarm.alarmRepeat(12,3,0, waterChange);
  Alarm.alarmRepeat(20,3,0, waterChange);
  Alarm.alarmRepeat(4,3,0, waterChange);
//  Alarm.alarmRepeat(dowSaturday,8,30,30,WeeklyAlarm);  // 8:30:30 every Saturday 
//  Alarm.timerRepeat(15, Repeats);            // timer for every 15 seconds    
//  Alarm.timerOnce(10, OnceOnly);             // called once after 10 seconds 


  postTwitter("Garage water change system started : " + getDateTime(1));


}

void  loop(){  
  digitalClockDisplay();
  //waterMonitor();
  Alarm.delay(1000); // wait one second between clock display
}


void waterChange(){
  
  /*
    1. disable fillwater ();
    2. turn filter water off
    3. turn pump on
    4. turn pump off after XXXX second  ( Alarm.timerOnce(....); )
    5. turn clean water on
    6. full?? turn clean water off
    7. turn clean water on after xxxxx second and enable fillwater ( alarm.timerOnce(.... ); )
  */


/*  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WC@ " + getDateTime(0)); */
  postTwitter("Start water change : " + getDateTime());


  needFilterWaterMonitor = 0;
  Alarm.delay(10000);
  turnFilteredWaterOff();


  Alarm.delay(5000);
  turnWaterPumpOn();
  Alarm.delay(180000);
  turnWaterPumpOff();
  Alarm.delay(5000);
  turnCleanWaterOn();
  Alarm.delay(180000);
  turnCleanWaterOff();
  Alarm.delay(3600000);
  turnCleanWaterOnAndStartMonitor();
}

void turnFilteredWaterOff(){
  postTwitter("Turn filtered water off : " + getDateTime());
  digitalWrite(relayPin1, 1);
  digitalWrite(relayPin2, 0);
}

void turnWaterPumpOn(){
  postTwitter("Turn water pump on : " + getDateTime());
  digitalWrite(relayPin5, 0);
}

void turnWaterPumpOff(){
  postTwitter("Turn water pump off : " + getDateTime());
  digitalWrite(relayPin5, 1);
}

void turnCleanWaterOn(){
  postTwitter("Turn clean water on : " + getDateTime());
  digitalWrite(relayPin3, 0);
  digitalWrite(relayPin4, 1);
}

void turnCleanWaterOff(){
  postTwitter("Turn clean water off : " + getDateTime());
  digitalWrite(relayPin3, 1);
  digitalWrite(relayPin4, 0);
}

void turnCleanWaterOnAndStartMonitor(){
  postTwitter("Turn clean water on after water is warm : " + getDateTime());
  needFilterWaterMonitor = 1;
  digitalWrite(relayPin3, 0);
  digitalWrite(relayPin4, 1);
}


void waterMonitor(){
   /*
   1.  both indicate high - turn clean water on
   2.  both indicate low - turn clean water off
   */
  if (needFilterWaterMonitor == 1){
    if ((digitalRead(floatValveHighPin) == 0) && (digitalRead(floatValveLowPin) == 0)){
      Serial.println("Water monitor turn clean water on : " + getDateTime());
      digitalWrite(relayPin3, 0);
      digitalWrite(relayPin4, 1);
      
    }
    if (digitalRead(floatValveHighPin) && digitalRead(floatValveLowPin)){
      Serial.println("Water monitor turn clean water off : " + getDateTime());
      digitalWrite(relayPin3, 1);
      digitalWrite(relayPin4, 0);
    }
  }
}


void digitalClockDisplay()
{
  // digital clock display of the time
  
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();

/*  lcd.setCursor(0,0);
  lcd.print(getDateTime(0)); */
}

void printDigits(int digits)
{
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void PostTwitterStatus(){
/*
  float temperature  = dht.readTemperature(true);
  float humid = dht.readHumidity();
  char temp[20];
  char humidstr[20];
  if (isnan(temperature)) {
    strcpy(temp, "Cannot get Temp");
  } else {
    dtostrf(temperature,5,2,temp);
  }
  if (isnan(humid)) {
    strcpy(humidstr, "Cannot get Humid");
  } else {
    dtostrf(humid,5,2,humidstr);
  }
  //Serial.println("Status : " + getTime() + " | " + String(temp) + " | " + String(humidstr));
  postTwitter("Status : " + getTime() + " | " + String(temp) + " | " + String(humidstr));
  */
  postTwitter("Status : " + getDateTime());
}

void postTwitter(String msg){
  // Twitter account : garage@barrowkwan.com  "Barrow Garage Aquari"
  
  Twitter twitter("927020131-DEvJRTHsclJFOVEQ85ahXeH4vmYTSLGOEEDi01Vg");
  char tmsg[msg.length()+1];
  msg.toCharArray(tmsg,(msg.length()+1));
  if (twitter.post(tmsg)) {
    int status = twitter.wait(&Serial);
    
    if (status == 200) {
      Serial.println("Tweets : " + getDateTime());
    } else {
      Serial.println("Twitter failed :" + status);
    }
  } else {
    Serial.println("Twitter connection failed.");
  }
  Alarm.delay(1000);
  Serial.println(msg);
}



// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

