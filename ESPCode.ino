
#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>

WiFiMulti WiFiMulti;
SocketIOclient socketIO;
bool needToSendStateChange = false;
String espCurrentChannel = "esp_control";
#define USE_SERIAL Serial

 int myPins[] = {12,13,14,15,16,17};

void IRAM_ATTR isr() {
}
void pwmfadein(){
  }
String lastStatusArraySendedAsString;

//example of eventstring "esp_control/led/14/off"
//with <DeviceName>/<Command>/<Pin>/<State>
 
void handleEventString(String eventString)
{
  String eventChannelMessage = getValue(eventString,'/', 0); 
  int pinNumber = getValue(eventString,'/', 2).toInt();
  if (eventChannelMessage  == espCurrentChannel)
  {
    USE_SERIAL.printf("channel is ok\n");
    if (getValue(eventString,'/', 3)=="on"){
      USE_SERIAL.printf("onéla \n");
      digitalWrite(pinNumber,1);
      }
    else{
      digitalWrite(pinNumber,0);
      }
    }
   else
   {
    USE_SERIAL.printf("wrong channel or pin \n");
    }
  
 }
String getCurrentStatusAsString()
{
  // creat JSON message for Socket.IO (event)
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();

  array.add(espCurrentChannel);

  // add payload (parameters) for the event
  // Hint: socket.on('event_name', ....

  // add payload (parameters) for the event
  JsonObject param1 = array.createNestedObject();
  
  param1["Sun_BUTTON"] = digitalRead(18);
  param1["Moon_BUTTON"] = digitalRead(19);
  param1["Fwd_BUTTON"] = digitalRead(21);
  param1["Hand_BUTTON"] = digitalRead(22);

  
  // JSON to String (serializion)
  String output;
  serializeJson(doc, output);
  return output;
}


  
void sendDataToServer(String output){
    socketIO.sendEVENT(output);

    // Print JSON for debugging
    // USE_SERIAL.println(output);
  }

String getValue(String data, char separator, int index)
  {
    int found = 0;
    int strIndex[] = {0,-1};
    int maxIndex = data.length()-1;
    for(int i=0; i<=maxIndex && found<=index; i++){
      if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0]=strIndex[1]+1;
        strIndex[1]= (i == maxIndex ) ? i+1 : i;

        
        }
      }
      return found>index? data.substring(strIndex[0],strIndex[1]):"";
    }

    
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            USE_SERIAL.printf("[IOc] Disconnected!\n");
            break;
        case sIOtype_CONNECT:
            USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);

            // join default namespace (no auto join in Socket.IO V3)
            socketIO.send(sIOtype_CONNECT, "/");
            break;
        case sIOtype_EVENT:
        {
          //here we have to handle all events 
            char * sptr = NULL;
            int id = strtol((char *)payload, &sptr, 10);
            USE_SERIAL.printf("[IOc] get event: %s id: %d\n", payload, id);
            if(id) {
                payload = (uint8_t *)sptr;
            }
            DynamicJsonDocument doc(1024);
              DeserializationError error = deserializeJson(doc, payload, length);
              if(error) {
                  USE_SERIAL.print(F("deserializeJson() failed: "));
                USE_SERIAL.println(error.c_str());
                return;
            }

            String eventStringContent = doc[0];
            handleEventString(eventStringContent);
            //USE_SERIAL.printf("[IOc] event name: %s\n", eventName.c_str());
            
            // Message Includes a ID for a ACK (callback)
            if(id) {
                // creat JSON message for Socket.IO (ack)
                DynamicJsonDocument docOut(1024);
                JsonArray array = docOut.to<JsonArray>();

                // add payload (parameters) for the ack (callback function)
                JsonObject param1 = array.createNestedObject();
                param1["now"] = millis();

                // JSON to String (serializion)
                String output;
                output += id;
                serializeJson(docOut, output);

                // Send event
                socketIO.send(sIOtype_ACK, output);
            }
        }
            break;
        case sIOtype_ACK:
            USE_SERIAL.printf("[IOc] get ack: %u\n", length);
            break;
        case sIOtype_ERROR:
            USE_SERIAL.printf("[IOc] get error: %u\n", length);
            break;
        case sIOtype_BINARY_EVENT:
            USE_SERIAL.printf("[IOc] get binary: %u\n", length);
            break;
        case sIOtype_BINARY_ACK:
            USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
            break;
    }
}

void setup() {
   
    pinMode(18, INPUT_PULLUP);
    pinMode(19, INPUT_PULLUP);
    pinMode(21, INPUT_PULLUP);
    pinMode(22, INPUT_PULLUP);
    pinMode(12, OUTPUT);
    digitalWrite(12,0);
    pinMode(14, OUTPUT);
    digitalWrite(14,0);
    pinMode(13, OUTPUT);
    digitalWrite(13,0);
    pinMode(15, OUTPUT);
    digitalWrite(15,0);
    pinMode(16, OUTPUT);
    digitalWrite(16,0);
    pinMode(17, OUTPUT);
    digitalWrite(17,0);
    pinMode(23, OUTPUT);
    digitalWrite(23,0);
    pinMode(25, OUTPUT);
    digitalWrite(25,0);
    pinMode(26, OUTPUT);
    digitalWrite(26,0);
    attachInterrupt(18, isr, FALLING);
    attachInterrupt(19, isr, FALLING);
    attachInterrupt(21, isr, FALLING);
    attachInterrupt(22, isr, FALLING);
    
    USE_SERIAL.begin(115200);

    USE_SERIAL.setDebugOutput(true);


      for(uint8_t t = 4; t > 0; t--) {
          USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
          USE_SERIAL.flush();
          delay(1000);
      }


    WiFiMulti.addAP("ssid", "yourpassword");

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }

    String ip = WiFi.localIP().toString();
    USE_SERIAL.printf("[SETUP] WiFi Connected %s\n", ip.c_str());

    //socketIO.begin("192.168.177.1", 8080, "/socket.io/?EIO=4");
    socketIO.begin("192.168.1.123", 8080, "/socket.io/?EIO=5");
    // event handler
    socketIO.onEvent(socketIOEvent);
    lastStatusArraySendedAsString="";
}

unsigned long messageTimestamp = 0;
void loop() {

    socketIO.loop();
    String currentStatusAsString = getCurrentStatusAsString();
    
    if (lastStatusArraySendedAsString!=currentStatusAsString){
        sendDataToServer(currentStatusAsString);
        needToSendStateChange = false;
        lastStatusArraySendedAsString = currentStatusAsString;
        USE_SERIAL.println(lastStatusArraySendedAsString);
      }

      
    /*
     uint64_t now = millis();
     if(now - messageTimestamp > 2000) {
        messageTimestamp = now;

        // creat JSON message for Socket.IO (event)
        DynamicJsonDocument doc(1024);
        JsonArray array = doc.to<JsonArray>();

        // add evnet name
        // Hint: socket.on('event_name', ....
        array.add("photo");

        // add payload (parameters) for the event
        JsonObject param1 = array.createNestedObject();
        param1["STATE_BUTTON1"] = digitalRead(18);

        // JSON to String (serializion)
        String output;
        serializeJson(doc, output);

        // Send event
        socketIO.sendEVENT(output);

        // Print JSON for debugging
        USE_SERIAL.println(output);
    }
    */
}
