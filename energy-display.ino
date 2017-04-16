/******************************************************/
/*!
      File: energy-display
      Author: Anthony Pearce | makehacktech
      Start Date: 10 April 2017
      Version 1.03 Date: 16 Apr 2017
      Licence: MIT Licence
      
      This sketch connects to WiFi, downloads live PV power generation data from pvoutput.org and displays it on an LED dial
      
  Compatible Hardware:
    NodeMCE V1.0 (ESP-8266 12E)
    Ring of 24 RGB LEDS, 5050, WS2812
    470 Ohm resistor IN on LED ring to Pin D1 / GPIO 5
    220 uF cap across VIN/GND of LED ring
    USB power to NodeMCU
      
  Version History
    1.0 Program structure start
    1.01 Added startup display, partial interval light
    1.02 Cleanup code for publishing
    1.03 (github edit) move startup lights before wifi connection

  Bugs
    Nil known

  Thanks to Adafruit for the Neopixel library and the ESP8266 community for the ESP8266Wifi library.

*/
/******************************************************/
// Libraries
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>

// Pin definitions
#define PIN 5         // Arduino GPIO Pin connected to "IN"
#define NUMPIXELS 24  // Number of LEDs in the LED ring

// Setup the display object
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Constant definitions
// WIFI connection details - replace these with your WIFI network login details
const char* ssid     = "<your network ssid>"; // The ssid of your wifi network
const char* password = "<your network password>"; // the password for your wifi network
const int maxtimeout = 5000; // timeout if no response after this many ms

// pvoutput.org URL and API details. Replace these with your own pvoutput.org account details
const char* host = "pvoutput.org"; // pvoutput URL
const char* service = "/service/r2/getstatus.jsp"; // We are using the getstatus service of pvoutput
const char* sid = "<your pvoutput sid>"; // sid
const char* key = "<your pvoutput API key>"; // API key

// pvoutput data get constants
//const char* delim = ","; // data delimeter from pvoutput
const int nvals = 8; // Number of delimiters between values returned from pvoutput
const int bindex = 2; // Index of the delimeter before the power value
//const int aindex = 3; // Index of the delimeter after the power value

// Display constants
// Take care if increasing LED brightness - they draw a lot of power
const int normal = 10; // Sets how bright the normal LEDs are - 0 to 255
const int low = 5; // Sets how bright the low LEDs are - 0 to 255
const int maxpow = 3000;  // Maximum power output that can be displayed, in Watts
const int powint = 200;   // The amount of power, in Watts, that each LED represents
const int blinkdur = 300; // duration of a standard blink

// Timing constants
const int delaytime = 300000; // Time in ms to wait before fetching new data
const int shortdelay = 3000; // Time delay in ms before connecting to pvoutput
const int dotdelay = 500; // Time between status dots for wifi connection on serial

// Variable definitions
int ind[8];  // indeces of the delimeters - locations of the delimeters in the string returned from pvoutput
int powgen = 0; // Power in Watts from pvoutput live interval data 
int numpix = 0; // Number of LEDs to turn on to display power
String url; // for combining the host, service, key and sid to make the URL

// String names - variables to store the data from pvoutput as a String
// Note that only spowgen is used in this sketch. refer to pvoutput.org for data definitions
String line;
//String sdate;
//String stime;
//String senergen;
String spowgen;
//String senercon;
//String spowcon;
//String seff;
//String stemp;
//String svolts;

/******************************************************/
// FUNCTIONS
/******************************************************/
//-----------------------------------------------------------------------------------------
// Function 1 Title
//-----------------------------------------------------------------------------------------
//int function1() {
//  function1 content here    
//}


