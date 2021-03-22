// +---------------------------------------------------------------------------+
// | MM7D v0.3 * Air quality measuring device                                  |
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

#define       TYP_SENSOR1 DHT11

// settings
const String  serialnumber      = "";  // serial number
const char   *wifi_ssid         = "";  // SSID of Wi-Fi AP
const char   *wifi_password     = "";  // password of Wi-Fi AP
const String  uid               = "";  // user ID
const String  allowedaddress    = "";  // client IP addresses with space delimiter
// GPIO ports
const int     prt_buzzer        = 14;
const int     prt_led_blue      = 2;
const int     prt_led_green     = 0;
const int     prt_led_red       = 5;
const int     prt_led_yellow    = 4;
const int     prt_sensor1       = 12;
// ADC input
const int     prt_sensor2       = 0;
// general constants
const int     maxadcvalue       = 1024;
const long    interval1         = 2000;
const long    interval3         = 60000;
const String  texthtml          = "text/html";
const String  textplain         = "text/plain";
const String  swversion         = "0.3";
// general variables
bool          autoopmode        = false;
float         humidity;
float         temperature;
float         unwantedgaslevel;
int           adcvalue          = 0;
int           g;
int           green             = 0;
int           h1, h2, h3, h4;
int           syslog[64]       = {};
int           red               = 0;
int           t1, t2, t3, t4;
int           yellow            = 0;
String        clientaddress;
String        deviceipaddress;
String        devicemacaddress;
String        line;
unsigned long prevtime1         = 0;
unsigned long prevtime2         = 0;
unsigned long prevtime3         = 0;
// messages
String msg[60]                  =
{
  /*  0 */  "",
  /*  1 */  "MM7D * Air quality measuring device",
  /*  2 */  "Copyright (C) 2020-2021",
  /*  3 */  "pozsar.zsolt@szerafingomba.hu",
  /*  4 */  "http://www.szerafingomba.hu/equipments/",
  /*  5 */  "* Initializing GPIO ports...",
  /*  6 */  "* Initializing sensors...",
  /*  7 */  "* Connecting to wireless network",
  /*  8 */  "done.",
  /*  9 */  "  my IP address:      ",
  /* 10 */  "  subnet mask:        ",
  /* 11 */  "  gateway IP address: ",
  /* 12 */  "* Starting webserver...",
  /* 13 */  "* HTTP request received from: ",
  /* 14 */  "* E01: Failed to read CO2 sensor!",
  /* 15 */  "* E02: Failed to read T/RH sensor!",
  /* 16 */  "MM7D",
  /* 17 */  "Authentication error!",
  /* 18 */  "* E03: Authentication error!",
  /* 19 */  "Not allowed client IP address!",
  /* 20 */  "* E04: Not allowed client IP address!",
  /* 21 */  "  green",
  /* 22 */  "  red",
  /* 23 */  "  yellow",
  /* 24 */  " LED is switched ",
  /* 25 */  "on.",
  /* 26 */  "off.",
  /* 27 */  "Done.",
  /* 28 */  "Pozsar Zsolt",
  /* 29 */  "  device MAC address: ",
  /* 30 */  "  Page not found!",
  /* 31 */  "Serial number of hardware: ",
  /* 32 */  "  get help page",
  /* 33 */  "  get device information",
  /* 34 */  "  get all measured data",
  /* 35 */  "  get relative unwanted gas level",
  /* 36 */  "  get relative humidity",
  /* 37 */  "  get temperature",
  /* 38 */  "  get all measured data and set limit values",
  /* 39 */  "    gas level:\t\t",
  /* 40 */  "    humidity:\t\t",
  /* 41 */  "    temperature:\t",
  /* 42 */  "* Periodic measure.",
  /* 43 */  "  limit values:",
  /* 44 */  "  Not enough argument!",
  /* 45 */  "  measured data:",
  /* 46 */  "* E05: Page not found!",
  /* 47 */  "  get status of green LED",
  /* 48 */  "  get status of yellow LED",
  /* 49 */  "  get status of red LED",
  /* 50 */  "  set status of green LED",
  /* 51 */  "  set status of yellow LED",
  /* 52 */  "  set status of red LED",
  /* 53 */  "  set automatic operation mode",
  /* 54 */  "  set manual operation mode",
  /* 55 */  "  get system log",
  /* 56 */  "  cannot set status of LEDs in manual mode",
  /* 57 */  "* HTTP request received.",
  /* 58 */  "  get operation mode",
  /* 59 */  "  get summary",
};

