// +---------------------------------------------------------------------------+
// | MM7D v0.4 * T/RH measuring device                                         |
// | Copyright (C) 2020-2023 Pozsár Zsolt <pozsarzs@gmail.com>                 |
// | mm7d.ino                                                                  |
// | Program for Adafruit Huzzah Breakout                                      |
// +---------------------------------------------------------------------------+

//   This program is free software: you can redistribute it and/or modify it
// under the terms of the European Union Public License 1.2 version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.

#include <DHT.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ModbusIP_ESP8266.h>
#include <ModbusRTU.h>
#include <StringSplitter.h> // Note: Change MAX = 5 to MAX = 6 in StringSplitter.h.

#define       TYP_SENSOR DHT11

// settings
const bool    SERIAL_CONSOLE    = true;           // enable/disable boot-time serial console
const bool    HTTP              = true;           // enable/disable HTTP access
const bool    MODBUS_TCP        = true;           // enable/disable Modbus/TCP access
const int     COM_SPEED         = 9600;           // baudrate of the serial port
const int     MB_UID            = 4;              // Modbus UID
const char   *WIFI_SSID         = "";             // Wifi SSID
const char   *WIFI_PASSWORD     = "";             // Wifi password

// ports
const int     PRT_AI            = 0;
const int     PRT_DI_SENSOR     = 12;
const int     PRT_DO_BUZZER     = 14;
const int     PRT_DO_LEDBLUE    = 2;
const int     PRT_DO_LEDGREEN   = 0;
const int     PRT_DO_LEDRED     = 5;
const int     PRT_DO_LEDYELLOW  = 4;

// name of the Modbus registers
const String  DI_NAME[3]        =
{
  /* 10001 */       "ledg",
  /* 10002 */       "ledy",
  /* 10003 */       "ledr"
};
const String  IR_NAME[3]        =
{
  /* 30001 */       "rhint",
  /* 30002 */       "tint",
  /* 30003 */       "vcc"
};
const String  HR_NAME[6]        =
{
  /* 40001-40008 */ "name",
  /* 40009-40011 */ "version",
  /* 40012-40017 */ "mac_address",
  /* 40018-40021 */ "ip_address",
  /* 40022       */ "modbus_uid",
  /* 40023-40028 */ "com_speed"
};

// other constants
const int     MINVCC            = 4000; // mV
const int     MAXVCC            = 6000; // mV
const long    INTERVAL          = 10000; // ms
const String  SWNAME            = "MM7D";
const String  SWVERSION         = "0.4.0";
const String  TEXTHTML          = "text/html";
const String  TEXTPLAIN         = "text/plain";
const String  DOCTYPEHTML       = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n";

// other variables
bool          extmbquery;
int           syslog[64]        = {};
String        htmlheader;
String        line;
String        myipaddress       = "0.0.0.0";
String        mymacaddress      = "00:00:00:00:00:00";
unsigned long prevtime          = 0;