/******************************************************/
// SETUP
/******************************************************/
void setup() {

 // Display setup
  pixels.begin(); // This initializes the NeoPixel library.

  // Startup display animation
  pixels.clear(); // blank out prior display
  pixels.setPixelColor(6, pixels.Color(0,0,normal)); 
  pixels.setPixelColor(14, pixels.Color(0,0,normal)); 
  pixels.setPixelColor(22, pixels.Color(0,0,normal)); 
  pixels.show(); // Send updated pixel colours to the hardware.
  delay(blinkdur);
  pixels.clear(); // blank out prior display
  delay(blinkdur);
  pixels.setPixelColor(2, pixels.Color(0,0,normal)); 
  pixels.setPixelColor(10, pixels.Color(0,0,normal));
  pixels.setPixelColor(18, pixels.Color(0,0,normal)); 
  pixels.show(); // Send updated pixel colours to the hardware.
  delay(blinkdur);
  pixels.clear(); // blank out prior display
  delay(blinkdur);
  pixels.setPixelColor(6, pixels.Color(0,0,normal)); 
  pixels.setPixelColor(14, pixels.Color(0,0,normal)); 
  pixels.setPixelColor(22, pixels.Color(0,0,normal));
  pixels.show(); // Send updated pixel colours to the hardware.
  delay(blinkdur);
  // Clear display
  pixels.clear(); // blank out prior display
  pixels.show(); // Send updated pixel colours to the hardware.
      
  // Enable serial comms
  Serial.begin(115200);
  delay(10);

  // Connect to wifi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //Set ESP8266 to be a wifi client
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(dotdelay);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // prints the local IP address assigned to your nodeMCU

  // Create the URL for the request from pvoutput
  url = service;
  url += "?sid=";
  url += sid;
  url += "&key=";
  url += key;
      
  // Short delay before starting to get data from pvoutput
  delay(shortdelay);

}
  
/******************************************************/
// MAIN
/******************************************************/
void loop() {


  // Start the HTTP connection
  Serial.print("connecting to ");
  Serial.println(host);

  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // Request the data from pvoutput
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > maxtimeout) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    //String line = client.readStringUntil('\r');
    line = client.readStringUntil('\r');
    Serial.print(line);
  }
  // Indicate that data download is finished
  Serial.println();
  Serial.println("closing connection");

  // line will now equal the last line of data sent, which is the response with pvoutput live interval data
  // Extract index of the delimeters from the last line sent
  //Serial.println();
  ind[0] = line.indexOf(','); // index of the first delimeter  
  //Serial.println(ind[0]);
  for (int i=1; i < nvals; i++) {
    ind[i] = line.indexOf(',',ind[i-1]+1); // next comma index starting at position after last delimeter index 
    //Serial.println(ind[i]);
    }

  // Extract the power from the data string
  spowgen = line.substring(ind[bindex]+1, ind[bindex+1]); // extract the string between the delimeters
  powgen = spowgen.toInt(); // convert the String type data to an integer

  // Print the power value to be displayed to serial
  Serial.println();
  Serial.print("Live Power Output: ");
  Serial.print(powgen);
  Serial.println();
 
  // Test code to test display of fixed powgen value
  //powgen = 3000;

  // Limit the display to maxpow
  if(powgen > maxpow){
    powgen = maxpow;
  }

  // Clear all LEDs prior to displaying the new value
   pixels.clear(); // blank out prior display
   pixels.show(); // This sends the updated pixel instructions to the hardware.
   delay(10); // short delay after clearing before new display
   
  // Display the power on the LED dial
  numpix = powgen / powint; // integer division will truncate to full intervals only
  // Setup the display for each pixel
  for(int i=0;i<numpix;i++){
    Serial.println(i);
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0,normal,0)); // Green
  }
 
  // Setup the display for the case where there are no full pixels to show
  if(numpix==0){
    if(powgen==0){
    // Power is zero
    pixels.setPixelColor(0, pixels.Color(0,0,low)); // Blue to show system is on but value is zero
    } else {
      // Power is less than powint
      pixels.setPixelColor(0, pixels.Color(low,low,0)); // Yellow to show low power generation  
    }
  }

  pixels.show(); // This sends the updated pixel colours to the hardware.

  delay(delaytime); // time interval to delay until the next value read from pvoutput

}
