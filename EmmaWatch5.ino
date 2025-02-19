
//#define NOBUTTON
//#define DEBUG
// always start with OTA home.  Clicking button remove ota.  open ota through phone (can change wifi)

/* Includes */
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "time.h"
#include <ArduinoJson.h>
#include "codes.h"
#include "watchled.h"
#include "watMode.h"
#include "wmIdle.h"
#include "wmTime.h"
#include "wmDST.h"
//#include <FastLED.h>  //http://fastled.io/docs/3.1/class_c_pixel_view.html

/* Definitions */
#define LED_BUILTIN 0
#define EEPROM_SIZE 4  
#define SLEEP_TIME 700e3 
#define SLEEP_ADJ 236e3 //0300e3 
#define SHUTDOWN_TIME 4000 // press time for shutdown
#define SELECT_TIME 1000 // time button execution
#define DBLCLK_TIME 330 // time button execution
#define AUTO_BAT_KILL 881 // battery level 3.48v (V-1M x V/1.3285M) x 1024
#define AUTO_BAT_LOOP 930 // battery level 3.48v (V-1M x V/1.3285M) x 1024
#define HOSTNAME "emma_watch" //at home emma_watch.zafhome:8095/w

//define pins
#define BUTT_P 5
#define ON_P 14
#define CHARGE_P 12
//const unsigned char LED_PIN=13 ;

const int chargePin = A0;

bool wDST; //Daylight Saving Time
bool AutoKill = false; // kill on low bat; set it in idle when no wifi; turn it off on destruct idle
bool AutoSleep = false;
unsigned long nextSleep;

//led array, led class
emmawatch::watchLed wLed(LED_DELAY, CRGB::Blue); 
CRGB * leds;

const char* ssid = ZAFSID;
const char* password = STAPSK;
const char* hssid = HOTSPOTSID;   //phone hotspot
const char* hspassword = HOTSPOTPSK;

WiFiServer server(8095);
bool WFOn = false;
bool OTAReady;
uint8_t statIP = 255; //stat id last element

//button control
unsigned long t_sav = 0; // button click time
bool t_up = true; // button up down flag
int t_repeat = 0; // number of clicks

// void (*menuAct)(bool) = NULL;
// int menuState = 0;
// int menuMax = 0;
emmawatch::watMode * ModeHdl; // mode of operation pointer to base class

struct tm timeinfo;
time_t realT = 0;
unsigned long realTimeOffset; // keeps the offset from the last time update
struct RTCBuff { //RTC data structure
  int32_t cnt ; // mark rtc sleep
  int32_t sleepT ; // number of naps
  int32_t dat1 ; // time split into two
  int32_t dat2 ; // time
};

//temp
int initChargeG;