// messages
const String  MSG[63]           =
{
  /*  0 */  "",
  /*  1 */  "MM7D * T/RH measuring device",
  /*  2 */  "Copyright (C) 2023 ",
  /*  3 */  "  software version:       ",
  /*  4 */  "Starting device...",
  /*  5 */  "* Initializing GPIO ports",
  /*  6 */  "* Initializing sensor",
  /*  7 */  "* Connecting to wireless network",
  /*  8 */  "done",
  /*  9 */  "  my MAC address:         ",
  /* 10 */  "  my IP address:          ",
  /* 11 */  "  subnet mask:            ",
  /* 12 */  "  gateway IP address:     ",
  /* 13 */  "* Starting Modbus/TCP server",
  /* 14 */  "* Starting Modbus/RTU slave",
  /* 15 */  "  my Modbus UID:          ",
  /* 16 */  "  serial port speed:      ",
  /* 17 */  " baud (8N1)",
  /* 18 */  "* Starting webserver",
  /* 19 */  "* Ready, the serial console is off.",
  /* 20 */  "* HTTP query received:",
  /* 21 */  "  get help page",
  /* 22 */  "  get summary page",
  /* 23 */  "  get log page",
  /* 24 */  "  get all data",
  /* 25 */  "* E01: Failed to read T/RH sensor!",
  /* 26 */  "* E02: Bad power voltage!",
  /* 27 */  "* E03: No such page!",
  /* 28 */  "Pozsar Zsolt",
  /* 29 */  "http://www.pozsarzs.hu",
  /* 30 */  "Error 404",
  /* 31 */  "No such page!",
  /* 32 */  "Help",
  /* 33 */  "Information and data access",
  /* 34 */  "Information pages",
  /* 35 */  "Data access with HTTP",
  /* 36 */  "all measured values and status in CSV format",
  /* 37 */  "all measured values and status in JSON format",
  /* 38 */  "all measured values and status in TXT format",
  /* 39 */  "all measured values and status in XML format",
  /* 40 */  "Data access with Modbus",
  /* 41 */  "Status: (read-only)",
  /* 42 */  "Measured values: (read-only)",
  /* 43 */  "Software version: (read-only)",
  /* 44 */  "device name",
  /* 45 */  "software version",
  /* 46 */  "Network settings: (read-only)",
  /* 47 */  "MAC address",
  /* 48 */  "IP address",
  /* 49 */  "Modbus UID",
  /* 50 */  "serial port speed",
  /* 51 */  "Summary",
  /* 52 */  "All measured values and status",
  /* 53 */  "Log",
  /* 54 */  "Last 64 lines of the system log:",
  /* 55 */  "back",
  /* 56 */  "Remote access:",
  /* 57 */  "  HTTP                    ",
  /* 58 */  "  Modbus/RTU              ",
  /* 59 */  "  Modbus/TCP              ",
  /* 60 */  "enable",
  /* 61 */  "disable",
  /* 62 */  "* Modbus query received"
};
const String DI_DESC[3]         =
{
  /*  0 */  "status of the green LED",
  /*  1 */  "status of the yellow LED",
  /*  2 */  "status of the red LED"
};
const String  IR_DESC[3]        =
{
  /*  0 */  "internal relative humidity in %",
  /*  1 */  "internal temperature in ",
  /*  2 */  "power voltage in "
};

DHT dht(PRT_DI_SENSOR, TYP_SENSOR, 11);
ESP8266WebServer httpserver(80);
ModbusIP mbtcp;
ModbusRTU mbrtu;

// --- SYSTEM LOG ---
// write a line to system log
void writetosyslog(int msgnum)
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

// --- STATIC MODBUS REGISTERS ---
// convert hex string to byte
byte hstol(String recv) {
  return strtol(recv.c_str(), NULL, 16);
}

// fill holding registers with configuration data
void fillholdingregisters()
{
  int    itemCount;
  String s;
  // name
  s = SWNAME;
  while (s.length() < 8)
    s = char(0x00) + s;
  for (int i = 0; i < 9; i++)
    mbrtu.Hreg(i, char(s[i]));
  // version
  StringSplitter *splitter1 = new StringSplitter(SWVERSION, '.', 3);
  itemCount = splitter1->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter1->getItemAtIndex(i);
    mbrtu.Hreg(8 + i, item.toInt());
  }
  delete splitter1;
  // MAC-address
  StringSplitter *splitter2 = new StringSplitter(mymacaddress, ':', 6);
  itemCount = splitter2->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter2->getItemAtIndex(i);
    mbrtu.Hreg(11 + i, hstol(item));
  }
  delete splitter2;
  // IP-address
  StringSplitter *splitter3 = new StringSplitter(myipaddress, '.', 4);
  itemCount = splitter3->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter3->getItemAtIndex(i);
    mbrtu.Hreg(17 + i, item.toInt());
  }
  delete splitter3;
  // MB UID
  mbrtu.Hreg(21, MB_UID);
  // serial speed
  s = String(COM_SPEED);
  while (s.length() < 6)
    s = char(0x00) + s;
  for (int i = 0; i < 6; i++)
    mbrtu.Hreg(22 + i, char(s[i]));
}

// --- LEDS AND BUZZER ---
// switch on/off blue LED
void blueled(boolean b)
{
  digitalWrite(PRT_DO_LEDBLUE, b);
}

// switch on/off green LED
void greenled(boolean b)
{
  mbrtu.Ists(0, b);
  digitalWrite(PRT_DO_LEDGREEN, b);
}