DHT dht(prt_sensor1, TYP_SENSOR1, 11);
ESP8266WebServer server(80);

// initializing function
void setup(void)
{
  // set serial port
  Serial.begin(115200);
  // write program information
  Serial.println("");
  Serial.println("");
  Serial.println(msg[1] + " * v" + swversion );
  Serial.println(msg[2] +  " " + msg[28] + " <" + msg[3] + ">");
  Serial.println(msg[31] + serialnumber );
  // initialize GPIO ports
  writesyslog(5);
  Serial.print(msg[5]);
  pinMode(prt_buzzer, OUTPUT);
  pinMode(prt_led_blue, OUTPUT);
  pinMode(prt_led_green, OUTPUT);
  pinMode(prt_led_red, OUTPUT);
  pinMode(prt_led_yellow, OUTPUT);
  digitalWrite(prt_led_blue, LOW);
  digitalWrite(prt_led_green, LOW);
  digitalWrite(prt_led_red, LOW);
  digitalWrite(prt_led_yellow, LOW);
  Serial.println(msg[8]);
  // initialize sensors
  writesyslog(6);
  Serial.print(msg[6]);
  dht.begin();
  Serial.println(msg[8]);
  // connect to wireless network
  writesyslog(7);
  Serial.print(msg[7]);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }
  Serial.println(msg[8]);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  deviceipaddress = WiFi.localIP().toString();
  devicemacaddress = WiFi.macAddress();
  Serial.println(msg[29] + devicemacaddress);
  Serial.println(msg[9] + deviceipaddress);
  Serial.println(msg[10] + WiFi.subnetMask().toString());
  Serial.println(msg[11] + WiFi.gatewayIP().toString());
  // start webserver
  writesyslog(12);
  Serial.print(msg[12]);
  server.onNotFound(handleNotFound);

  // Group #1: information pages
  // write help page
  server.on("/", []()
  {
    writeclientipaddress();
    Serial.println(msg[32]);
    writesyslog(32);
    line =
      "<html>\n"
      "  <head>\n"
      "    <title>" + msg[1] + " | Help page</title>\n"
      "  </head>\n"
      "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
      "    <h2>" + msg[1] + "</h2>\n"
      "    <br>\n"
      "    IP address: " + deviceipaddress + "<br>\n"
      "    MAC address: " + devicemacaddress + "<br>\n"
      "    Hardware serial number: " + serialnumber + "<br>\n"
      "    Software version: v" + swversion + "<br>\n"
      "    <hr>\n"
      "    <h3>Pages:</h3>\n"
      "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Information pages</b></td></tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "</td>\n"
      "        <td>This page</td>\n"
      "        <td>" + texthtml + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/summary?uid=abcdef</td>\n"
      "        <td>Summary of status</td>\n"
      "        <td>" + texthtml + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/log?uid=abcdef</td>\n"
      "        <td>System log</td>\n"
      "        <td>" + texthtml + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/version</td>\n"
      "        <td>Device information</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Operation mode</b></td>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/mode/?uid=abcdef</td>\n"
      "        <td>Get operation mode</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/mode/auto?uid=abcdef</td>\n"
      "        <td>Set automatic mode</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/mode/manual?uid=abcdef</td>\n"
      "        <td>Set manual mode</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Get data</b></td></tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/all?uid=abcdef</td>\n"
      "        <td>Get all measured data</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/humidity?uid=abcdef</td>\n"
      "        <td>Get relative humidity in %</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/temperature?uid=abcdef</td>\n"
      "        <td>Get temperature in &deg;C</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/unwantedgaslevel?uid=abcdef</td>\n"
      "        <td>Get relative level of unwanted gases in %</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/greenled?uid=abcdef</td>\n"
      "        <td>Get status of green LED</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/yellowled?uid=abcdef</td>\n"
      "        <td>Get status of yellow LED</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/redled?uid=abcdef</td>\n"
      "        <td>Get status of red LED</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Automatic operation</b></td></tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/operation?uid=abcdef&g=20&h1=65&h2=70&h3=80&h4=85&t1=13&t2=150&t3=20&t4=22</td>\n"
      "        <td>Get all measured data and set limit values</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Manual operation</b></td></tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/all/off?uid=abcdef</td>\n"
      "        <td>Switch off all LEDs</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/greenled/off?uid=abcdef</td>\n"
      "        <td>Switch off green LED</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/greenled/on?uid=abcdef</td>\n"
      "        <td>Switch on green LED</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/yellowled/off?uid=abcdef</td>\n"
      "        <td>Switch off yellow LED</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/yellowled/on?uid=abcdef</td>\n"
      "        <td>Switch on yellow LED</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/redled/off?uid=abcdef</td>\n"
      "        <td>Switch off red LED</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/redled/on?uid=abcdef</td>\n"
      "        <td>Switch on red LED</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "    </table>\n"
      "    <br>\n"
      "    <hr>\n"
      "    <center>" + msg[2] + " <a href=\"mailto:" + msg[3] + "\">" + msg[28] + "</a> - <a href=\"" + msg[4] + "\">Homepage</a><center>\n"
      "    <br>\n"
      "  </body>\n"
      "</html>\n";
    server.send(200, texthtml, line);
    delay(100);
  });
  // write summary of status
  server.on("/summary", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[59]);
        writesyslog(59);
        line =
          "<html>\n"
          "  <head>\n"
          "    <title>" + msg[1] + " | Summary of status</title>\n"
          "    <meta http-equiv=\"refresh\" content=\"60\">\n"
          "  </head>\n"
          "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
          "    <h2>" + msg[1] + "</h2>\n"
          "    <br>\n"
          "    IP address: " + deviceipaddress + "<br>\n"
          "    MAC address: " + devicemacaddress + "<br>\n"
          "    Hardware serial number: " + serialnumber + "<br>\n"
          "    Software version: v" + swversion + "<br>\n"
          "    <hr>\n"
          "    <h3>Summary of status:</h3>\n"
          "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
          "      <tr><td colspan=\"3\" align=\"center\"><b>Operation</b></td></tr>\n"
          "      <tr>\n"
          "        <td>Mode:</td>\n"
          "        <td>";
        if (autoopmode == false) line = line + "manual"; else line = line + "automatic";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Temperature limit values:</td>\n"
               "        <td>" + String((int)t1) + " &deg;C, " + String((int)t2) + " &deg;C, " + String((int)t3) + " &deg;C, " + String((int)t4) + " &deg;C</td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Humidity limit values:</td>\n"
               "        <td>" + String((int)h1) + "%, " + String((int)h2) + " %, " + String((int)h3) + "%, " + String((int)h4) + "%</td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Gas level limit value:</td>\n"
               "        <td>" + String((int)g) + "%</td>\n"
               "      </tr>\n"
               "      <tr><td colspan = \"3\" align=\"center\"><b>Measured value</b></td></tr>\n"
               "      <tr>\n"
               "        <td>Temperature:</td>\n"
               "        <td>" + String((int)temperature) + " &deg;C</td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Relative humidity:</td>\n"
               "        <td>" + String((int)humidity) + "%</td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Relative gas level:</td>\n"
               "        <td>" + String((int)unwantedgaslevel) + "%</td>\n"
               "      </tr>\n"
               "      <tr><td colspan=\"3\" align=\"center\"><b>Status LEDs</b></td></tr>\n"
               "      <tr>\n"
               "        <td>Green:</td>\n"
               "        <td>";
        if (green == 1) line = line + "ON"; else line = line + "OFF";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Yellow:</td>\n"
               "        <td>";
        if (yellow == 1) line = line + "ON"; else line = line + "OFF";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Red:</td>\n"
               "        <td>";
        if (red == 1) line = line + "ON"; else line = line + "OFF";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "    </table>\n"
               "    <br>\n"
               "    <hr>\n"
               "    <center>" + msg[2] + " <a href=\"mailto:" + msg[3] + "\">" + msg[28] + "</a> - <a href=\"" + msg[4] + "\">Homepage</a><center>\n"
               "    <br>\n"
               "  </body>\n"
               "</html>\n";
        server.send(200, texthtml, line);
      }
  });
  // write log
  server.on("/log", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[55]);
        writesyslog(55);
        line =
          "<html>\n"
          "  <head>\n"
          "    <title>" + msg[1] + " | System log</title>\n"
          "  </head>\n"
          "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
          "    <h2>" + msg[1] + "</h2>\n"
          "    <br>\n"
          "    IP address: " + deviceipaddress + "<br>\n"
          "    MAC address: " + devicemacaddress + "<br>\n"
          "    Hardware serial number: " + serialnumber + "<br>\n"
          "    Software version: v" + swversion + "<br>\n"
          "    <hr>\n"
          "    <h3>Last 64 lines of system log:</h3>\n"
          "    <table border=\"0\" cellpadding=\"3\" cellspacing=\"0\">\n";
        for (int i = 0; i < 64; i++)
          if (syslog[i] > 0)
            line = line + "      <tr><td><pre>" + String(i) + "</pre></td><td><pre>" + msg[syslog[i]] + "</pre></td></tr>\n";
        line = line +
               "    </table>\n"
               "    <br>\n"
               "    <hr>\n"
               "    <center>" + msg[2] + " <a href=\"mailto:" + msg[3] + "\">" + msg[28] + "</a> - <a href=\"" + msg[4] + "\">Homepage</a><center>\n"
               "    <br>\n"
               "  </body>\n"
               "</html>\n";
        server.send(200, texthtml, line);
      }
  });
  // write device information
  server.on("/version", []()
  {
    writeclientipaddress();
    Serial.println(msg[33]);
    writesyslog(33);
    line = msg[16] + "\n" + swversion + "\n" + serialnumber;
    server.send(200, textplain, line);
  });

  // Group #2: set operation mode (auto/manual)
  // get operation mode
  server.on("/mode", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[58]);
        writesyslog(58);
        line = String((int)autoopmode);
        server.send(200, textplain, line);
      }
  });
  // set automatic operation mode
  server.on("/mode/auto", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[53]);
        writesyslog(53);
        autoopmode = true;
        server.send(200, textplain, msg[27]);
      }
  });
  // set manual operation mode
  server.on("/mode/manual", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[54]);
        writesyslog(54);
        autoopmode = false;
        server.send(200, textplain, msg[27]);
      }
  });

  // Group #3: get data
  // get all measured parameters
  server.on("/get/all", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[34]);
        writesyslog(34);
        line = String((int)unwantedgaslevel) + "\n" + String((int)humidity) + "\n" + String((int)temperature);
        server.send(200, textplain, line);
      }
  });
  // get relative unwanted gas level in percent
  server.on("/get/unwantedgaslevel", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[35]);
        writesyslog(35);
        line = String((int)unwantedgaslevel);
        server.send(200, textplain, line);
      }
  });
  // get relative humidity in percent
  server.on("/get/humidity", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[36]);
        writesyslog(36);
        line = String((int)humidity);
        server.send(200, textplain, line);
      }
  });
  // get temperature in degree Celsius
  server.on("/get/temperature", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[37]);
        writesyslog(37);
        line = String((int)temperature);
        server.send(200, textplain, line);
      }
  });
  // get status of green LED
  server.on("/get/greenled", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[47]);
        writesyslog(47);
        line = String((int)green);
        server.send(200, textplain, line);
      }
  });
  // get status of yellow LED
  server.on("/get/yellowled", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[48]);
        writesyslog(48);
        line = String((int)yellow);
        server.send(200, textplain, line);
      }
  });
  // get status of red LED
  server.on("/get/redled", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[49]);
        writesyslog(49);
        line = String((int)red);
        server.send(200, textplain, line);
      }
  });

  // Group #4: automatic operation
  // set limit values and get all measured parameters
  server.on("/operation", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[38]);
        writesyslog(38);
        if (setlimitvalues() == 0) leds();
        line = String((int)unwantedgaslevel) + "\n" + String((int)humidity) + "\n" + String((int)temperature);
        server.send(200, textplain, line);
      }
  });

  // Group #5: manual operation
  // set status of all LEDs to off in manual operation mode
  server.on("/set/all/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
        if (autoopmode == false)
        {
          Serial.println(msg[50]);
          Serial.println(msg[51]);
          Serial.println(msg[52]);
          writesyslog(50);
          writesyslog(51);
          writesyslog(52);
          green = 0;
          red = 0;
          yellow = 0;
          server.send(200, textplain, msg[27]);
        }
  });
  // set status of green LED to off in manual operation mode
  server.on("/set/greenled/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
        if (autoopmode == false)
        {
          Serial.println(msg[50]);
          writesyslog(50);
          green = 0;
          server.send(200, textplain, msg[27]);
        } else
          server.send(200, textplain, msg[56]);
  });
  // set status of green LED to on in manual operation mode
  server.on("/set/greenled/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
        if (autoopmode == false)
        {
          Serial.println(msg[50]);
          writesyslog(50);
          green = 1;
          server.send(200, textplain, msg[27]);
        } else
          server.send(200, textplain, msg[56]);
  });
  // set status of yellow LED to off in manual operation mode
  server.on("/set/yellowled/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
        if (autoopmode == false)
        {
          Serial.println(msg[51]);
          writesyslog(51);
          yellow = 0;
          server.send(200, textplain, msg[27]);
        } else
          server.send(200, textplain, msg[56]);
  });
  // set status of yellow LED to on in manual operation mode
  server.on("/set/yellowled/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
        if (autoopmode == false)
        {
          Serial.println(msg[51]);
          writesyslog(51);
          yellow = 1;
          server.send(200, textplain, msg[27]);
        } else
          server.send(200, textplain, msg[56]);
  });
  // set status of red LED to off in manual operation mode
  server.on("/set/redled/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
        if (autoopmode == false)
        {
          Serial.println(msg[52]);
          writesyslog(52);
          red = 0;
          server.send(200, textplain, msg[27]);
        } else
          server.send(200, textplain, msg[56]);
  });
  // set status of red LED to on in manual operation mode
  server.on("/set/redled/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
        if (autoopmode == false)
        {
          Serial.println(msg[52]);
          writesyslog(52);
          red = 1;
          server.send(200, textplain, msg[27]);
        } else
          server.send(200, textplain, msg[56]);
  });
  server.begin();
  Serial.println(msg[8]);
  beep(1);
}