/*
/ SETUP   ******************
*/
void setup() {

  const int batLoopLevel = AUTO_BAT_LOOP;

  // nextSleep = RTCmillis();
  nextSleep = millis();

  // time config
  const char* ntpServer = NPTSERVER;
  const long  gmtOffset_sec = NYC_TIME; // ET
  
  RTCBuff rtc_b; 
  realTimeOffset = 0;

  // uint8_t hue = 0;
  // pin setup

  
  pinMode(ON_P, OUTPUT);
  pinMode(CHARGE_P, INPUT_PULLUP);
  pinMode(BUTT_P, INPUT);

  digitalWrite(ON_P, HIGH);
  
  EEPROM.begin(EEPROM_SIZE);
  //eeprom for ip address
  wDST = (EEPROM.read(1) > 0);
  const int daylightOffset_sec = (int) (wDST?1:0) * 3600;
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  ESP.rtcUserMemoryRead(0, (uint32_t * )&rtc_b, sizeof(rtc_b));
  if (rtc_b.cnt == 2022){ //wakeup
    AutoKill = true;
    AutoSleep = true;
    
    realT = rtc_b.dat2;
    realT = rtc_b.dat1 + (realT << 32);
    realT += (time_t) (rtc_b.sleepT * (double) ((SLEEP_TIME + SLEEP_ADJ) / 1e6) + 0.5); //return to start realT
    realT += (millis() + 0.5) / 1000;
    //updateTimeN();
    //bool noCheck = true;
    //if (initialCharge < batLoopLevel) autoKillCheck(noCheck);
    
    /* When the watch wakes up the current goes up because the capasitor is being charged
     * It means that the battery is being stressed and can fail when below 3.6V (910). Therefore 930 is set as a treshold for stopping the sleep
     * (loop treshold) from that point forward the watch stays awake and will die on the batt treshold (850) */

    initChargeG = chargeLevel();;
    // don't kill here if below loop treshold continue and die in loop kill
    if (initChargeG > batLoopLevel) {
        if (!digitalRead(BUTT_P) && AutoSleep) autoSleepNow();
    } else AutoSleep = false;  // stop autosleep. Watch will die by autokill without sleeping again
    
  }

  leds = wLed.init();
  //ModeHdl = new emmawatch::wmIdle(&ModeHdl);

// connect to wifi
  Serial.begin(115200);
  Serial.println("Booting");
  //Serial.println(EEPROM.read(0));
  
  statIP = EEPROM.read(0);
  Serial.println(statIP);
  Serial.println("/EPROM");
  
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);

  // if coming back from sleep get time back and add sleep time x number of naps
  //ESP.rtcUserMemoryRead(0, (uint32_t * )&rtc_b, sizeof(rtc_b));
  if (rtc_b.cnt != 2022){ // connect to wifi
      WiFi.begin(ssid, password);

      if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! ");
        wLed.rInd(CRGB::Red);
        //leds[16]= CRGB::Red;
      } else  WFOn = true;
      
      //ESP.rtcUserMemoryRead(0, (uint32_t * )&rtc_b, sizeof(rtc_b));
      //updateTimeN(); 
      //realT -= (time_t) (rtc_b.sleepT * (double) (SLEEP_TIME / 1e6)); //return to start realT
      //Serial.print("realTAdj:");
      //Serial.println((time_t) (rtc_b.sleepT * (double) (SLEEP_TIME / 1e6)));
      rtc_b.sleepT = 0;
      rtc_b.cnt = 0;
      rtc_b.dat1 = 0;
      rtc_b.dat2 = 0;
      ESP.rtcUserMemoryWrite(0, (uint32_t * )&rtc_b, sizeof(rtc_b));
  }
  nextSleep = millis() + 15000;

  

//      const char* ntpServer = "pool.ntp.org";
//      const long  gmtOffset_sec = -5*3600; // ET
//      const int   daylightOffset_sec = 3600; // DST on
//      struct tm timeinfo;
//
//    
//
//      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
//    if(!getLocalTime(&timeinfo)) Serial.println("Failed to obtain time"); // get time for the first time...

  // uint32_t val = READ_PERI_REG(PERIPHS_IO_MUX + 12);
  // Serial.println("val");
  // Serial.println(val);
  // uint32_t mask = 8;
  // uint32_t maskoff = 14;
  // WRITE_PERI_REG(PERIPHS_IO_MUX + 12, ((mask) | val) );
  // //WRITE_PERI_REG(PERIPHS_IO_MUX + 12, val);
  // val = READ_PERI_REG(PERIPHS_IO_MUX + 12);
  // Serial.println(val);



  if (WFOn) {
    
    wLed.rInd(CRGB::Green);
   // leds[16]= CRGB::Green;
    server.begin();
    OTAStart();
    OTAReady = true;

    realT = getLocalTimeN(&timeinfo);
    
    //realTimeOffset = millis();
    Serial.print("realT:");
    Serial.println(realT);
    
    // struct RTCBuff {
    //   int32_t cnt = 2022;
    //   int32_t dat1 = (uint32_t) (realT & 0xffffffffL);
    //   int32_t dat2 = (uint32_t) (realT >> 32);
    // } rtc_b;

    // rtc_b.cnt = 2022;
    // rtc_b.dat1 = (uint32_t) (realT & 0xffffffffL);
    // rtc_b.dat2 = (uint32_t) (realT >> 32);
    // rtc_b.sleepT = 0;
    // Serial.println(ESP.rtcUserMemoryWrite(0, (uint32_t * )&rtc_b, sizeof(rtc_b)));
    // rtc_b.dat1=0;
    // rtc_b.dat2=0;
    // Serial.println(ESP.rtcUserMemoryRead(0, (uint32_t * )&rtc_b, sizeof(rtc_b)));
    // time_t rtcR = rtc_b.dat1+ (rtc_b.dat2 << 32);
    // Serial.println(rtcR);
    
		// timeinfo = *localtime(&realT);
		// realT -= 24*60*60; //yesterday
    
    if(!realT) Serial.println("Failed to obtain time");


  }
  
  ModeHdl = new emmawatch::wmTime(&ModeHdl);
  //wLed.refreshAll();


  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());
  
}