// switch on/off yellow LED
void yellowled(boolean b)
{
  mbrtu.Ists(1, b);
  digitalWrite(PRT_DO_LEDYELLOW, b);
}

// switch on/off red LED
void redled(boolean b)
{
  mbrtu.Ists(2, b);
  digitalWrite(PRT_DO_LEDRED, b);
}

// blinking blue LED
void blinkblueled()
{
  blueled(true);
  delay(25);
  blueled(false);
}

// blinking yellow LED
void blinkyellowled()
{
  yellowled(true);
  delay(25);
  yellowled(false);
}

// blinking all LEDs
void knightrider()
{
  blueled(true);
  delay(75);
  blueled(false);
  greenled(true);
  delay(75);
  greenled(false);
  yellowled(true);
  delay(75);
  yellowled(false);
  redled(true);
  delay(75);
  redled(false);
  yellowled(true);
  delay(75);
  yellowled(false);
  greenled(true);
  delay(75);
  greenled(false);
}

// beep sign
void beep(int num)
{
  for (int i = 0; i < num; i++)
  {
    tone(PRT_DO_BUZZER, 880);
    delay (100);
    noTone(PRT_DO_BUZZER);
    delay (100);
  }
}

// --- MEASURING ---
// measure internal temperature and relative humidity
boolean measureinttemphum()
{
  float fh, ft;
  fh = dht.readHumidity();
  ft = dht.readTemperature(false);
  if (isnan(fh) || isnan(ft))
  {
    beep(1);
    writetosyslog(25);
    return false;
  } else
  {
    mbrtu.Ireg(0, round(fh));
    mbrtu.Ireg(1, round(ft + 273.15)); // convert to K
    return true;
  }
}

// measure power voltage
boolean measurepowervoltage()
{
  int adc;
  float vadc;
  float vcc;
  const float R107 = 4700; // ohm
  const float R108 = 680; // ohm
  const int VREF = 1000; // mV

  adc = analogRead(PRT_AI);
  vadc = (adc * VREF) / 1024;
  vcc = vadc * (1 + (R107 / R108));
  mbrtu.Ireg(2, vcc);
  if ((vcc < MINVCC) || (vcc > MAXVCC))
  {
    beep(1);
    writetosyslog(26);
    return false;
  } else return true;
}

// --- DATA RETRIEVING ---
// blink blue LED and write to log
uint16_t modbusquery(TRegister* reg, uint16_t val)
{
  blinkblueled();
  if (extmbquery) writetosyslog(62);
  return val;
}

// --- WEBPAGES ---
// header for pages
void header_html()
{
  htmlheader = 
    "    <table border=\"0\">\n"
    "      <tbody>\n"
    "      <tr><td><i>" + MSG[9] + "</i></td><td>" + mymacaddress + "</td></tr>\n"
    "      <tr><td><i>" + MSG[10] + "</i></td><td>" + myipaddress + "</td></tr>\n"
    "      <tr><td><i>" + MSG[3] + "</i></td><td>v" + SWVERSION + "</td></tr>\n"
    "      <tr><td><i>" + MSG[15] + "</i></td><td>" + String(MB_UID) + "</td></tr>\n"
    "      <tr><td><i>" + MSG[16] + "</i></td><td>" + String(COM_SPEED) + MSG[17] + "</td></tr>\n"
    "      </tbody>\n"
    "    </table>\n";
}

// error 404 page
void handleNotFound()
{
  writetosyslog(20);
  writetosyslog(27);
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | " + MSG[30] + "</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n" + htmlheader + "    <hr>\n"
         "    <h3>" + MSG[30] + "</h3>\n" + MSG[31] + "\n"
         "    <br>\n"
         "    <hr>\n"
         "    <div align=\"right\"><a href=\"/\">" + MSG[55] + "</a></div>\n"
         "    <br>\n"
         "    <center>" + MSG[2] + " <a href=\"" + MSG[29] + "\">" + MSG[28] + "</a></center>\n"
         "    <br>\n"
         "  </body>\n"
         "</html>\n";
  httpserver.send(404, TEXTHTML, line);
  delay(100);
}

