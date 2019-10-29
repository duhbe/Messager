/*
 * Adapted from from https://github.com/aapicella/WiFi-enables-LED-Matrix
 * Uses max7219 display control library "MD_MAX72xx" from MajicDesigns
 * at https://github.com/MajicDesigns/MD_MAX72XX
 * 
 * For ESP8266 MOD
 * 
 */

#include <MD_MAX72xx.h>
#include <ESP8266WiFi.h>

// -----------------------------------
// IO configuration
// -----------------------------------

#define PIN_MSG_NEW 4
#define PIN_MSG_CONNECTED 2
#define PIN_BUTTON 5

// -----------------------------------
// WiFi configuration
// -----------------------------------

#ifndef STASSID
#define STASSID "edvac"
#define STAPSK  "ae03f5ab16"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

// Web server
WiFiServer server(80);

// -----------------------------------
// Display configuration
// -----------------------------------

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES  8
#define CLK_PIN   14  // or SCK
#define DATA_PIN  13  // or MOSI
#define CS_PIN    12  // or SS

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// We always wait a bit between updates of the display
#define  DELAYTIME  100  // in milliseconds


// ---------------------------------------
// Display a scrolling text
// ---------------------------------------

void scrollText(const char *p)
{
  uint8_t charWidth;
  uint8_t cBuf[8];  // this should be ok for all built-in fonts
  while (*p != '\0')
  {
    charWidth = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

    for (uint8_t i=0; i<=charWidth; i++)  // allow space between characters
    {
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
        mx.setColumn(0, cBuf[i]);
      delay(DELAYTIME);
    }
  }
}

// ---------------------------------------
// Wrap text
// ---------------------------------------

void wrapText()
{
  mx.clear();
  mx.wraparound(MD_MAX72XX::ON);

  // draw something that will show changes
  for (uint16_t j=0; j<mx.getDeviceCount(); j++)
  {
    mx.setChar(((j+1)*COL_SIZE)-1, (j&1 ? 'M' : 'W'));
  }
  delay(DELAYTIME*5);

  // run thru transformations
  for (uint16_t i=0; i<3*COL_SIZE*MAX_DEVICES; i++)
  {
    mx.transform(MD_MAX72XX::TSL);
    delay(DELAYTIME/2);
  }
  for (uint16_t i=0; i<3*COL_SIZE*MAX_DEVICES; i++)
  {
    mx.transform(MD_MAX72XX::TSR);
    delay(DELAYTIME/2);
  }
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    mx.transform(MD_MAX72XX::TSU);
    delay(DELAYTIME*2);
  }
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    mx.transform(MD_MAX72XX::TSD);
    delay(DELAYTIME*2);
  }

  mx.wraparound(MD_MAX72XX::OFF);
}

// ---------------------------------------
// Bouncing ball
// ---------------------------------------
void bounce()
// Animation of a bouncing ball
{
  const int minC = 0;
  const int maxC = mx.getColumnCount()-1;
  const int minR = 0;
  const int maxR = ROW_SIZE-1;

  static int r = 0, c = 0;
  static int8_t dR = 1, dC = 1;  // delta row and column

  mx.setPoint(r, c, false);
  r += dR;
  c += dC;
  mx.setPoint(r, c, true);
 
  if ((r == minR) || (r == maxR))
    dR = -dR;
  if ((c == minC) || (c == maxC))
    dC = -dC;
 
}

char text[100];
int len = strlen(text);


// ---------------------------------------
// SETUP
// ---------------------------------------
void clean()
{
  for ( uint8_t i=0; i< COL_SIZE*MAX_DEVICES;i++)
    mx.transform(MD_MAX72XX::TSL);
    delay(DELAYTIME/2);
}

// ---------------------------------------
// SETUP
// ---------------------------------------

void setup() {
  // Setup display
  mx.begin();
  mx.clear();

  // Setup serial line
  Serial.begin(115200);

  // Setup MSG pins
  pinMode(PIN_MSG_NEW, OUTPUT);
  digitalWrite(PIN_MSG_NEW, 1);
  pinMode(PIN_MSG_CONNECTED, OUTPUT);
  digitalWrite(PIN_MSG_CONNECTED, 1);
  
  // Setup button
  pinMode(PIN_BUTTON, INPUT);
  

  // LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));

  // Start the server
  server.begin();
  Serial.println(F("Server started"));

  // Print the IP address
  Serial.println(WiFi.localIP());
  String s = "Connected to " + WiFi.localIP().toString() + "        ";
  scrollText(s.c_str());
  clean();

  // Emit audio msg
  digitalWrite(PIN_MSG_NEW, 0);
  delay(10);
  digitalWrite(PIN_MSG_NEW, 1);

}


// ---------------------------------------
// LOOP
// ---------------------------------------

typedef enum {
  kSETUP_DISPLAY_MSG, kDISPLAY_MSG, kDISPLAY_SCREENSAVER
} eState;