void loop() {

  //static bool firstClick = true; // ignore turn on click

  //ota and webserv off without wifi
  if (WFOn) { // check if wifi is on
    if (OTAReady) ArduinoOTA.handle();
    WebServ();
  }
  
 #ifndef NOBUTTON
  //button manipulation
  if (digitalRead(BUTT_P)) { //button clicked
    nextSleep = millis() + 15000; // temp
    if (t_up) { //new click
      t_up = false;
      t_sav = millis();
      t_repeat++;
      Serial.print("rep");
      Serial.println(t_repeat);
    }
    else if (t_sav && (millis() > (t_sav + SHUTDOWN_TIME))) {
      Serial.print("off");
      digitalWrite(ON_P, LOW);
      wLed.lInd(CRGB::Red);
      wLed.exitL(CRGB::Red);
    }
    else if (t_sav && (t_repeat && millis() > (t_sav + SELECT_TIME)))
    {
      ModeHdl->exeMode(millis() - t_sav);
      t_repeat = 0;
    }
  }
  else {
    t_up = true;
    if (millis() > (t_sav + DBLCLK_TIME)) { //finished clicking
      if (millis() < t_sav + ModeHdl->timeoutMS)
      {
      //t_sav = 0;
        switch (t_repeat) { // one click option
          case 1:
            Serial.println("click loop");
            ModeHdl->click(); 
            t_sav = millis();  // test time also without click
            break;

          case 2: // double click
            ModeHdl->dblClick();
            t_sav = millis();  // test time also without click
            break;

          case 3: // quick wifi shut down regardless of mode
            turnWF(!WFOn);
            break;

          default:
            break;
        }
      }
      else 
      {
        ModeHdl->timeout();
        t_sav = millis();  // start recounting time
      }
      t_repeat = 0;
    }
  }
 
  #endif
  wLed.rInd((digitalRead(CHARGE_P))? (CRGB::Black) : (CRGB::Red));
  //kill on low bat. if autosleep goes to setup, auto kill should also be called from there before.
  //autoKillCheck(false);
  
  autoKillCheck();

  if (AutoSleep) { // can be added to setup if button is not pressed and returning from sleep.  and to idle timeout.  cancelled from menu. next sleep not required
    if (millis()>nextSleep) {
      autoSleepNow();
    }
  }
}


//*** TIME ********************//
time_t getLocalTimeN(struct tm * info)
{
    uint32_t ms = 5000;
    uint32_t start = millis();
    time_t now;
    

    while((millis()-start) <= ms) {
        time(&now);
        localtime_r(&now, info);
        if(info->tm_year > (2016 - 1900)){
            realTimeOffset = millis();
            return now;
        }
        delay(10);
    }
    return 0;
}

void updateTimeN() {

  if (realT) {
    time_t curTime;
    curTime = realT + (millis() - realTimeOffset + 0.5)/1000;
    //realTimeOffset = millis();
    timeinfo = *localtime(&curTime);

  }
}

// uint32_t RTCmillis() {
//     uint32_t SYS_Time = system_get_rtc_time();
//     uint32_t cal_factor = system_rtc_clock_cali_proc();
//     Serial.print("RTC:");
//     Serial.print(SYS_Time);
//     Serial.print("RTC:");
//     Serial.println(cal_factor);
//     return  SYS_Time / ((cal_factor * 1000) >> 12));
// }

