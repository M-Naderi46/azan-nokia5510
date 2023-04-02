#include "Wire.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DateConvL.h"
#include <stdio.h>
#include "DFRobotDFPlayerMini.h"
SoftwareSerial mySoftwareSerial(10, 11);  // RX, TX
DFRobotDFPlayerMini myDFPlayer;
#include "RTClib.h"
RTC_DS3231 rtc;
#define BUSY_PIN 9
int key = 8;

DateConvL dateC;
//rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));//auto update from computer time
//rtc.setDateDs1307(DateTime(2022, 10, 19, 18, 40, 20));// to set the time manualy

Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

int xegg, yegg;
int gYear, pYear = 0;
char gMonth, gDay, pMonth, pDay, Weekday = 0;

#define DS1307_I2C_ADDRESS 0x68  // This is the I2C address
long previousMillis = 0;         // will store last time time was updated

long interval = 200;
int displayx, displayy, displayradius, x2, y2, x3, y3;
int zero = 0;
char *Day[] = { "yek", "Dos", "Ses", "Cha", "Pan", "Jom", "Sha" };

struct TimeOfDay {
  int hour, minute;
};

struct Oghat {
  TimeOfDay fajr, sunRise, zuhr, asr, maghrib, isha;
};

struct Config {
  double longitude, latitude;
  float timeZone;
  double fajrTwilight, ishaTwilight;
};

Config config = Config{ 51.4, 35.57, 3.43, -14.5, -14 };

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val) {
  return ((val / 10 * 16) + (val % 10));
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val) {
  return ((val / 16 * 10) + (val % 16));
}

void displayShamsiDate(DateConvL &date, Print &out) {
  out.print(date.global_year);
  out.print('/');
  out.print(date.global_month);
  out.print('/');
  out.print(date.global_day);
  out.println();
}

Oghat calcOghat(DateTime &time) {
  double fajr, sunRise, zuhr, asr, maghrib, isha;
  Oghat today;
  int hours, minutes;
  calcPrayerTimes(time, config, fajr, sunRise, zuhr, asr, maghrib, isha);
  doubleToHrMin(fajr, hours, minutes);
  today.fajr = TimeOfDay{ hours, minutes };

  doubleToHrMin(sunRise, hours, minutes);
  today.sunRise = TimeOfDay{ hours, minutes };

  doubleToHrMin(zuhr, hours, minutes);
  today.zuhr = TimeOfDay{ hours, minutes };

  doubleToHrMin(asr, hours, minutes);
  today.asr = TimeOfDay{ hours, minutes };

  doubleToHrMin(maghrib, hours, minutes);
  today.maghrib = TimeOfDay{ hours, minutes };

  doubleToHrMin(isha, hours, minutes);
  today.isha = TimeOfDay{ hours, minutes };

  return today;
}

void displayDate(DateTime &date, Print &out) {
  out.print(date.day());
  out.print("/");
  out.print(date.month());
  out.print("/");
  out.print(date.year());
  out.print(" ");
  out.print(Day[date.dayOfTheWeek()]);
}

void displayTime(DateTime &time, Print &out) {
  out.print(time.hour());
  out.print(":");
  out.print(time.minute());
  out.print(":");
  out.print(time.second());
}

void displayOghatTime(TimeOfDay time, Print &out) {
  out.print(time.hour);
  out.print(":");
  out.print(time.minute);
}

String formatOghat(const String &title, TimeOfDay &time) {
  return title + " " + time.hour + ":" + time.minute;
}

void oghatPage(Oghat &day, Print &out) {
  out.println(formatOghat("Fajr", day.fajr));
  out.println(formatOghat("Zuhr", day.zuhr));
  out.println(formatOghat("Asr", day.asr));
  out.println(formatOghat("Maghrib", day.maghrib));
  out.println(formatOghat("Isha", day.isha));
}

void dateTimePage(DateConvL &dateC, DateTime &time) {
  display.setCursor(0, 8);
  displayShamsiDate(dateC, display);
  displayShamsiDate(dateC, Serial);

  display.setCursor(1, 16);
  displayDate(time, display);
  displayDate(time, Serial);
  Serial.println();

  display.setCursor(18, 26);
  display.setTextSize(1, 2);
  displayTime(time, display);
  displayTime(time, Serial);
  display.setTextSize(1);
  Serial.println();
  display.display();
}