// error 404 page
void handleNotFound()
{
  writeclientipaddress();
  Serial.println(msg[46]);
  writesyslog(46);
  server.send(404, textplain, msg[30]);
}

// loop function
void loop(void)
{
  server.handleClient();
  unsigned long currtime3 = millis();
  if (currtime3 - prevtime3 >= interval3)
  {
    prevtime3 = currtime3;
    Serial.println(msg[42]);
    Serial.println(msg[45]);
    writesyslog(42);
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
    Serial.println(msg[21] + msg[24] + msg[26]);
  } else
  {
    digitalWrite(prt_led_green, HIGH);
    Serial.println(msg[21] + msg[24] + msg[25]);
  }
}

// switch on/off yellow LED
void yellowled(int i)
{
  if (i == 0)
  {
    digitalWrite(prt_led_yellow, LOW);
    Serial.println(msg[23] + msg[24] + msg[26]);
  } else
  {
    digitalWrite(prt_led_yellow, HIGH);
    Serial.println(msg[23] + msg[24] + msg[25]);
  }
}

// switch on/off red LED
void redled(int i)
{
  if (i == 0)
  {
    digitalWrite(prt_led_red, LOW);
    Serial.println(msg[22] + msg[24] + msg[26]);
  } else
  {
    digitalWrite(prt_led_red, HIGH);
    Serial.println(msg[22] + msg[24] + msg[25]);
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
    Serial.println(msg[43]);
    Serial.println(msg[39] + String(g) + "%");
    Serial.println(msg[40] + String(h1) + "%; " + String(h2) + "%; " + String(h3) + "%; " + String(h4) + "%" );
    Serial.println(msg[41] + String(t1) + "°C; " + String(t2) + "°C; " + String(t3) + "°C; " + String(t4) + "°C" );
    return 0;
  } else
  {
    Serial.println(msg[44]);
    writesyslog(44);
    return 1;
  }
}