//*** WEB ********************//
//Web server + app API //
//--------------------//
void WebServ() {

	String stringheader, requestbody = "";
	bool gotRequest = false;
	//unsigned long currClock = millis();
  RTCBuff rtc_b;
  
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == 0) break;
        stringheader += c;
        if (c == '\n') {                    // if the byte is a newline character
					  // if the current line is blank, you got two newline characters in a row.
					  // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            gotRequest = true;
            while (client.available()) {
                requestbody += (char)client.read();
              }									
            break;
          } 
          else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } 
        else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
			}
				//add yield + timeout for connection (mills() - currClock) > 10000
      // if ((millis()-currClock)>200) { //7 secs timeout on connection
      //   break;
      // }
      //yield();
      else break;
		}

    //***************************************************************************
    // respond by showing page
    //***************************************************************************
    if (gotRequest) {
      
				// JSON for APP
				if (stringheader.indexOf("GET /Data5020") >= 0) AppMainScreenGET(&client); //app main
				else if (stringheader.indexOf("POST /Prog5020") >= 0) AppPOST(&client, requestbody); // 
			  else if ((stringheader.indexOf("GET /w") >= 0)) WebMainGet(&client, stringheader); //webmain
				else if (stringheader.indexOf("POST /w/ipaddress") >= 0) ipaddPOST(&client, stringheader, requestbody); //change address in epp
    }
    
    // Close the connection
    if (client.connected()) client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");

  // buttons
  }

  if (stringheader.indexOf("GET /wf/on") >= 0) {
      Serial.println("WF on");
      if (!WFOn) turnWF(!WFOn);
  } else if (stringheader.indexOf("GET /wf/off") >= 0) {
    //client.stop();
    Serial.println("WF off");
    if (WFOn) {
      
      ESP.rtcUserMemoryRead(0, (uint32_t * )&rtc_b, sizeof(rtc_b));
      //updateTimeN(); 
      if (realT) { //if time is not set don't increase number of naps
        realT += (millis() - realTimeOffset  + 0.5) / 1000;
        realT -= (time_t) (rtc_b.sleepT * (double) ((SLEEP_TIME + SLEEP_ADJ) / 1e6) + 0.5); //return to start realT
        Serial.print("realTAdj:");
        Serial.println((time_t) (rtc_b.sleepT * (double) (SLEEP_TIME / 1e6)));
        rtc_b.sleepT++;
      }
      rtc_b.cnt = 2022;
      rtc_b.dat1 = (uint32_t) (realT & 0xffffffffL);
      rtc_b.dat2 = (uint32_t) (realT >> 32);
      ESP.rtcUserMemoryWrite(0, (uint32_t * )&rtc_b, sizeof(rtc_b));

      ESP.deepSleep(SLEEP_TIME); //bye bye
    }
  } else if (stringheader.indexOf("GET /wf/rtc") >= 0) {
    
      realT = getLocalTimeN(&timeinfo);//   ESP.rtcUserMemoryRead(0, (uint32_t * )&rtc_b, sizeof(rtc_b));
    // rtc_b.cnt = 100;
    // rtc_b.dat1 = 0;
    // rtc_b.dat2 = 0;
    // rtc_b.sleepT = 0;
    // ESP.rtcUserMemoryWrite(0, (uint32_t * )&rtc_b, sizeof(rtc_b));



    // time_t rtcR = buf.cnt;
    // if (rtc_b.cnt = 100)  rtcR = 100 ;//rtc_b.dat1+ (rtc_b.dat2 << 32);
    // else {
    //   rtc_b.cnt = 100;
    //   rtcR = 2022 ;
    //   ESP.rtcUserMemoryWrite(0,(uint32_t * )&rtc_b, sizeof(rtc_b));
    // }
  }
}// webserver

