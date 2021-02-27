// +---------------------------------------------------------------------------+
// | MM7D v0.21 * Air quality measuring device                                 |
// | Copyright (C) 2020-2021 Pozsár Zsolt <pozsar.zsolt@szerafingomba.hu>      |
// | mm7d.ino                                                                  |
// | Program for ESP8266 Huzzah Breakout                                       |
// +---------------------------------------------------------------------------+

//   This program is free software: you can redistribute it and/or modify it
// under the terms of the European Union Public License 1.1 version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.

#include <DHT.h>
#include <ESP8266WebServer.h>
#include <StringSplitter.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define TYP_SENSOR1 DHT11

// settings
const String serialnumber   = "";  // serial number
const char* wifi_ssid       = "";  // SSID of Wi-Fi AP
const char* wifi_password   = "";  // password of Wi-Fi AP
const String uid            = "";  // user ID
const String allowedaddress = "";  // client IP addresses with space delimiter

// GPIO ports
const int prt_buzzer        = 14;
const int prt_led_blue      = 2;
const int prt_led_green     = 0;
const int prt_led_red       = 5;
const int prt_led_yellow    = 4;
const int prt_sensor1       = 12;

// ADC input
const int prt_sensor2       = 0;

// messages
const String msg01          = "MM7D * Air quality measuring device";
const String msg02          = "Copyright (C) 2020-2021";
const String msg03          = "pozsar.zsolt@szerafingomba.hu";
const String msg04          = "http://www.szerafingomba.hu/equipments/";
const String msg05          = "* Initializing GPIO ports...";
const String msg06          = "* Initializing sensors...";
const String msg07          = "* Connecting to wireless network";
const String msg08          = "done.";
const String msg09          = "  my IP address:      ";
const String msg10          = "  subnet mask:        ";
const String msg11          = "  gateway IP address: ";
const String msg12          = "* Starting webserver...";
const String msg13          = "* HTTP request received from: ";
const String msg14          = "* E01: Failed to read CO2 sensor!";
const String msg15          = "* E02: Failed to read T/RH sensor!";
const String msg16          = "MM7D";
const String msg17          = "Authentication error!";
const String msg18          = "* E03: Authentication error!";
const String msg19          = "Not allowed client IP address!";
const String msg20          = "* E04: Not allowed client IP address!";
const String msg21          = "  green";
const String msg22          = "  red";
const String msg23          = "  yellow";
const String msg24          = " LED is switched ";
const String msg25          = "on.";
const String msg26          = "off.";
const String msg27          = "Done.";
const String msg28          = "Pozsar Zsolt";
const String msg29          = "  device MAC address: ";
const String msg30          = "  Page not found!";
const String msg31          = "Serial number of hardware: ";
const String msg32          = "  get homepage";
const String msg33          = "  get version data";
const String msg34          = "  get all measured data";
const String msg35          = "  get relative unwanted gas level";
const String msg36          = "  get relative humidity";
const String msg37          = "  get temperature";
const String msg38          = "  get all measured data and set limit values";
const String msg39          = "    gas level:\t\t";
const String msg40          = "    humidity:\t\t";
const String msg41          = "    temperature:\t";
const String msg42          = "* Periodic measure.";
const String msg43          = "  limit values:";
const String msg44          = "  Not enough argument!";
const String msg45          = "  measured data:";
const String msg46          = "* E05: Page not found!";

// general constants
const int maxadcvalue       = 1024;
const long interval1        = 2000;
const long interval3        = 60000;
const String swversion      = "0.21";

// variables
float humidity, temperature, unwantedgaslevel;
int adcvalue                = 0;
String clientaddress;
String devicemacaddress;
String line;
String localipaddress;
unsigned long prevtime1     = 0;
unsigned long prevtime2     = 0;
unsigned long prevtime3     = 0;
int h1, h2, h3, h4;
int t1, t2, t3, t4;
int g;
int green, yellow, red;

DHT dht(prt_sensor1, TYP_SENSOR1, 11);
ESP8266WebServer server(80);

