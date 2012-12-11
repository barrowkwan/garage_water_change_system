/* Web_Demo.pde -- sample code for Webduino server library */

/*
 * To use this demo,  enter one of the following USLs into your browser.
 * Replace "host" with the IP address assigned to the Arduino.
 *
 * http://host/
 * http://host/json
 *
 * This URL brings up a display of the values READ on digital pins 0-9
 * and analog pins 0-5.  This is done with a call to defaultCmd.
 * 
 * 
 * http://host/form
 *
 * This URL also brings up a display of the values READ on digital pins 0-9
 * and analog pins 0-5.  But it's done as a form,  by the "formCmd" function,
 * and the digital pins are shown as radio buttons you can change.
 * When you click the "Submit" button,  it does a POST that sets the
 * digital pins,  re-reads them,  and re-displays the form.
 * 
 */

#include <Wire.h>
#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"

RTC_DS1307 RTC;
LiquidCrystal_I2C lcd(0x27,16,2);

// no-cost stream operator as described at 
// http://sundial.org/arduino/?page_id=119
template<class T>
inline Print &operator <<(Print &obj, T arg)
{ obj.print(arg); return obj; }


// CHANGE THIS TO YOUR OWN UNIQUE VALUE
static uint8_t mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x45, 0x1D };

// CHANGE THIS TO MATCH YOUR HOST NETWORK
static uint8_t ip[] = { 192, 168, 210, 54 };


const int relayPin1 = 4;
const int relayPin2 = 5;
const int relayPin3 = 6;
const int relayPin4 = 7;
const int relayPin5 = 8;
const int floatValveHighPin = 9;
const int floatValveLowPin = 3;


#define PREFIX ""

WebServer webserver(PREFIX, 80);

// commands are functions that get called by the webserver framework
// they can read any posted data from client, and they output to server

void outputPins(WebServer &server, WebServer::ConnectionType type, bool addControls = false)
{
  DateTime now = RTC.now();
  P(htmlHead) =
    "<html>"
    "<head>"
    "<title>Arduino Web Server</title>"
    "<style type=\"text/css\">"
    "BODY { font-family: sans-serif }"
    "H1 { font-size: 14pt; text-decoration: underline }"
    "P  { font-size: 10pt; }"
    "</style>"
    "</head>"
    "<body>";

  int i;
  server.httpSuccess();
  server.printP(htmlHead);

  if (addControls)
    server << "<form action='" PREFIX "/form' method='post'>";

  server << "<h1>Device Status</h1><p>";
  
  // Show Date Time
  server << now.month() << "/" << now.day() << "/" << now.year() << " " << now.hour() << ":" << now.minute() << ":" << now.second() << "<br/><br/>";
  
  int sw1, sw2, devStatus;

  // Water Pump Status:
  devStatus = digitalRead(relayPin5);
  server << "Water Pump is : ";
  if (addControls)
  {
    server.radioButton("p", "1", "On", devStatus);
    server << " ";
    server.radioButton("p", "0", "Off", !devStatus);
  }else{
    server << (devStatus ? "On" : "Off");
  }
  server << "<br/><br/>";
  



  // Aquarium Filter Ball Valve Status:
  sw1 = digitalRead(relayPin1);
  sw2 = digitalRead(relayPin2);
  server << "Aquarium Filter Ball Valve is : ";
  if ((sw1 == HIGH) && (sw2 == LOW)) {
    devStatus = 0;
  }
  else if ((sw1 == LOW) && (sw2 == HIGH)) {\
    devStatus = 1;
  }else{
    devStatus = -1;
  }
  if (addControls)
  {
    if (devStatus == -1)
      server << " <font color=\"red\">(Unknown State)</font>";
    server.radioButton("a", "1", "On", devStatus);
    server << " ";
    server.radioButton("a", "0", "Off", !devStatus);
  }else{
    if (devStatus == -1)
      server << " <font color=red>(Unknown State)</font> ";
    else
      server << (devStatus ? "On" : "Off");
  }  
  server << "<br/><br/>";
  
  // Clean Water Ball Value Status:
  sw1 = digitalRead(relayPin3);
  sw2 = digitalRead(relayPin4);
  server << "Clean Water Ball Valve is : ";
  if ((sw1 == HIGH) && (sw2 == LOW)) {
    devStatus = 0;
  }
  else if ((sw1 == LOW) && (sw2 == HIGH)) {\
    devStatus = 1;
  }else{
    devStatus = -1;
  }
  if (addControls)
  {
    if (devStatus == -1)
      server << " <font color=\"red\">(Unknown State)</font>";
    server.radioButton("c", "1", "On", devStatus);
    server << " ";
    server.radioButton("c", "0", "Off", !devStatus);
  }else{
    if (devStatus == -1)
      server << " <font color=red>(Unknown State)</font> ";
    else
      server << (devStatus ? "On" : "Off");
  }  
  server << "<br/><br/>";  

  if (addControls)
    server << "<input type='submit' value='Submit'/></form>";
    
  server << "<br/><br/>";  
    
  server << "<h1>Digital Pins</h1><p>";

  for (i = 0; i <= 9; ++i)
  {
    // ignore the pins we use to talk to the Ethernet chip
    int val = digitalRead(i);
    server << "Digital " << i << ": ";
    server << (val ? "HIGH" : "LOW");
    server << "<br/>";
  }

  server << "</p><h1>Analog Pins</h1><p>";
  for (i = 0; i <= 5; ++i)
  {
    int val = analogRead(i);
    server << "Analog " << i << ": " << val << "<br/>";
  }
  server << "</p>";



  server << "</body></html>";
}

void formCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  int deviceState;
  if (type == WebServer::POST)
  {
    bool repeat;
    char name[16], value[16];
    do
    {
      repeat = server.readPOSTparam(name, 16, value, 16);
      deviceState = strtoul(value, NULL, 10);
      if (name[0] == 'a')
      {
        if (deviceState == 0){
          digitalWrite(relayPin1, 1);
          digitalWrite(relayPin2, 0);
        }else{
          digitalWrite(relayPin1, 0);
          digitalWrite(relayPin2, 1);
        }
      }else if (name[0] == 'c'){
        if (deviceState == 0){
          digitalWrite(relayPin3, 1);
          digitalWrite(relayPin4, 0);
        }else{
          digitalWrite(relayPin3, 0);
          digitalWrite(relayPin4, 1);
        }
      }else if (name[0] == 'p'){
          digitalWrite(relayPin5,deviceState);
      }else{
        // Do nothing
      }
    } while (repeat);

    server.httpSeeOther(PREFIX "/form");
  }
  else
    outputPins(server, type, true);
}

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  outputPins(server, type, false);  
}

void lcdDisplay()
{
  DateTime now = RTC.now();
  int sec, minu, hr, dayOfMonth, mon, yr;

  sec     = now.second();
  minu     = now.minute();
  hr       = now.hour();
  dayOfMonth = now.day();
  mon      = now.month();
  yr       = now.year();
  
  lcd.setCursor(0,0);
  lcd.print(mon);
  lcd.print("/");
  lcd.print(dayOfMonth);
  lcd.print("/");
  lcd.print(yr);
  lcd.setCursor(0,1);
  if (hr < 10)
    lcd.print("0");
  lcd.print(hr);
  lcd.print(":");
  if (minu < 10)
    lcd.print("0");
  lcd.print(minu);
  lcd.print(":");
  if (sec < 10)
    lcd.print("0");
  lcd.print(sec);
}

void setup()
{

  Wire.begin();
  RTC.begin();
  lcd.init();
  lcd.backlight();
 

  Ethernet.begin(mac, ip);
  webserver.begin();
  lcd.setCursor(0,0);
  lcd.print("System is");
  lcd.setCursor(0,1);
  lcd.print("starting...");
  
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

  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("form", &formCmd);
  
  lcd.clear();
}

void loop()
{
  // process incoming connections one at a time forever
  webserver.processConnection();

  // if you wanted to do other work based on a connecton, it would go here
}