void WebMainGet(WiFiClient *WebClient, String &strheader) {
  
    WebClient->println("HTTP/1.1 200 OK");
    WebClient->println("Content-type:text/html");
    WebClient->println("Connection: close");
    WebClient->println();
    
    // turns the GPIOs on and off
    
    // Display the HTML web page
    WebClient->println("<!DOCTYPE html><html>");
    WebClient->println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    WebClient->println("<link rel=\"icon\" href=\"data:,\">");
    // CSS to style the on/off buttons 
    // Feel free to change the background-color and font-size attributes to fit your preferences
    WebClient->println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
    WebClient->println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
    WebClient->println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
    WebClient->println(".button2 {background-color: #77878A;}</style></head>");
    
    // Web Page Heading
    WebClient->println("<body><h1>Watch Web Server</h1>");
    WebClient->println("<p>Charge level: " + String(chargeLevel())  + "</p>");
    // Display current state, and ON/OFF buttons for GPIO 5  
    updateTimeN(); 
    WebClient->println("<p> Time is " +  String(timeinfo.tm_hour) + String((timeinfo.tm_min<10)?":0":":") + String(timeinfo.tm_min) + String((timeinfo.tm_sec<10)?":0":":") + String(timeinfo.tm_sec) + "</p>");
    //WebClient->println("<p> RTC Time is " + String(system_get_rtc_time()) + "</p>");
    
    WebClient->println("<p> Init Charge: " + String(initChargeG)  + "</p>");
    WebClient->println("<p> last exit time: " + String(EEPROM.read(2)) + ":" + String(EEPROM.read(3)) + "</p>");
    RTCBuff rtc_b;
    ESP.rtcUserMemoryRead(0, (uint32_t * )&rtc_b, sizeof(rtc_b));

    time_t rtcR = rtc_b.cnt;
    // if (rtc_b.cnt = 100)  rtcR = 100 ;//rtc_b.dat1+ (rtc_b.dat2 << 32);
    // else {
    //   rtc_b.cnt = 100;
    //   rtcR = 2022 ;
    //   ESP.rtcUserMemoryWrite(0,(uint32_t * )&rtc_b, sizeof(rtc_b));
    // }


    //WebClient->println("<p> RTC is " + String(rtc_b.cnt) + ":" + String(rtcR) + "</p>");
    WebClient->println("<p> RTC is " + String(rtcR) + "</p>");

    WebClient->println("<p> Wifi is " + String(WFOn?"On":"Off") + "</p>");
    
    WebClient->println("<p><a href=\"/wf/rtc\"><button class=\"button\">RTC</button></a></p>");
    // If the output5State is off, it displays the ON button       
    if (!WFOn) {
      WebClient->println("<p><a href=\"/wf/on\"><button class=\"button\">ON</button></a></p>");
    } else {
      WebClient->println("<p><a href=\"/wf/off\"><button class=\"button button2\">Reset WIFI</button></a></p>");
    }
    if (statIP == 255) WebClient->println("<p>Static IP Address is not set</p>");
    else WebClient->println("<p>Static IP element: " + String(statIP)  + "</p>");
    WebClient->println("<form action=\"/w/ipaddress\" method=\"post\"><label for=\"ipadd\">Static IP Address (between 5 and 255):</label>");
    WebClient->println("<input type=\"number\" id=\"ipadd\" name=\"ipadd\" min=\"5\" max=\"255\">");
    WebClient->println("<input type=\"submit\"></form>");
    
    // //back and refresh
		WebClient->println("<script>");
		//go back and refresh
		if (strheader.indexOf("GET /wf") >= 0 || strheader.indexOf("/w/") >= 0) {  //} || (strheader.indexOf("GET /sysoff5020") >= 0)) {
			//WebClient->println("if (" + String((strheader.indexOf("GET /Read_Now5020") >= 0)?"true":"false") +") {"); // is read clicked
			Serial.print("Refresh from");
			Serial.println(strheader);

			WebClient->println("sessionStorage.setItem(\"WRef\", \"y\");"); 
			WebClient->println("window.history.back();"); // }");
		}
		else if (strheader.indexOf("GET /w") >= 0) {
			WebClient->println("if (sessionStorage.getItem(\"WRef\") == \"y\") {"); 
			WebClient->println("sessionStorage.setItem(\"WRef\", \"n\");"); 
			WebClient->println("setTimeout(function(){ location.reload(true); }, 1500);}"); // refresh after 1.5 secs
		}
		WebClient->println("</script>");

    // WebClient->println("</body></html>");
    
    // The HTTP response ends with another blank line
    WebClient->println();
}
	
	// info to app main screen JSON
