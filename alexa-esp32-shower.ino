/// ESP32 shower control with Alexa 

#include <WiFi.h>
#define ESPALEXA_ASYNC
#include <Espalexa.h>
#include "time.h"

Espalexa espalexa;
AsyncWebServer server(80);

// define the GPIO connected with Relays and switches
#define RelayPin1 5  //D5
#define TouchPin 4   //D4
#define SwitchPin 3   //D3

#define wifiLed    2   //D2
int ledState = 0;
boolean relayState = false;
boolean switchState, lastSwitchState, touchSensor = true;
unsigned long onTime = 0;
unsigned int touchReadCount = 0;
unsigned int maxShowerMinutes = 20;
// WiFi Credentials
const char* ssid = "NETGEAR";
const char* password = "smoothunicorn2344";

// device names
String Device_1_Name = "shower";

// prototypes
boolean connectWifi();

//callback functions
void firstLightChanged(uint8_t brightness);

unsigned long currentTime = millis();
unsigned long totalOnTime = 0;

// Previous time
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
// Variable to store the HTTP request
String header, showerTimes = "Showertimes here";
struct tm timeinfo, ontimeinfo, boottime;

boolean wifiConnected = false;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3600*6;
const int   daylightOffset_sec = 3600;
void getLocalTime();
void printLocalTime();
float lastShowerDuration =0;
float totalShowerTime =0;
float totalShowerTimeMonth[12];

#define maxShowTime 100

struct totalShowerTimeMonth {
    char count; 
    float duration;
} totalShowerTimesMonth[12]; 

struct ShowTime {
    struct tm startTime; 
    float duration;
} showTimes[maxShowTime]; 

unsigned int showTimePtr = 0;
#define maxStringBuff 500 
char timeStringBuff[maxStringBuff]; //500 chars should be enough

void buildShowTimes() {     
         //getLocalTime(); 
         char timeString[100]; //100 chars should be enough
         char durStringBuff[50]; // 50 chars should be enough
         strftime(timeString, sizeof(timeString), "%a, %b %d %Y %H:%M:%S", &showTimes[showTimePtr].startTime);
         String value = String(showTimes[showTimePtr].duration);
         sprintf(durStringBuff," - %s min\n", value.c_str());

         strcat(timeString, durStringBuff);

         // Check for buffer overflow
         if ((strlen(timeStringBuff) + strlen(timeString)) < (maxStringBuff -3)) {
            strcat(timeStringBuff, timeString);
         } else { 
            strcpy(timeStringBuff, timeString);
         }
         Serial.print("length of str is ");
         Serial.println(strlen(timeStringBuff));
}
char * getShowTimes() {
       return timeStringBuff;
}

//our callback functions
void firstLightChanged(uint8_t brightness)
{
  //Control the device
  if (brightness == 255 or relayState == false)
    {
      digitalWrite(RelayPin1, HIGH);
      Serial.println("Device1 ON");
      Serial.println(brightness);
      relayState = true;
      onTime= millis();
      getLocalTime();
      ontimeinfo = timeinfo;
      showTimes[showTimePtr].startTime = timeinfo;
    }
  else
  {
      digitalWrite(RelayPin1, LOW);
      Serial.println("Device1 OFF");
      Serial.println(brightness);
      relayState = false;
      Serial.print("shower time: ");
      float seconds = (millis() - onTime)/(float) 60000;
      Serial.println(seconds);
      Serial.print("showTimesPtr = ");
      Serial.println(showTimePtr);

      // onTimes[onTimePtr++].duration = seconds;
      getLocalTime();
      totalShowerTime = totalShowerTime + seconds;
      totalShowerTimesMonth[timeinfo.tm_mon].duration += seconds;
      totalShowerTimesMonth[timeinfo.tm_mon].count ++;

      char nextMonth = (timeinfo.tm_mon + 1) % 12;
      
      totalShowerTimesMonth[nextMonth].duration =0;
      totalShowerTimesMonth[nextMonth].count =0;
      
      //lastShowerDuration = seconds;
      showTimes[showTimePtr].duration = seconds;
      buildShowTimes();

      // rotate the buffer if needed 
      showTimePtr = (showTimePtr+1) % maxShowTime;

      //postResult();
      

  }
}


// connect to wifi – returns true if successful or false if not
boolean connectWifi()
{
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state) {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }
  return state;
}

void addDevices(){
  // Define your devices here.
  espalexa.addDevice(Device_1_Name, firstLightChanged); //simplest definition, default state off

  espalexa.begin();
}

