/*
   Mkr1000-Servo control web responder

   Goal in life:
      Control servos via http get requests.

   Written By - Scott Beasley 2016.
*/

#include <SPI.h>
#include <WiFi101.h>
#include <Servo.h>
#include <string.h>
#include <ctype.h>

void ServoOpen (String pin);
void ServoClose (String command);
void ServoSet (String command);
void ServoRead (String command);
void ServoCtrlReset (void);
void sendReturnResp (WiFiClient client, String ret_data);
bool request_is (String request, String reqdata);

// Define your Wifi info here
#define SSID "YOUR SSID"
#define PASSWD "YOUR WIFI PASSWORD"

#define BOARDNAME "mkr1000_01"

// Define to get extra info from the Serial port
#define DEBUG

// Globals
WiFiServer server (80);

// Allow for at least 8 servos.
struct userservos {
   Servo servo;
   int pin;
} userservos[8];
int servosinuse = 0;

String json_ret = "";

void setup (void)
{
   servosinuse = 0;
   int status = WL_IDLE_STATUS;
   int i = 0;
   Serial.begin (9600);
   while (!Serial) {if (i++ >= 1024) break;}

   // check for the presence of the shield:
   if (WiFi.status ( ) == WL_NO_SHIELD) {
      Serial.println ("WiFi shield not present");
      // don't continue:
      while (true);
   }

   // Mark all servo slots as free on start.
   for (int i = 0; i < 9; i++) {
      userservos[i].pin = -1;
   }

   // Connect to the WiFi network
   Serial.print ("\nConnecting to: ");
   Serial.println (SSID);

   while (status != WL_CONNECTED) {
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      status = WiFi.begin(SSID, PASSWD);

      // wait 10 seconds for connection:
      delay (10000);
   }

   Serial.println ("\nWiFi connected");

   // Start the tcp listener
   server.begin ( );

   // Print the IP address
   IPAddress ip = WiFi.localIP ( );
   Serial.print ("Use this URL to connect: ");
   Serial.print ("http://");
   Serial.print (ip);
   Serial.println ("/");
}

void loop ( )
{
   bool endofline = false;
   json_ret = "";

   WiFiClient client = server.available ( );

   if (client) {
      #ifdef DEBUG
        Serial.println ("Client connected");
      #endif
      String currentLine = "";
      while (client.connected ( )) {
         if (client.available ( )) {
            char c = client.read ( );
            if (c == '\n') {
               endofline = true;
            } else if (c != '\r') {
               currentLine += c;
            }

            if (endofline) {
               endofline = false;
               String parms = "";

               #ifdef DEBUG
                 Serial.println ("<" + currentLine + ">");
               #endif

               // Copy the parms sent in the request.
               int ndx1 = currentLine.indexOf ('?');
               int ndx2 = currentLine.indexOf (' ', ndx1);

               #ifdef DEBUG
                 Serial.println ("<" + String (ndx1) + ">");
                 Serial.println ("<" + String (ndx2) + ">");
               #endif

               if (ndx1 >= 0) {
                  parms = currentLine.substring (ndx1 + 1, ndx2);
               }

               #ifdef DEBUG
                 if (parms.length ( ) > 0)
                    Serial.println ("<" + parms + ">");
               #endif

               if (request_is (currentLine, "/servoOpen")){
                  ServoOpen (parms);
               } else if (request_is (currentLine, "/servoClose")) {
                  ServoClose (parms);
               } else if (request_is (currentLine, "/servoSet")) {
                  ServoSet (parms);
               } else if (request_is (currentLine, "/servoRead")) {
                  ServoRead (parms);
               } else if (request_is (currentLine, "/servoCtrlReset")) {
                  ServoCtrlReset ( );
                  json_ret = "{\n\t\"return_code\": 0\n}";
               } else {
                  break;
               }

               #ifdef DEBUG
                 Serial.println ("<=" + json_ret + "=>");
               #endif

               sendReturnResp (client, json_ret);
               currentLine = "";
               break;
            }
         }
      }

      // Close the connection when done
      client.stop ( );
      #ifdef DEBUG
         Serial.println ("Client disconnected normally");
      #endif
   }
}

bool request_is (String request, String reqdata)
{
   return (request.indexOf (reqdata) != -1 ? 1 : 0);
}

void sendReturnResp (WiFiClient client, String ret_data)
{
   String return_hdr = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: application/json\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Connection: close\r\n"
                       "Content-Length: " + String (ret_data.length ( )) +
                       "\r\n\r\n" + ret_data +"\r\n";
   client.print (return_hdr);
}

void ServoOpen (String pin)
{
	int i, return_code = 0;

	//convert ascii to integer
	int pinNumber = pin.charAt (0) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber < 0 || pinNumber > 9) return_code = -1;

	servosinuse++;
   if (servosinuse > 8) {
	   return_code = -3; // All servos inuse.
	}

	for (i = 0; i < 9; i++) {
      if (userservos[i].pin == -1) {
         // Zap any old attachment.
         userservos[i].servo.detach ( );

         userservos[i].pin = pinNumber;
         userservos[i].servo.attach (pinNumber);
         break;
		}
	}

  // Return the servo array element index used.
  json_ret = "{\n\t\"data_value\": \"" + String (i) +
             "\",\n\t\"return_code\": " + return_code + "\n}";
}

void ServoClose (String command)
{
    int return_code = 0;

    //convert ascii to integer
    int indexNumber = command.charAt(0) - '0';
    //Sanity check to see if the element numbers are within limits
    if (indexNumber < 0 || indexNumber > 9) return_code = -1;

    userservos[indexNumber].pin = -1;
    servosinuse--;
    userservos[indexNumber].servo.detach ( );

    json_ret = "{\n\t\"return_code\": " + String (return_code) + "\n}";
}

void ServoSet (String command)
{
    int ret = -1;
    //convert ascii to integer
    int indexNumber = command.charAt (0) - '0';
    //Sanity check to see if the pin numbers are within limits
    if (indexNumber< 0 || indexNumber > 8) {
       ret = -1;
    } else {
       String value = command.substring (2);
       ret = 0;
       userservos[indexNumber].servo.write (value.toInt ( ));
    }

    json_ret = "{\n\t\"return_code\": " + String (ret) + "\n}";
}

void ServoRead (String command)
{
    int return_code = 0;
    int raw_val = 0;

    //convert ascii to integer
    int indexNumber = command.charAt (0) - '0';
    //Sanity check to see if the pin numbers are within limits
    if (indexNumber < 0 || indexNumber > 8) return_code = -1;

    raw_val = userservos[indexNumber].servo.read ( );
    json_ret = "{\n\t\"data_value\": \"" + String (raw_val) +
               "\",\n\t\"return_code\": " + String (return_code) + "\n}";
}

void ServoCtrlReset (void)
{
   servosinuse = 0;
   // Mark all servo slots as free and close any open.
   for (int i = 0; i < 9; i++) {
      if (userservos[i].pin != -1) {
         userservos[i].servo.write (90); // Try and stop if moving.
         userservos[i].servo.detach ( );
      }

      userservos[i].pin = -1;
   }
}