bool operator==(DateTime &time, TimeOfDay &tod) {
  return time.hour() == tod.hour && time.minute() == tod.minute;
}
uint16_t operator-(DateTime &time, TimeOfDay &tod) {
  return time.hour() * 3600 + time.minute() * 60 + time.second() - tod.hour * 3600 - tod.minute * 60;
}
bool operator<(DateTime &time, TimeOfDay &tod) {
  return (time.hour() < tod.hour) || (time.hour() == tod.hour && time.minute() < tod.minute);
}
bool operator>(DateTime &time, TimeOfDay &tod) {
  return (time.hour() > tod.hour) || (time.hour() == tod.hour && time.minute() > tod.minute);
}
bool operator<=(DateTime &time, TimeOfDay &tod) {
  return time < tod || time == tod;
}
bool operator>=(DateTime &time, TimeOfDay &tod) {
  return time > tod || time == tod;
}

String nextOghatTime(DateTime &time, Oghat &oghat) {
  if (time < oghat.fajr) return formatOghat("Fajr", oghat.fajr);
  if (time < oghat.sunRise) return formatOghat("SunRise", oghat.sunRise);
  if (time < oghat.zuhr) return formatOghat("Zuhr", oghat.zuhr);
  if (time < oghat.maghrib) return formatOghat("Maghrib", oghat.maghrib);
  return "";
}

bool checkAzan(DateTime &time, Oghat &oghat) {
  return (time == oghat.fajr || time == oghat.zuhr || time == oghat.maghrib) && digitalRead(BUSY_PIN);
}

void dateTimePage2(DateConvL &dateC, DateTime &time, Oghat &oghat) {
  display.setCursor(0, 4);
  displayShamsiDate(dateC, display);
  displayShamsiDate(dateC, Serial);

  display.setCursor(0, 12);
  displayDate(time, display);
  displayDate(time, Serial);
  Serial.println();

  display.setCursor(18, 22);
  display.setTextSize(1, 2);
  displayTime(time, display);
  displayTime(time, Serial);
  display.setTextSize(1);
  Serial.println();
  display.setCursor(0, 22 + 16);
  display.println(nextOghatTime(time, oghat));
  display.display();
}



void setup() {
  mySoftwareSerial.begin(9600);
  Serial.begin(9600);
  Wire.begin();
  pinMode(BUSY_PIN, INPUT);
  pinMode(key,INPUT_PULLUP);;
  if (!rtc.begin()) {
    Serial.print("Couldn't find RTC");
    //while (1)
      ;
  }

  if (!myDFPlayer.begin(mySoftwareSerial))  //{  //Use softwareSerial to communicate with mp3.
    // Serial.println(F("Unable to begin:"));
    // Serial.println(F("1.Please recheck the connection!"));
    //  Serial.println(F("2.Please insert the SD card!"));
    //  while(true);
    // if (! rtc.isrunning())
    // {
    //   Serial.print("RTC is NOT running!");
    // }
    // Serial.print("RTC is NOT running!");
    myDFPlayer.setTimeOut(500);  //Set serial communictaion time out 500ms

  //----Set volume----
  myDFPlayer.volume(30);  //Set volume value (0~30).
  //myDFPlayer.volumeUp(); //Volume Up
  //myDFPlayer.volumeDown(); //Volume Down

  //----Set different EQ----
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);

  // myDFPlayer.play(1);  //Play the first mp3

  display.begin();
  display.clearDisplay();
  display.setContrast(25);
  xegg = (display.width()) / 2;
  yegg = (display.height()) / 2;
  display.setTextColor(BLACK);
  display.setTextSize(1,2);
  display.setCursor(18, 10);
  display.print("Mahmood");
  display.display();
  delay(1000);
  display.setCursor(18, 30);
  display.print("Naderi");
  display.setTextSize(1);
  display.display();
  delay(1500);
}

void loop() {
  DateTime now;
  Oghat oghat;
  now = rtc.now();
  oghat = calcOghat(now);
  dateC.ToShamsi(now.year(), now.month(), now.day());
  display.clearDisplay();
  dateTimePage2(dateC, now, oghat);
  oghatPage(oghat, Serial);
  if(checkAzan(now, oghat)) myDFPlayer.play(1);
  delay(800);
  if(digitalRead(key)== LOW) {
  display.clearDisplay();
  oghatPage(oghat, display);
  
  display.display();
  delay(5000);
  }
}

/*-------------------------------------------------------------------------------------*/
// PRAYER TIME (by Mahmoud Adly Ezzat, Cairo)

//convert Degree to Radian
double degToRad(double degree) {
  return ((3.1415926 / 180) * degree);
}