// initializing function
void setup(void)
{
  // set serial port
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  Serial.println(msg01 + " * v" + swversion );
  Serial.println(msg02 +  " " + msg28 + " <" + msg03 + ">");
  Serial.println(msg31 + serialnumber );
  // initialize ports
  Serial.print(msg05);
  pinMode(prt_buzzer, OUTPUT);
  pinMode(prt_led_blue, OUTPUT);
  pinMode(prt_led_green, OUTPUT);
  pinMode(prt_led_red, OUTPUT);
  pinMode(prt_led_yellow, OUTPUT);
  digitalWrite(prt_led_blue, LOW);
  digitalWrite(prt_led_green, LOW);
  digitalWrite(prt_led_red, LOW);
  digitalWrite(prt_led_yellow, LOW);
  Serial.println(msg08);
  // initialize sensors
  Serial.print(msg06);
  dht.begin();
  Serial.println(msg08);
  // connect to wireless network
  Serial.print(msg07);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }
  Serial.println(msg08);
  localipaddress = WiFi.localIP().toString();
  devicemacaddress = WiFi.macAddress();
  Serial.println(msg29 + devicemacaddress);
  Serial.println(msg09 + localipaddress);
  Serial.println(msg10 + WiFi.subnetMask().toString());
  Serial.println(msg11 + WiFi.gatewayIP().toString());
  // start webserver
  Serial.print(msg12);
  server.onNotFound(handleNotFound);
  // write help page as html text
  server.on("/", []()
  {
    writeclientipaddress();
    Serial.println(msg32);
    line = "<html>\n"
           "  <head>\n"
           "    <title>" + msg01 + "</title>\n"
           "  </head>\n"
           "  <body bgcolor=\"#e2f4fd\">\n"
           "    <h2>" + msg01 + "</h2>\n"
           "    <br>\n"
           "    Hardware serial number: " + serialnumber + "<br>\n"
           "    Software version: v" + swversion + "<br>\n"
           "    <hr>\n"
           "    <h3>Plain text data and control pages:</h3>\n"
           "    <br>\n"
           "    <table border=\"0\" cellpadding=\"5\">\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/version</td>\n"
           "        <td>Get software name and version</td>\n"
           "      </tr>\n"
           "      <tr><td colspan=\"2\">&nbsp;</td></tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/operation?uid=abcdef&g=20&h1=65&h2=70&h3=80&h4=85&t1=13&t2=150&t3=20&t4=22</td>\n"
           "        <td>Get all data and set limit values to switch on/off LEDs</td>\n"
           "      </tr>\n"
           "      <tr><td colspan=\"2\">&nbsp;</td></tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/get/all?uid=abcdef</a></td>\n"
           "        <td>Get all data</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/get/humidity?uid=abcdef</a></td>\n"
           "        <td>Get relative humidity in %</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/get/temperature?uid=abcdef</a></td>\n"
           "        <td>Get temperature in &deg;C</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/get/unwantedgaslevel?uid=abcdef</a></td>\n"
           "        <td>Get relative level of unwanted gases in %</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td colspan=\"2\">&nbsp;</td></tr>"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/all/off?uid=abcdef</a></td>\n"
           "        <td>Switch off all LEDs</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/greenled/off?uid=abcdef</a></td>\n"
           "        <td>Switch off green LED</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/greenled/on?uid=abcdef</a></td>\n"
           "        <td>Switch on green LED</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/yellowled/off?uid=abcdef</a></td>\n"
           "        <td>Switch off yellow LED</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/yellowled/on?uid=abcdef</a></td>\n"
           "        <td>Switch on yellow LED</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/redled/off?uid=abcdef</a></td>\n"
           "        <td>Switch off red LED</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/redled/on?uid=abcdef</td>\n"
           "        <td>Switch on red LED</td>\n"
           "      </tr>\n"
           "    </table>\n"
           "    <br>\n"
           "    <hr>\n"
           "    <center>" + msg02 + " <a href=\"mailto:" + msg03 + "\">" + msg28 + "</a> - <a href=\"" + msg04 + "\">Homepage</a><center>\n"
           "    <br>\n"
           "  <body>\n"
           "</html>\n";
    server.send(200, "text/html", line);
    delay(100);
  });
  // write software version as plain text
  server.on("/version", []()
  {
    writeclientipaddress();
    Serial.println(msg33);
    line = msg16 + "\n" + swversion + "\n" + serialnumber;
    server.send(200, "text/plain", line);
  });
  // write all measured parameters as plain text and set limit values
  server.on("/operation", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg38);
        if (setlimitvalues() == 0) leds();
        line = String((int)unwantedgaslevel) + "\n" + String((int)humidity) + "\n" + String((int)temperature);
        server.send(200, "text/plain", line);
      }
  });
  // write all measured parameters as plain text
  server.on("/get/all", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg34);
        line = String((int)unwantedgaslevel) + "\n" + String((int)humidity) + "\n" + String((int)temperature);
        server.send(200, "text/plain", line);
      }
  });
  // write relative unwanted gas level as plain text
  server.on("/get/unwantedgaslevel", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg35);
        line = String((int)unwantedgaslevel);
        server.send(200, "text/plain", line);
      }
  });
  // write relative humidity as plain text
  server.on("/get/humidity", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg36);
        line = String((int)humidity);
        server.send(200, "text/plain", line);
      }
  });
  // write temperature in degree Celsius as plain text
  server.on("/get/temperature", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg37);
        line = String((int)temperature);
        server.send(200, "text/plain", line);
      }
  });
  // switch off all LEDs
  server.on("/set/all/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        green = 0;
        red = 0;
        yellow = 0;
        server.send(200, "text/plain", msg27);
      }
  });
  // switch off green LED
  server.on("/set/greenled/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        green = 0;
        server.send(200, "text/plain", msg27);
      }
  });
  // switch on green LED
  server.on("/set/greenled/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        green = 1;
        server.send(200, "text/plain", msg27);
      }
  });
  // switch off red LED
  server.on("/set/redled/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        red = 0;
        server.send(200, "text/plain", msg27);
      }
  });
  // switch on red LED
  server.on("/set/redled/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        red = 1;
        server.send(200, "text/plain", msg27);
      }
  });
  // switch off yellow LED
  server.on("/set/yellowled/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        yellow = 0;
        server.send(200, "text/plain", msg27);
      }
  });
  // switch on yellow LED
  server.on("/set/yellowled/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        yellow = 1;
        server.send(200, "text/plain", msg27);
      }
  });
  server.begin();
  Serial.println(msg08);
  beep(1);
}