void AppMainScreenGET(WiFiClient *AppClient)
{		
  StaticJsonDocument<100> jdoc;
  
  int aveCharge = 0;
  for (int i=0; i < 10; i++) aveCharge += analogRead(chargePin);
  jdoc["AveCharge"] = aveCharge/10;
  jdoc["Time"] = String(timeinfo.tm_hour) + String((timeinfo.tm_min<10)?":0":":") + String(timeinfo.tm_min) + String((timeinfo.tm_sec<10)?":0":":") + String(timeinfo.tm_sec);
  jdoc["DeviceOn"] = F("On");
  

  AppClient->println(F("HTTP/1.1 200 OK"));
  AppClient->println(F("Content-type:application/json"));
  AppClient->println(F("Connection: close"));
  AppClient->print(F("Content-Length: "));
  AppClient->println(measureJsonPretty(jdoc));
  AppClient->println();

  serializeJsonPretty(jdoc, *AppClient);
}

// change static ipaddress
void ipaddPOST(WiFiClient *AppClient, String &strheader, String &POSTBody) {

    int postlength = POSTBody.length();
    int newStatIP = 0;
    
    if (postlength > POSTBody.indexOf("=")){
        String s1 =	POSTBody.substring(POSTBody.indexOf('=')+1,postlength);
        newStatIP = s1.toInt(); 
    }


        Serial.print("Post:");
        Serial.println(postlength);

        Serial.println(POSTBody);

    // AppClient->println("HTTP/1.1 200 OK");
    // AppClient->println("Content-type:text/plain");
    // AppClient->println("Connection: close");
    // AppClient->println();
    // AppClient->println("postOK");
    // AppClient->println();
    WebMainGet(AppClient, strheader);
    
    if (newStatIP && statIP != newStatIP) {
      statIP = newStatIP;
      EEPROM.write(0, statIP);
      EEPROM.commit();
      // Serial.print("eeprom:");
      // Serial.println(EEPROM.read(0));
    }

    // Serial.print("ipadd:");
    // Serial.println(newStatIP);
}

	// receives read command and frequency from APP
void AppPOST(WiFiClient *AppClient, String &POSTBody)
{
    int wprog = 0;
    String postResp;

    // if (POSTBody.indexOf("Read") >= 0) {
    //   if (m_active) readSeq();
    // }
    // else if 
    Serial.print("Post:");
    Serial.println(POSTBody);
    if (POSTBody.indexOf("Prog") >= 0) {
				String s1 =	POSTBody.substring(POSTBody.indexOf('?')+1,POSTBody.indexOf('?')+2); // + '\0';
				wprog = s1.toInt(); 
        Serial.print("Prog:");
        Serial.println(wprog);
        postResp = "postProgOK";
    }
    else if (POSTBody.indexOf("TimeRef") >= 0) {
        realT = getLocalTimeN(&timeinfo);
        postResp = "postRefreshOK";
    }
    else postResp = "postNoneOK";
    AppClient->println("HTTP/1.1 200 OK");
    AppClient->println("Content-type:text/plain");
    AppClient->println("Connection: close");
    AppClient->println();
    AppClient->println(postResp);
    AppClient->println();
    
    if (AppClient->connected()) AppClient->stop();
    
    if (wprog==1) wLed.testLights();
}