// help page
void handleHelp()
{
  writetosyslog(20);
  writetosyslog(21);
  extmbquery = false;
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | " + MSG[32] + "</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n" + htmlheader + "    <hr>\n"
         "    <h3>" + MSG[33] + "</h3>\n"
         "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
         "      <tr><td colspan=\"3\" align=\"center\"><b>" + MSG[34] + "</b></td></tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/\">http://" + myipaddress + "/</a></td>\n"
         "        <td>" + MSG[32] + "</td>\n"
         "        <td>" + TEXTHTML + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/summary\">http://" + myipaddress + "/summary</a></td>\n"
         "        <td>" + MSG[51] + "</td>\n"
         "        <td>" + TEXTHTML + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/log\">http://" + myipaddress + "/log</a></td>\n"
         "        <td>" + MSG[53] + "</td>\n"
         "        <td>" + TEXTHTML + "</td>\n"
         "      </tr>\n"
         "      <tr><td colspan=\"3\" align=\"center\"><b>" + MSG[35] + "</b></td>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/get/csv\">http://" + myipaddress + "/get/csv</a></td>\n"
         "        <td>" + MSG[36] + "</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/get/json\">http://" + myipaddress + "/get/json</a></td>\n"
         "        <td>" + MSG[37] + "</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/get/txt\">http://" + myipaddress + "/get/txt</a></td>\n"
         "        <td>" + MSG[38] + "</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/get/xml\">http://" + myipaddress + "/get/xml</a></td>\n"
         "        <td>" + MSG[39] + "</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr><td colspan=\"3\" align=\"center\"><b>" + MSG[40] + "</b></td>\n"
         "      <tr><td colspan=\"3\"><i>" + MSG[41] + "</i></td>\n";
  for (int i = 0; i < 3; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + String(i + 10001) + "</td>\n"
      "        <td>" + DI_DESC[i] + "</td>\n"
      "        <td>bit</td>\n"
      "      </tr>\n";
  }
  line += "      <tr><td colspan=\"3\"><i>" + MSG[42] + "</i></td>\n";
  String s;
  for (int i = 0; i < 3; i++)
  {
    s = IR_DESC[i] ;
    if (i == 1) s += "K";
    if (i == 2) s += "mV";
    line +=
      "      <tr>\n"
      "        <td>" + String(i + 30001) + "</td>\n"
      "        <td>" + s + "</td>\n"
      "        <td>uint16</td>\n"
      "      </tr>\n";
  }
  line +=
    "      <tr><td colspan=\"3\"><i>" + MSG[43] + "</i></td>\n"
    "      <tr>\n"
    "        <td>40001-40008</td>\n"
    "        <td>" + MSG[44] + "</td>\n"
    "        <td>8 char</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>40009-40011</td>\n"
    "        <td>" + MSG[45] + "</td>\n"
    "        <td>3 byte</td>\n"
    "      </tr>\n"
    "      <tr><td colspan=\"3\"><i>" + MSG[46] + "</i></td>\n"
    "      <tr>\n"
    "        <td>40012-40017</td>\n"
    "        <td>" + MSG[47] + "</td>\n"
    "        <td>6 byte</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>40018-40021</td>\n"
    "        <td>" + MSG[48] + "</td>\n"
    "        <td>4 byte</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>40022</td>\n"
    "        <td>" + MSG[49] + "</td>\n"
    "        <td>1 byte</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>40023-40028</td>\n"
    "        <td>" + MSG[50] + "</td>\n"
    "        <td>6 char</td>\n"
    "      </tr>\n"
    "    </table>\n"
    "    <br>\n"
    "    <hr>\n"
    "    <br>\n"
    "    <center>" + MSG[2] + " <a href=\"" + MSG[29] + "\">" + MSG[28] + "</a></center>\n"
    "    <br>\n"
    "  </body>\n"
    "</html>\n";
  httpserver.send(200, TEXTHTML, line);
  extmbquery = true;
  delay(100);
};