// error 404
void handleNotFound()
{
  writeclientipaddress();
  Serial.println(msg46);
  server.send(404, "text/plain", msg30);
}

// loop function
void loop(void)
{
  server.handleClient();
  unsigned long currtime3 = millis();
  if (currtime3 - prevtime3 >= interval3)
  {
    prevtime3 = currtime3;
    Serial.println(msg42);
    Serial.println(msg45);
    getunwantedgaslevel();
    gettemphum();
    leds();
    greenled(green);
    yellowled(yellow);
    redled(red);
  }
}

// switch on/off green LED
void greenled(int i)
{
  if (i == 0)
  {
    digitalWrite(prt_led_green, LOW);
    Serial.println(msg21 + msg24 + msg26);
  } else
  {
    digitalWrite(prt_led_green, HIGH);
    Serial.println(msg21 + msg24 + msg25);
  }
}

// switch on/off red LED
void redled(int i)
{
  if (i == 0)
  {
    digitalWrite(prt_led_red, LOW);
    Serial.println(msg22 + msg24 + msg26);
  } else
  {
    digitalWrite(prt_led_red, HIGH);
    Serial.println(msg22 + msg24 + msg25);
  }
}

// switch on/off yellow LED
void yellowled(int i)
{
  if (i == 0)
  {
    digitalWrite(prt_led_yellow, LOW);
    Serial.println(msg23 + msg24 + msg26);
  } else
  {
    digitalWrite(prt_led_yellow, HIGH);
    Serial.println(msg23 + msg24 + msg25);
  }
}

// set limit values
int setlimitvalues()
{
  if (server.args() > 9)
  {
    String arg;
    arg = server.arg("g");
    if (arg.length() != 0) g = arg.toInt(); else return 1;
    arg = server.arg("h1");
    if (arg.length() != 0) h1 = arg.toInt(); else return 1;
    arg = server.arg("h2");
    if (arg.length() != 0) h2 = arg.toInt(); else return 1;
    arg = server.arg("h3");
    if (arg.length() != 0) h3 = arg.toInt(); else return 1;
    arg = server.arg("h4");
    if (arg.length() != 0) h4 = arg.toInt(); else return 1;
    arg = server.arg("t1");
    if (arg.length() != 0) t1 = arg.toInt(); else return 1;
    arg = server.arg("t2");
    if (arg.length() != 0) t2 = arg.toInt(); else return 1;
    arg = server.arg("t3");
    if (arg.length() != 0) t3 = arg.toInt(); else return 1;
    arg = server.arg("t4");
    if (arg.length() != 0) t4 = arg.toInt(); else return 1;
    Serial.println(msg43);
    Serial.println(msg39 + String(g) + "%");
    Serial.println(msg40 + String(h1) + "%; " + String(h2) + "%; " + String(h3) + "%; " + String(h4) + "%" );
    Serial.println(msg41 + String(t1) + "°C; " + String(t2) + "°C; " + String(t3) + "°C; " + String(t4) + "°C" );
    return 0;
  } else
  {
    Serial.println(msg44);
    return 1;
  }
}