void loop() {
  static eState state = kDISPLAY_SCREENSAVER;
  static int timer = 0;
  static int ctr = 0;
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    switch (state)
    {
      case kSETUP_DISPLAY_MSG:
        ctr = 5;
        mx.clear();
        state = kDISPLAY_MSG;
        break;
        
      case kDISPLAY_MSG:
        if ( ctr > 0 )
        {
          if ( timer > 0 )
          {
             timer--;
          }
          else
          {
            scrollText(text);
            ctr--;
            timer = 10000;
          }
        }  
        else
        {
          clean();
          state = kDISPLAY_SCREENSAVER;
          timer = 0;
        }
        break;
        
      case kDISPLAY_SCREENSAVER:
        if (digitalRead(PIN_BUTTON) == HIGH)
        {
            state = kSETUP_DISPLAY_MSG;
        }

        if ( timer == 0 ) 
        {
          bounce();
          timer = 1000;
        }
        else 
          timer--;
        break;
        
      default:
      break;
    }

    return;
  }
  Serial.println(F("new client"));

  client.setTimeout(5000); // default is 1000

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(F("request: "));
  Serial.println(req);

  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    return;
  }
  req = req.substring(addr_start + 1, addr_end);
  client.flush();
  
  String s;
  String answer;
  int pos;
  if (req.indexOf('?')>0) {

    
     //Change url tags to text
     answer=req.substring(req.indexOf('=')+1);
    answer.replace('+',' ');
    //Conver HTML URL Encode to Text:
    //https://www.w3schools.com/tags/ref_urlencode.asp
    answer.replace("%21","\!");
    answer.replace("%22","\"");
    answer.replace("%23","\#");
    answer.replace("%24","\$");
    answer.replace("%25","%");
    answer.replace("%26","&");
    answer.replace("%27","\'");
    answer.replace("%28","\(");
    answer.replace("%29","\)");
    answer.replace("%2B","\+");
    answer.replace("%2C","\,");
    answer.replace("%2D","\-");
    answer.replace("%2E","\.");
    answer.replace("%2F","\/");
    answer.replace("%3A","\:");
    answer.replace("%3B","\;");
    answer.replace("%3C","\<");
    answer.replace("%3D","\=");
    answer.replace("%3E","\>");
    answer.replace("%3F","\?");
    answer.replace("%40","\@");
    answer.replace("%5B","\[");
    answer.replace("%5C","\\");
    answer.replace("%5D","\]");
    answer.replace("%5E","\^");
    answer.replace("%7B","\{");
    answer.replace("%7C","\|");
    answer.replace("%7D","\}");
    answer+=" - ";
    strcpy(text, answer.c_str());
    len=strlen(text);

    // Emit audio msg
    digitalWrite(PIN_MSG_NEW, 0);
    delay(200);
    digitalWrite(PIN_MSG_NEW, 1);
    // and display the message
    state = kSETUP_DISPLAY_MSG;
     
    
    s="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML><style> #header{ min-height: 20px; background-color: #b3ffb3; } #menu{ min-height: 20px; margin-top: 1%; background-color: #999999; } #body{ min-height: 200px; margin-top: 1%; } #footer{ min-height: 20px; margin-top: 1%; background-color: #b3ffb3; } #header, #menu, #body, #footer{ margin-left: 10%; margin-right: 10%; box-shadow: 3px 5px 7px #666666; border: 1px solid black; } @viewport{ zoom: 1.0; width: extend-to-zoom; } @-ms-viewport{ width: extend-to-zoom; zoom: 1.0; } </style> <html lang='en'> <head> <meta name='viewport' content='width=device-width, initial-scale=1'> <title>Bienvenue sur notre nessagerie interne</title> </head> <body> <div id='header'><center><h1>MESSAGERIE</H1></center></div>";
    s+=" <div id='menu'><center><H2>Message transmis!</h2></center></div> ";
    s+="<div id='body'><center><div><div><br/><H3>";
    s+=answer;
    s+="<p><a href='./'>Retour</a></p>";
    s+="</H3></div></div> </center> </div> <div id='footer'> </div> </body></html> ";
    s+"</body></html>\r\n\r\n";
    client.print(s);
    return;
  }  
  else
  {
    
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    s+="<!doctype html> <style> \#header{ min-height: 20px; background-color: \#b3ffb3; } \#menu{ min-height: 20px; margin-top: 1%; background-color: \#999999; } \#body{ min-height: 200px; margin-top: 1%; } \#footer{ min-height: 20px; margin-top: 1%; background-color: \#b3ffb3; } \#header, \#menu, \#body, \#footer{ margin-left: 10%; margin-right: 10%; box-shadow: 3px 5px 7px \#666666; border: 1px solid black; } @viewport{ zoom: 1.0; width: extend-to-zoom; } @-ms-viewport{ width: extend-to-zoom; zoom: 1.0; } </style> <html lang='en'> <head> <meta name='viewport' content='width=device-width, initial-scale=1'> <title>MESSAGERIE</title> </head> <body> <div id='header'><center><h1>Bienvenue sur notre nessagerie interne</H1></center></div> <div id='body'><center><div><div><br> <form action='esp8266'> <br>Saisissez votre message ci-dessous<br><input type='text' maxlength='70' name='max' value=''><br><br> <input type='submit' value='Submit'></form> </div></div> </center> </div> <div id='footer'> </div> </body>";
    s += "</html>\r\n\r\n";
    client.print(s);
   return;
  }

}