// summary page
void handleSummary()
{
  writetosyslog(20);
  writetosyslog(22);
  extmbquery = false;
  float f;
  f = mbrtu.Ireg(2);
  f = f / 1000;
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | " + MSG[51] + "</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n" + htmlheader + "    <hr>\n"
         "    <h3>" + MSG[52] + "</h3>\n"
         "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
         "      <tr>\n"
         "        <td>" + IR_DESC[0] + "</td>\n"
         "        <td align=\"right\">" + String(mbrtu.Ireg(0)) + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td>" + IR_DESC[1] + "&deg;C</td>\n"
         "        <td align=\"right\">" + String(mbrtu.Ireg(1) - 273) + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td>" + IR_DESC[2] + "V</td>\n"
         "        <td align=\"right\">" + String(f) + "</td>\n"
         "      </tr>\n";
  for (int i = 0; i < 3; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + DI_DESC[i] + "</td>\n"
      "        <td align=\"right\">" + String(mbrtu.Ists(i)) + "</td>\n"
      "      </tr>\n";
  }
  line +=
    "    </table>\n"
    "    <br>\n"
    "    <hr>\n"
    "    <div align=\"right\"><a href=\"/\">" + MSG[55] + "</a></div>\n"
    "    <br>\n"
    "    <center>" + MSG[2] + " <a href=\"" + MSG[29] + "\">" + MSG[28] + "</a></center>\n"
    "    <br>\n"
    "  </body>\n"
    "</html>\n";
  httpserver.send(200, TEXTHTML, line);
  extmbquery = true;
  delay(100);
}

// log page
void handleLog()
{
  writetosyslog(20);
  writetosyslog(23);
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | " + MSG[53] + "</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n" + htmlheader + "    <hr>\n"
         "    <h3>" + MSG[54] + "</h3>\n"
         "    <table border=\"0\" cellpadding=\"3\" cellspacing=\"0\">\n";
  for (int i = 0; i < 64; i++)
    if (syslog[i] > 0)
      line += "      <tr><td align=right><b>" + String(i) + "</b></td><td>" + MSG[syslog[i]] + "</td></tr>\n";
  line +=
    "    </table>\n"
    "    <br>\n"
    "    <hr>\n"
    "    <div align=\"right\"><a href=\"/\">" + MSG[55] + "</a></div>\n"
    "    <br>\n"
    "    <center>" + MSG[2] + " <a href=\"" + MSG[29] + "\">" + MSG[28] + "</a></center>\n"
    "    <br>\n"
    "  </body>\n"
    "</html>\n";
  httpserver.send(200, TEXTHTML, line);
  delay(100);
}

// get all measured data in CSV format
void handleGetCSV()
{
  writetosyslog(20);
  writetosyslog(24);
  extmbquery = false;
  line = "\"" + HR_NAME[0] + "\",\"" + SWNAME + "\"\n"
         "\"" + HR_NAME[1] + "\",\"" + SWVERSION + "\"\n"
         "\"" + HR_NAME[2] + "\",\"" + mymacaddress + "\"\n"
         "\"" + HR_NAME[3] + "\",\"" + myipaddress + "\"\n"
         "\"" + HR_NAME[4] + "\",\"" + String(MB_UID) + "\"\n"
         "\"" + HR_NAME[5] + "\",\"" + String(COM_SPEED) + MSG[17] + "\"\n";
  for (int i = 0; i < 3; i++)
    line += "\"" + IR_NAME[i] + "\",\"" + String(mbrtu.Ireg(i)) + "\"\n";
  for (int i = 0; i < 3; i++)
    line += "\"" + DI_NAME[i] + "\",\"" + String(mbrtu.Ists(i)) + "\"\n";
  httpserver.send(200, TEXTPLAIN, line);
  extmbquery = true;
  delay(100);
}

