#include <stdlib.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Twitter.h>

#define DS1307_I2C_ADDRESS 0x68  // This is the I2C address
// Arduino version compatibility Pre-Compiler Directives
#if defined(ARDUINO) && ARDUINO >= 100   // Arduino v1.0 and newer
  #define I2C_WRITE Wire.write 
  #define I2C_READ Wire.read
#else                                   // Arduino Prior to v1.0 
  #define I2C_WRITE Wire.send 
  #define I2C_READ Wire.receive
#endif

#define DHTPIN 2 
#define DHTTYPE DHT22

LiquidCrystal_I2C lcd(0x27,16,2);
RTC_DS1307 RTC;
DHT dht(DHTPIN, DHTTYPE);
EthernetUDP Udp;
const int lcdStatusPin = 8;
const int relayPin1 = 4;
const int relayPin2 = 5;
const int relayPin3 = 6;
const int relayPin4 = 7;
const int floatValveHighPin = 9;

int lcdStatusPinState;
int lcdStatusMenu = 0;
int floatValveHighState;


void setup()
{
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x45, 0x1D };
//  byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x44, 0xA8 };
  byte ip[] = { 192, 168, 210, 52 };
  byte twitter_server[] = { 128, 121, 146, 100 };

  Serial.begin(9600);
  //Wire.begin();
  pinMode(lcdStatusPin, INPUT);
  pinMode(floatValveHighPin, INPUT);
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(relayPin3, OUTPUT);
  pinMode(relayPin4, OUTPUT);
  digitalWrite(relayPin1, 1);
  digitalWrite(relayPin2, 1);
  digitalWrite(relayPin3, 1);
  digitalWrite(relayPin4, 1);
  Ethernet.begin(mac, ip);
  Udp.begin(8888);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  
  updateTimer();
  // create the alarms
  Alarm.timerRepeat(21600, updateTimer);
  Alarm.alarmRepeat(8,30,0, MorningAlarm);  // 8:30am every day
  Alarm.alarmRepeat(22,46,0,EveningAlarm);  // 5:45pm every day 
  Alarm.alarmRepeat(dowSaturday,8,30,30,WeeklyAlarm);  // 8:30:30 every Saturday 

 
  Alarm.timerRepeat(15, Repeats);            // timer for every 15 seconds    
  Alarm.timerOnce(10, OnceOnly);             // called once after 10 seconds
  floatValveHighState = bitRead(PORTD,floatValveHighPin);
}

void  loop(){
  int currentfloatValveHighState;
  lcdStatusPinState = digitalRead(lcdStatusPin);
  if (lcdStatusPinState == LOW){
    //Serial.println("LOW.....");                                                                                                                                                
    ++lcdStatusMenu;
    if (lcdStatusMenu >= 5)
      lcdStatusMenu = 0;
  }
  lcdDisplay();
  digitalClockDisplay();
  currentfloatValveHighState = digitalRead(floatValveHighPin);
  if (floatValveHighState != currentfloatValveHighState ){
    Serial.println("Float Valve High State : " + String(currentfloatValveHighState));
    floatValveHighState = currentfloatValveHighState;
  }
  Alarm.delay(1000); 
}

void lcdDisplay(){
  IPAddress my_ip;
  String currentTime;
  lcd.clear();
  switch (lcdStatusMenu){
    case 1:
      lcd.display();
      currentTime = getCurrentTime();
      lcd.setCursor(0,0);
      lcd.print(currentTime.substring(0,10));
      lcd.setCursor(0,1);
      lcd.print(currentTime.substring(11,19));
      break;
    case 2:
      lcd.display();
      my_ip = Ethernet.localIP();
      lcd.setCursor(0,0);
      lcd.print("IP Address :    ");
      lcd.setCursor(0,1);
      lcd.print(String(my_ip[0],DEC) + "." + String(my_ip[1],DEC) + "." + String(my_ip[2],DEC) + "." + String(my_ip[3],DEC));
      break;
    case 3:
      lcd.display();
      lcd.setCursor(0,0);
      lcd.print("Air Temperature :");
      lcd.setCursor(0,1);
      lcd.print(getAirTemperature());
      break;
    case 4:
      lcd.display();
      lcd.setCursor(0,0);
      lcd.print("Humidity :");
      lcd.setCursor(0,1);
      lcd.print(getHumidity());
      break;
    default:
      lcd.noDisplay();
  }
}