//*** NET ********************//
//ota
void OTAStart() {
  
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

// void Wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info){
//   Serial.println("Disconnected from WiFi access point");
//   Serial.print("WiFi lost connection. Reason: ");
//   Serial.println(info.wifi_sta_disconnected.reason);
//   Serial.println("Trying to Reconnect");
//   WiFi.begin(ssid, password);
// }

//home wifi on off
void turnWF(bool WFState) {

  WFOn = WFState;
  if (WFOn) {
    
    WiFi.forceSleepWake();
    //WiFi.mode(WIFI_STA);
    //WiFi.setHostname(HOSTNAME);
    if (statIP != 255) { //if stat ip is set
      IPAddress ip_stat(192, 168, 11, 200);  
      IPAddress ip_subnet(255, 255, 255, 0);  
      IPAddress ip_gateway(192, 168, 11, 1);
      ip_stat[3] = statIP;
      WiFi.config(ip_stat, ip_gateway, ip_subnet);
      WiFi.setHostname(HOSTNAME);
    }
    else WiFi.config(0,0,0);

    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! ");
      leds[16]= CRGB::Red;
    }
    else {
      leds[16]= CRGB::Green;
      server.begin();
      OTAStart();
      OTAReady = true;
    }
  }
  else {
    leds[16]= CRGB::Red;
    server.close();
    server.stop();
    //WiFi.disconnect();
    WiFi.forceSleepBegin();
    OTAReady = false;
    
  }
  wLed.refreshAll();
}

//hotspot on
void hotspotConnect() {
  
  IPAddress ip_stat(192, 168, 200, 200);  
  IPAddress ip_subnet(255, 255, 255, 0);
  if (statIP != 255) ip_stat[3] = statIP; //if ip is stored on flash use it

  if (WFOn) {
    WiFi.disconnect();
    delay(2000);
    WFOn = false;
    OTAReady = false;
  }
  
  WiFi.begin(hssid, hspassword);
  
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! ");
    leds[16]= CRGB::Red;
  }
  else {
    leds[16]= CRGB::Yellow;
    wLed.refreshAll();
    IPAddress * ip_gateway = new IPAddress(WiFi.gatewayIP()); 
    for (int i = 0; i < 3; i++) {
      ip_stat[i] = (* ip_gateway)[i];      
    }
    
    WiFi.disconnect();
    delay(5000);
    WiFi.config(ip_stat, *ip_gateway, ip_subnet);
    delete[] ip_gateway;
    WiFi.begin(hssid, hspassword); 
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! ");
      leds[16]= CRGB::Red;
    }
    else {
      server.begin();
      leds[16]= CRGB::Green;
      WFOn = true;
    }
  }
  wLed.refreshAll();
  delay(500);
}

int chargeLevel() {
  
    int aveCharge = 0;
    for (int i=0; i < 10; i++) aveCharge += analogRead(chargePin);

    return aveCharge/10;
}

void autoKillCheck() {
  
//void autoKillCheck(bool NoKillCheck) {
  //if (NoKillCheck || (AutoKill && chargeLevel()<AUTO_BAT_KILL)) {
  if (AutoKill && chargeLevel()<AUTO_BAT_KILL) {

      if (!leds) leds = wLed.init();
      updateTimeN();
      const uint8_t t1 = timeinfo.tm_hour ;
      const uint8_t t2 =  timeinfo.tm_min;
      EEPROM.write(2,t1);
      EEPROM.write(3,t2);
      EEPROM.commit();

      AutoSleep = false; // avoid sleeping , just in case...
      digitalWrite(ON_P,LOW); // die

      wLed.allBlack();
      leds[5] = CRGB::Red;
      leds[6] = CRGB::Red;
      leds[9] = CRGB::Red;
      leds[10] = CRGB::Red;
      wLed.refreshAll(20);
      delay(10000); //enough time to die !!!
  }
} 

void autoSleepNow() {
  
  RTCBuff rtc_b;
  ESP.rtcUserMemoryRead(0, (uint32_t * )&rtc_b, sizeof(rtc_b));
  //updateTimeN(); 
  if (realT) { //if time is not set don't increase number of naps
    realT += (millis() - realTimeOffset + 0.5) / 1000; // 0.5 for round
    realT -= (time_t) (rtc_b.sleepT * (double) ((SLEEP_TIME + SLEEP_ADJ) / 1e6) + 0.5); //return to start realT
    rtc_b.sleepT++;
  }
  rtc_b.cnt = 2022;
  rtc_b.dat1 = (uint32_t) (realT & 0xffffffffL);
  rtc_b.dat2 = (uint32_t) (realT >> 32);
  ESP.rtcUserMemoryWrite(0, (uint32_t * )&rtc_b, sizeof(rtc_b));

  ESP.deepSleep(SLEEP_TIME); //bye bye

}