// get all measured values in JSON format
void handleGetJSON()
{
  writetosyslog(20);
  writetosyslog(24);
  extmbquery = false;
  line = "{\n"
         "  \"software\": {\n"
         "    \"" + HR_NAME[0] + "\": \"" + SWNAME + "\",\n"
         "    \"" + HR_NAME[1] + "\": \"" + SWVERSION + "\"\n"
         "  },\n"
         "  \"hardware\": {\n"
         "    \"" + HR_NAME[2] + "\": \"" + mymacaddress + "\",\n"
         "    \"" + HR_NAME[3] + "\": \"" + myipaddress + "\",\n"
         "    \"" + HR_NAME[4] + "\": \"" + String(MB_UID) + "\",\n"
         "    \"" + HR_NAME[5] + "\": \"" + String(COM_SPEED) + MSG[17] + "\"\n"
         "  },\n"
         "  \"data\": {\n"
         "    \"integer\": {\n";
  for (int i = 0; i < 3; i++)
  {
    line += "      \"" + IR_NAME[i] + "\": \"" + String(mbrtu.Ireg(i));
    if (i < 2 ) line += "\",\n"; else  line += "\"\n";
  }
  line +=
    "    },\n"
    "    \"bit\": {\n";
  for (int i = 0; i < 3; i++)
  {
    line += "      \"" + DI_NAME[i] + "\": \"" + String(mbrtu.Ists(i));
    if (i < 2 ) line += "\",\n"; else  line += "\"\n";
  }
  line +=
    "    }\n"
    "  }\n"
    "}\n";
  httpserver.send(200, TEXTPLAIN, line);
  extmbquery = true;
  delay(100);
}

// get all measured data in TXT format
void handleGetTXT()
{
  writetosyslog(20);
  writetosyslog(24);
  extmbquery = false;
  line = SWNAME + "\n" +
         SWVERSION + "\n" +
         mymacaddress + "\n" +
         myipaddress + "\n" + \
         String(MB_UID) + "\n" + \
         String(COM_SPEED) + MSG[17] + "\n";
  for (int i = 0; i < 3; i++)
    line += String(mbrtu.Ireg(i)) + "\n";
  for (int i = 0; i < 3; i++)
    line += String(mbrtu.Ists(i)) + "\n";
  httpserver.send(200, TEXTPLAIN, line);
  extmbquery = true;
  delay(100);
}

// get all measured values in XML format
void handleGetXML()
{
  writetosyslog(20);
  writetosyslog(24);
  extmbquery = false;
  line = "<xml>\n"
         "  <software>\n"
         "    <" + HR_NAME[0] + ">" + SWNAME + "</" + HR_NAME[0] + ">\n"
         "    <" + HR_NAME[1] + ">" + SWVERSION + "</" + HR_NAME[1] + ">\n"
         "  </software>\n"
         "  <hardware>\n"
         "    <" + HR_NAME[2] + ">" + mymacaddress + "</" + HR_NAME[2] + ">\n"
         "    <" + HR_NAME[3] + ">" + myipaddress + "</" + HR_NAME[3] + ">\n"
         "    <" + HR_NAME[4] + ">" + String(MB_UID) + "</" + HR_NAME[4] + ">\n"
         "    <" + HR_NAME[5] + ">" + String(COM_SPEED) + MSG[17] + "</" + HR_NAME[5] + ">\n"
         "  </hardware>\n"
         "  <data>\n"
         "    <integer>\n";
  for (int i = 0; i < 3; i++)
    line += "      <" + IR_NAME[i] + ">" + String(mbrtu.Ireg(i)) + "</" + IR_NAME[i] + ">\n";
  line +=
    "    </integer>\n"
    "    <bit>\n";
  for (int i = 0; i < 3; i++)
    line += "      <" + DI_NAME[i] + ">" + String(mbrtu.Ists(i)) + "</" + DI_NAME[i] + ">\n";
  line +=
    "    </bit>\n"
    "  </data>\n"
    "</xml>";
  httpserver.send(200, TEXTPLAIN, line);
  extmbquery = true;
  delay(100);
}