String getCurrentTime(){
  return String(month()) + "/" + String(day()) + "/" + String(year()) + " " + String(hour()) + ":" + returnDigits(minute()) + ":" + returnDigits(second());
}

void updateTimer(){
  Serial.println("********************** Update Timer *********************");
  time_t currentTime = getNTPTime();
  if (currentTime == -1){
    Serial.println("Cannot get time from NTP server");
    setTime(0);
  postTwitter("GWCS - Cannot get time from NTP server");
  }else{
    setTime(getNTPTime());
    postTwitter("GWCS - Update Timer " + getCurrentTime());
  }
}

String getAirTemperature(){
  char temp[16];
  float temperature  = dht.readTemperature(true);
  if (isnan(temperature)) {
    return "-1";
  } else {
    dtostrf(temperature,5,2,temp);
    return temp;
  }
}

String getHumidity(){
  float humid = dht.readHumidity();
  char humidstr[16];
  if (isnan(humid)) {
    return "-1";
  } else {
    dtostrf(humid,5,2,humidstr);
    return humidstr;
  }
}

time_t getNTPTime()
{
  const int NTP_PACKET_SIZE= 48;
  byte packetBuffer[ NTP_PACKET_SIZE];
  IPAddress timeServer(192, 43, 244, 18);
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  Udp.beginPacket(timeServer, 123);
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
  Alarm.delay(2000);
  if (Udp.parsePacket()){
    Udp.read(packetBuffer,NTP_PACKET_SIZE);
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // unixtime since 1970/1/1 - timezone(PST-8) - 70years + 2s(from alarm.delay above)
    return ((highWord << 16 | lowWord) + (60*60*-8) - 2208988800 + 2);
  }else{
    return -1;
  }
}

void postTwitter(String msg){
  // Twitter account : garage@barrowkwan.com  "Barrow Garage Aquari"
  Twitter twitter("927020131-DEvJRTHsclJFOVEQ85ahXeH4vmYTSLGOEEDi01Vg");
  char tmsg[msg.length()+1];
  msg.toCharArray(tmsg,msg.length()+1);
  if (twitter.post(tmsg)) {
    int status = twitter.wait(&Serial);
    if (status == 200) {
      Serial.println("Tweets @ " + getCurrentTime());
    } else {
      Serial.println("Twitter failed : " + getCurrentTime());
    }
  } else {
    Serial.println("Twitter connection failed.");
  }
  Alarm.delay(1000);
}


// functions to be called when an alarm triggers:
void MorningAlarm(){
  Serial.println("Alarm: - turn lights off");    
}

void EveningAlarm(){
  Serial.println("Alarm: - turn lights on");           
}

void WeeklyAlarm(){
  Serial.println("Alarm: - its Monday Morning");      
}

void ExplicitAlarm(){
  Serial.println("Alarm: - this triggers only at the given date and time");       
}

void Repeats(){
  int testPin = relayPin1;
  Serial.println("15 second timer");
  //digitalWrite(testPin,!bitRead(PORTD,testPin));
}

void OnceOnly(){
  Serial.println("This timer only triggers once");  
}

void digitalClockDisplay()
{
  // digital clock display of the time
  String currentTime = getCurrentTime();
  Serial.println(currentTime);
/*  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(currentTime.substring(0,10));
  lcd.setCursor(0,1);
  lcd.print(currentTime.substring(11,19));*/
}

String returnDigits(int digits)
{
  if(digits < 10)
    return "0" + String(digits);
  return String(digits);
}

void printDigits(int digits)
{
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