//convert Radian to Degree
double radToDeg(double radian) {
  return (radian * (180 / 3.1415926));
}

//make sure a value is between 0 and 360
double moreLess360(double value) {
  while (value > 360 || value < 0) {
    if (value > 360)
      value -= 360;

    else if (value < 0)
      value += 360;
  }

  return value;
}

//make sure a value is between 0 and 24
double moreLess24(double value) {
  while (value > 24 || value < 0) {
    if (value > 24)
      value -= 24;

    else if (value < 0)
      value += 24;
  }

  return value;
}

//convert the double number to Hours and Minutes
void doubleToHrMin(double number, int &hours, int &minutes) {
  hours = floor(moreLess24(number));
  minutes = floor(moreLess24(number - hours) * 60);
}

void calcPrayerTimes(DateTime &time, Config &config,
                     double &fajrTime, double &sunRiseTime, double &zuhrTime,
                     double &asrTime, double &maghribTime, double &ishaTime) {
  uint16_t year = time.year() - 2000;
  uint16_t month = time.month();
  uint16_t day = time.day();
  double longitude = config.longitude, latitude = config.latitude;
  float timeZone = config.timeZone;
  double fajrTwilight = config.fajrTwilight, ishaTwilight = config.ishaTwilight;

  double D = (367 * year) - ((year + (int)((month + 9) / 12)) * 7 / 4) + (((int)(275 * month / 9)) + day - 730531.5);

  double L = 280.461 + 0.9856474 * D;
  L = moreLess360(L);

  double M = 357.528 + (0.9856003) * D;
  M = moreLess360(M);

  double Lambda = L + 1.915 * sin(degToRad(M)) + 0.02 * sin(degToRad(2 * M));
  Lambda = moreLess360(Lambda);

  double Obliquity = 23.439 - 0.0000004 * D;
  double Alpha = radToDeg(atan((cos(degToRad(Obliquity)) * tan(degToRad(Lambda)))));
  Alpha = moreLess360(Alpha);

  Alpha = Alpha - (360 * (int)(Alpha / 360));
  Alpha = Alpha + 90 * (floor(Lambda / 90) - floor(Alpha / 90));

  double ST = 100.46 + 0.985647352 * D;
  double Dec = radToDeg(asin(sin(degToRad(Obliquity)) * sin(degToRad(Lambda))));
  double Durinal_Arc = radToDeg(acos((sin(degToRad(-0.8333)) - sin(degToRad(Dec)) * sin(degToRad(latitude))) / (cos(degToRad(Dec)) * cos(degToRad(latitude)))));

  double Noon = Alpha - ST;
  Noon = moreLess360(Noon);


  double UT_Noon = Noon - longitude;


  ////////////////////////////////////////////
  // Calculating Prayer Times Arcs & Times //
  //////////////////////////////////////////

  // 2) Zuhr Time [Local noon]
  zuhrTime = UT_Noon / 15 + timeZone;

  // Asr Hanafi
  //double Asr_Alt =radToDeg(atan(2+tan(degToRad(latitude - Dec))));
  double Asr_Alt = radToDeg(atan(1.7 + tan(degToRad(latitude - Dec))));
  // Asr Shafii
  //double Asr_Alt = radToDeg(atan(1 + tan(degToRad(latitude - Dec))));
  double Asr_Arc = radToDeg(acos((sin(degToRad(90 - Asr_Alt)) - sin(degToRad(Dec)) * sin(degToRad(latitude))) / (cos(degToRad(Dec)) * cos(degToRad(latitude)))));
  Asr_Arc = Asr_Arc / 15;
  // 3) Asr Time
  asrTime = zuhrTime + Asr_Arc;

  // 1) Shorouq Time
  sunRiseTime = zuhrTime - (Durinal_Arc / 15);

  // 4) Maghrib Time
  maghribTime = zuhrTime + (Durinal_Arc / 15);

  double Esha_Arc = radToDeg(acos((sin(degToRad(ishaTwilight)) - sin(degToRad(Dec)) * sin(degToRad(latitude))) / (cos(degToRad(Dec)) * cos(degToRad(latitude)))));
  // 5) Isha Time
  ishaTime = zuhrTime + (Esha_Arc / 15);

  // 0) Fajr Time
  double Fajr_Arc = radToDeg(acos((sin(degToRad(fajrTwilight)) - sin(degToRad(Dec)) * sin(degToRad(latitude))) / (cos(degToRad(Dec)) * cos(degToRad(latitude)))));
  fajrTime = zuhrTime - (Fajr_Arc / 15);

  return;
}