void setup()
{
  Serial.begin(115200);

  pinMode(RelayPin1, OUTPUT);
  
  pinMode(wifiLed, OUTPUT);

  pinMode(TouchPin, INPUT_PULLUP);
  pinMode(SwitchPin, INPUT_PULLUP);

  
  switchState = lastSwitchState = digitalRead(SwitchPin);

  //During Starting all Relays should TURN OFF
  digitalWrite(RelayPin1, LOW);
  

  // Initialise wifi connection
  wifiConnected = connectWifi();

  if (wifiConnected)
  {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      int args = request->args();
      for(int i=0;i<args;i++){
           Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
      }
      request->send(200, "text/plain", "Here are the things you can try \n/month - monthly stats\n/info - all shower records\n/touchOff - turn off touch sensor\n/touchOn - turn on touch sensor [default]");
    });
    server.on("/clearInfo", HTTP_GET, [](AsyncWebServerRequest *request){
      int args = request->args();

      strcpy(timeStringBuff, "");
      request->send(200, "text/plain", "cleared shower data");
    });
     server.on("/clearMonth", HTTP_GET, [](AsyncWebServerRequest *request){
      int args = request->args();
      for(int m=0;m<12;m++){
         totalShowerTimesMonth[m].duration =0;
         totalShowerTimesMonth[m].count =0;
      }
      showTimePtr = 0;
      
      request->send(200, "text/plain", "cleared monthly data");
    });
     server.on("/touchOff", HTTP_GET, [](AsyncWebServerRequest *request){
      getLocalTime();
      Serial.println(&timeinfo, "%B %d %Y %H:%M:%S");
      touchSensor = false;
      request->send(200, "text/plain", "touch sensor has been turned off");
    });
    server.on("/touchOn", HTTP_GET, [](AsyncWebServerRequest *request){
      getLocalTime();
      Serial.println(&timeinfo, "%B %d %Y %H:%M:%S");
      touchSensor = true;
      request->send(200, "text/plain", "touch sensor has been turned on");
    });
    server.on("/uptime", HTTP_GET, [](AsyncWebServerRequest *request){

      char timeString[100]; //100 chars should be enough
      char durStringBuff[50]; // 50 chars should be enough
      strftime(timeString, sizeof(timeString), "Last booted on %a, %b %d %Y %H:%M:%S", &boottime);
      request->send(200, "text/plain", timeString);
    });
    server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request){

         Serial.println(timeStringBuff);

         request->send(200, "text/plain", timeStringBuff);

     });
     server.on("/month", HTTP_GET, [](AsyncWebServerRequest *request){
         getLocalTime();
         char buffer1[100];
         char bigBuffer[500] = "Monthly shower usage\n\n";
         for (int m =0; m < 12; m++) { 
             String value = String(totalShowerTimesMonth[m].duration);
             String avg = String(totalShowerTimesMonth[m].duration / totalShowerTimesMonth[m].count);
             
             sprintf(buffer1," %2d - (%d)  %s min  avg %smin\n",m +1, totalShowerTimesMonth[m].count, value.c_str(), avg);             
             strcat(bigBuffer,buffer1);
             Serial.print(m);
             Serial.print(" ");
             Serial.print(totalShowerTimesMonth[m].count);
             Serial.print(" ");
             Serial.println(totalShowerTimesMonth[m].duration);

         }
         request->send(200, "text/plain", bigBuffer);

      });
     server.onNotFound([](AsyncWebServerRequest *request){
      if (!espalexa.handleAlexaApiCall(request)) //if you don't know the URI, ask espalexa whether it is an Alexa control request
      {
        //whatever you want to do with 404s
        request->send(404, "text/plain", "Not found");
      }
    });
    espalexa.begin(&server); //give espalexa a pointer to your server object so it can use your server instead of creating its own

    addDevices();
  }
  else
  {
    Serial.println("Cannot connect to WiFi. So in Manual Mode");
    delay(1000);
  }
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime();
  boottime = timeinfo;

}

void loop()
{
   if (WiFi.status() != WL_CONNECTED)
  {
    //Serial.print("WiFi Not Connected ");
    digitalWrite(wifiLed, ledState); //Turn off WiFi LED
    ledState = !ledState;
  }
  else
  {
    //Serial.print("WiFi Connected  ");
    digitalWrite(wifiLed, HIGH);
    //Manual Switch Control
    //WiFi Control
    if (wifiConnected){
      espalexa.loop();
      delay(1);
      //httpserverloop();
    }
    else 
    {
      wifiConnected = connectWifi(); // Initialise wifi connection
      if(wifiConnected){
      addDevices();
      }
    }
  }
  //Serial.println(touchRead(T0));  // get value using T0

  if (relayState) { 
      Serial.print("Shower Time: "); 
      Serial.print((millis() - onTime)/(float)60000);
      Serial.println(" min");
  }
  if (touchRead(T0) < 30 && touchSensor) { //was 70

      Serial.print("detected from touch sensor: ");
      Serial.println( touchRead(T0));

      //debounce touch sensor
      if (touchReadCount++ > 3 ) { 
         Serial.println("change relay state from touch sensor: ");
         touchReadCount = 0;
         if (relayState) { 
             firstLightChanged(0);
         } else { 
             firstLightChanged(255);
         }
      }
      
      delay(100);
  } else { 
     touchReadCount = 0;
  }
  switchState = digitalRead(SwitchPin);
  if (switchState != lastSwitchState) { 
      lastSwitchState = switchState;
         if (relayState) { 
             firstLightChanged(0);
         } else { 
             firstLightChanged(255);
         }
         delay(500);

  }

  // Set Max Shower Time Just in case 
  if (relayState && (millis() - onTime)> 60000 * maxShowerMinutes ) { 
        firstLightChanged(0);
        Serial.println("change relay state from max shower time");

  }
  delay(150);
  //button1.check();
  
}

void getLocalTime(){
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  
}
/*   Attempt to push results to an external site 
void postResult() {
        HTTPClient http;

        USE_SERIAL.print("[HTTP] begin...\n");
        // configure traged server and url
        //http.begin("https://www.howsmyssl.com/a/check", ca); //HTTPS
        http.begin("http://yahoo.com"); //HTTP

        USE_SERIAL.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

         // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                USE_SERIAL.println(payload);
            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
}
*/
void printLocalTime(){

  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println(&timeinfo, "%B %d %Y %H:%M:%S");
  
  /*
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  
  Serial.println(timeWeekDay);
  */
  Serial.println();
} 