// --- MAIN ---
// initializing function
void setup(void)
{
  // set serial port
  Serial.begin(COM_SPEED, SERIAL_8N1);
  // write program information
  if (SERIAL_CONSOLE)
  {
    Serial.println("");
    Serial.println("");
    Serial.println(MSG[1]);
    Serial.println(MSG[2] + MSG[28]);
    Serial.println(MSG[3] + "v" + SWVERSION );
    Serial.println(MSG[56]);
    Serial.print(MSG[57]);
    if (HTTP) Serial.println(MSG[60]); else Serial.println(MSG[61]);
    Serial.println(MSG[58] + MSG[60]);
    Serial.print(MSG[59]);
    if (MODBUS_TCP) Serial.println(MSG[60]); else Serial.println(MSG[61]);
  }
  writetosyslog(4);
  if (SERIAL_CONSOLE) Serial.println(MSG[4]);
  // initialize GPIO ports
  writetosyslog(5);
  if (SERIAL_CONSOLE) Serial.println(MSG[5]);
  pinMode(PRT_DO_BUZZER, OUTPUT);
  pinMode(PRT_DO_LEDBLUE, OUTPUT);
  pinMode(PRT_DO_LEDGREEN, OUTPUT);
  pinMode(PRT_DO_LEDRED, OUTPUT);
  pinMode(PRT_DO_LEDYELLOW, OUTPUT);
  digitalWrite(PRT_DO_LEDBLUE, LOW);
  digitalWrite(PRT_DO_LEDGREEN, LOW);
  digitalWrite(PRT_DO_LEDRED, LOW);
  digitalWrite(PRT_DO_LEDYELLOW, LOW);
  // initialize sensor
  writetosyslog(6);
  if (SERIAL_CONSOLE) Serial.println(MSG[6]);
  dht.begin();
  // connect to wireless network
  if (HTTP || MODBUS_TCP)
  {
    writetosyslog(7);
    if (SERIAL_CONSOLE) Serial.print(MSG[7]);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
      knightrider();
      if (SERIAL_CONSOLE) Serial.print(".");
    }
    if (SERIAL_CONSOLE) Serial.println(MSG[8]);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    myipaddress = WiFi.localIP().toString();
    mymacaddress = WiFi.macAddress();
    if (SERIAL_CONSOLE)
    {
      Serial.println(MSG[9] + mymacaddress);
      Serial.println(MSG[10] + myipaddress);
      Serial.println(MSG[11] + WiFi.subnetMask().toString());
      Serial.println(MSG[12] + WiFi.gatewayIP().toString());
    }
  }
  // start Modbus/TCP server
  if (MODBUS_TCP)
  { 
    writetosyslog(13);
    if (SERIAL_CONSOLE) Serial.println(MSG[13]);
    mbtcp.server();
  }
  // start Modbus/RTU slave
  writetosyslog(14);
  if (SERIAL_CONSOLE) Serial.println(MSG[14]);
  mbrtu.begin(&Serial);
  mbrtu.setBaudrate(COM_SPEED);
  mbrtu.slave(MB_UID);
  if (SERIAL_CONSOLE)
  {
    Serial.println(MSG[15] + String(MB_UID));
    Serial.println(MSG[16] + String(COM_SPEED) + MSG[17]);
  }
  // set Modbus registers
  mbrtu.addIsts(0, false, 3);
  mbrtu.addIreg(0, 0, 3);
  mbrtu.addHreg(0, 0, 28);
  // set Modbus callback
  mbrtu.onGetIsts(0, modbusquery, 3);
  mbrtu.onGetIreg(0, modbusquery, 3);
  mbrtu.onGetHreg(0, modbusquery, 28);
  // fill Modbus holding registers
  fillholdingregisters();
  // start webserver
  if (HTTP)
  {
    writetosyslog(17);
    if (SERIAL_CONSOLE) Serial.println(MSG[18]);
    header_html();
    httpserver.onNotFound(handleNotFound);
    httpserver.on("/", handleHelp);
    httpserver.on("/summary", handleSummary);
    httpserver.on("/log", handleLog);
    httpserver.on("/get/csv", handleGetCSV);
    httpserver.on("/get/json", handleGetJSON);
    httpserver.on("/get/txt", handleGetTXT);
    httpserver.on("/get/xml", handleGetXML);
    httpserver.begin();
  }
  if (SERIAL_CONSOLE) Serial.println(MSG[19]);
  beep(1);
}

// loop function
void loop(void)
{
  boolean measureerror;
  boolean vccerror;
  if (HTTP) httpserver.handleClient();
  unsigned long currtime = millis();
  if (currtime - prevtime >= INTERVAL)
  {
    prevtime = currtime;
    measureerror = measureinttemphum();
    vccerror = measurepowervoltage();
    greenled(measureerror);
    redled(! measureerror);
    redled(! vccerror);
    blinkyellowled();
  }
  if (MODBUS_TCP) mbtcp.task();
  delay(10);
  mbrtu.task();
  yield();
}