// switch on/off all LED
void leds()
{
  int green = 0;
  int yellow = 0;
  int red = 0;
  if ((unwantedgaslevel != 999) && (humidity != 999) && (temperature != 999))
  {
    if ((int)unwantedgaslevel < g) green = 1;
    if ((int)unwantedgaslevel == g) yellow = 1;
    if ((int)unwantedgaslevel > g) red = 1;
    if ((int)humidity < h1) red = 1;
    if (((int)humidity > h1) && ((int)humidity < h2)) yellow = 1;
    if (((int)humidity > h2) && ((int)humidity < h3)) green = 1;
    if (((int)humidity > h3) && ((int)humidity < h4)) yellow = 1;
    if ((int)humidity > h4) red = 1;
    if ((int)temperature < t1) red = 1;
    if (((int)temperature > t1) && ((int)temperature < t2)) yellow = 1;
    if (((int)temperature > t2) && ((int)temperature < t3)) green = 1;
    if (((int)temperature > t3) && ((int)temperature < t4)) yellow = 1;
    if ((int)temperature > t4) red = 1;
    if (red == 1)
    {
      green = 0;
      yellow = 0;
    }
  }
}

// get air quality
void getunwantedgaslevel()
{
  unsigned long currtime1 = millis();
  if (currtime1 - prevtime1 >= interval1)
  {
    prevtime1 = currtime1;
    adcvalue = analogRead(prt_sensor2);
    unwantedgaslevel = adcvalue / (maxadcvalue / 100);
    if (unwantedgaslevel > 100)
    {
      beep(1);
      Serial.println(msg14);
      unwantedgaslevel = 999;
      return;
    } else Serial.println(msg39 + String((int)unwantedgaslevel) + "%");
  }
}

// get temperature and relative humidity
void gettemphum()
{
  unsigned long currtime2 = millis();
  if (currtime2 - prevtime2 >= interval1)
  {
    prevtime2 = currtime2;
    humidity = dht.readHumidity();
    temperature = dht.readTemperature(false);
    if (isnan(humidity) || isnan(temperature))
    {
      beep(1);
      Serial.println(msg15);
      temperature = 999;
      humidity = 999;
      return;
    } else
    {
      Serial.println(msg40 + String((int)humidity) + "%");
      Serial.println(msg41 + String((int)temperature) + " °C");
    }
  }
}

// blink blue LED and write client IP address to serial console
void writeclientipaddress()
{
  digitalWrite(prt_led_blue, HIGH);
  delay(500);
  digitalWrite(prt_led_blue, LOW);
  clientaddress = server.client().remoteIP().toString();
  Serial.println(msg13 + clientaddress + ".");
}

// check IP address of client
int checkipaddress()
{
  int allowed = 0;
  writeclientipaddress();
  StringSplitter *splitter = new StringSplitter(allowedaddress, ' ', 3);
  int itemCount = splitter->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter->getItemAtIndex(i);
    if (clientaddress == String(item)) allowed = 1;
  }
  if (allowed == 1) return 1; else
  {
    server.send(401, "text/plain", msg19);
    Serial.println(msg20);
    beep(3);
    return 0;
  }
}

// authentication
int checkuid()
{
  if (server.arg("uid") == uid) return 1; else
  {
    server.send(401, "text/plain", msg17);
    Serial.println(msg18);
    beep(2);
    return 0;
  }
}

// beep sign
void beep(int num)
{
  for (int i = 0; i < num; i++)
  {
    tone(prt_buzzer, 880);
    delay (100);
    noTone(prt_buzzer);
    delay (100);
  }
}
