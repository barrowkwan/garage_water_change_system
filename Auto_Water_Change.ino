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

DHT dht(2, 22);
LiquidCrystal_I2C lcd(0x27,16,2);
int needFilterWaterMonitor = 1;

const int relayPin1 = 4;
const int relayPin2 = 5;
const int relayPin3 = 6;
const int relayPin4 = 7;
const int relayPin5 = 8;
const int floatValveHighPin = 3;
const int floatValveLowPin = 9;

void setup()
{
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x45, 0x1D };
  byte ip[] = { 192, 168, 210, 52 };
  byte twitter_server[] = { 128, 121, 146, 100 };

  Serial.begin(9600);
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

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Start...");

  //Serial.println(String(RTC.get));
  Alarm.timerRepeat(600,PostTwitterStatus);

  // create the alarms 
  Alarm.alarmRepeat(9,30,0, waterChange);  
//  Alarm.alarmRepeat(dowSaturday,8,30,30,WeeklyAlarm);  // 8:30:30 every Saturday 
//  Alarm.timerRepeat(15, Repeats);            // timer for every 15 seconds    
//  Alarm.timerOnce(10, OnceOnly);             // called once after 10 seconds 


  postTwitter("Garage water change system started : " + getTime());

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
  int jobInt = 0;
  lcd.setCursor(0,1);
  lcd.print("WC@ " + getTimeOnly());
  postTwitter("Start water change : " + getTime());
  needFilterWaterMonitor = 0;
  jobInt = jobInt + 10;
  Alarm.timerOnce(jobInt,turnFilteredWaterOff);
  jobInt = jobInt + 2;
  Alarm.timerOnce(jobInt,turnFilteredWaterOff);
  jobInt = jobInt + 5;
  Alarm.timerOnce(jobInt,turnWaterPumpOn);
  jobInt = jobInt + 2;
  Alarm.timerOnce(jobInt,turnWaterPumpOn);
  jobInt = jobInt + 180;
  Alarm.timerOnce(jobInt,turnWaterPumpOff);
  jobInt = jobInt + 2;
  Alarm.timerOnce(jobInt,turnWaterPumpOff);
  jobInt = jobInt + 5;
  Alarm.timerOnce(jobInt,turnCleanWaterOn);
  jobInt = jobInt + 2;
  Alarm.timerOnce(jobInt,turnCleanWaterOn);
  jobInt = jobInt + 180;
  Alarm.timerOnce(jobInt,turnCleanWaterOff);
  jobInt = jobInt + 2;
  Alarm.timerOnce(jobInt,turnCleanWaterOff);
  jobInt = jobInt + 3600;
  Alarm.timerOnce(jobInt, turnCleanWaterOnAndStartMonitor);
  jobInt = jobInt + 2;
  Alarm.timerOnce(jobInt, turnCleanWaterOnAndStartMonitor);
}

void turnFilteredWaterOff(){
  postTwitter("Turn filtered water off : " + getTime());
  digitalWrite(relayPin1, 1);
  digitalWrite(relayPin2, 0);
}

void turnWaterPumpOn(){
  postTwitter("Turn water pump on : " + getTime());
  digitalWrite(relayPin5, 0);
}

void turnWaterPumpOff(){
  postTwitter("Turn water pump off : " + getTime());
  digitalWrite(relayPin5, 1);
}

void turnCleanWaterOn(){
  postTwitter("Turn clean water on : " + getTime());
  digitalWrite(relayPin3, 0);
  digitalWrite(relayPin4, 1);
}

void turnCleanWaterOff(){
  postTwitter("Turn clean water off : " + getTime());
  digitalWrite(relayPin3, 1);
  digitalWrite(relayPin4, 0);
}

void turnCleanWaterOnAndStartMonitor(){
  postTwitter("Turn clean water on after water is warm : " + getTime());
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
      Serial.println("Water monitor turn clean water on : " + getTime());
      digitalWrite(relayPin3, 0);
      digitalWrite(relayPin4, 1);
      
    }
    if (digitalRead(floatValveHighPin) && digitalRead(floatValveLowPin)){
      Serial.println("Water monitor turn clean water off : " + getTime());
      digitalWrite(relayPin3, 1);
      digitalWrite(relayPin4, 0);
    }
  }
}


void digitalClockDisplay()
{
  // digital clock display of the time
  
  /*Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();*/
  lcd.setCursor(0,0);
  lcd.print(getTimeOnly());
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
  postTwitter("Status : " + getTime());
}

void postTwitter(String msg){
  // Twitter account : garage@barrowkwan.com  "Barrow Garage Aquari"
  
  Twitter twitter("927020131-DEvJRTHsclJFOVEQ85ahXeH4vmYTSLGOEEDi01Vg");
  char tmsg[msg.length()+1];
  msg.toCharArray(tmsg,(msg.length()+1));
  if (twitter.post(tmsg)) {
    int status = twitter.wait(&Serial);
    
    if (status == 200) {
      Serial.println("Tweets : " + getTime());
    } else {
      Serial.println("Twitter failed :" + status);
    }
  } else {
    Serial.println("Twitter connection failed.");
  }
  Alarm.delay(1000);
}

String getTime(){
  char  *Mon[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  char timestring[22];
  snprintf(timestring,sizeof(timestring),"%02d:%02d:%02d %02d %s %4d",hour(),minute(),second(),day(),Mon[month()],year());
  return String(timestring);
}


String getTimeOnly(){
  char timestring[9];
  snprintf(timestring,sizeof(timestring),"%02d:%02d:%02d",hour(),minute(),second());
  return String(timestring);
}