// set status of all LEDs in automatic operation mode
void leds()
{
  if (autoopmode == true)
  {
    green = 0;
    yellow = 0;
    red = 0;
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
}

// measure air quality
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
      Serial.println(msg[14]);
      writesyslog(14);
      unwantedgaslevel = 999;
      return;
    } else Serial.println(msg[39] + String((int)unwantedgaslevel) + "%");
  }
}

// measure temperature and relative humidity
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
      Serial.println(msg[15]);
      writesyslog(15);
      temperature = 999;
      humidity = 999;
      return;
    } else
    {
      Serial.println(msg[40] + String((int)humidity) + "%");
      Serial.println(msg[41] + String((int)temperature) + " °C");
    }
  }
}

// blink blue LED and write client IP address to serial console
void writeclientipaddress()
{
  digitalWrite(prt_led_blue, HIGH);
  delay(100);
  digitalWrite(prt_led_blue, LOW);
  clientaddress = server.client().remoteIP().toString();
  Serial.println(msg[13] + clientaddress + ".");
  writesyslog(57);
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
    server.send(401, textplain, msg[19]);
    Serial.println(msg[20]);
    writesyslog(20);
    beep(3);
    return 0;
  }
}

// check UID
int checkuid()
{
  if (server.arg("uid") == uid) return 1; else
  {
    server.send(401, textplain, msg[17]);
    Serial.println(msg[18]);
    writesyslog(18);
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

// write a line to system log
void writesyslog(int msgnum)
{
  if (syslog[63] == 0)
  {
    for (int i = 0; i < 64; i++)
    {
      if (syslog[i] == 0)
      {
        syslog[i] = msgnum;
        break;
      }
    }
  } else
  {
    for (int i = 1; i < 64; i++)
      syslog[i - 1] = syslog[i];
    syslog[63] = msgnum;
  }
}
