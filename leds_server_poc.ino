#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define NUM_PIXELS 10
#define PIN_PIXELS 14

const char* ssid = "xxx";
const char* password = "xxx";

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, PIN_PIXELS, NEO_GRB + NEO_KHZ800);

void HandleRoot();
void HandleCommand();
void FlashLeds();
void TurnOffLeds();
void HandleNotFound();
uint8_t HSLToRGB(struct HSL hsl);
float HueToRGB(float v1, float v2, float vH);


struct RGB
{
  unsigned char R;
  unsigned char G;
  unsigned char B;
};

struct HSL
{
  int H;
  float S;
  float L;
};
ESP8266WebServer server(80);

// what to do when a client requests "http://<IP>"
void HandleRoot() {
    server.send(200, "text/plain", "hello from esp8266!"); 
}

// What to do when a client requests "http://<IP>/command"
// This function doesn't care whether it is a GET or POST at the moment, 
// it will treat them the same just to make things easier.
void HandleCommand() {
    int i;
    for (uint8_t i=0; i<server.args(); i++) {
        if (server.argName(i) == "flash") {
            if (server.arg(i) == "on") {
                FlashLeds();
                server.send(200, "text/plain", "LEDs turned on");
            } else {
                TurnOffLeds();
                server.send(200, "text/plain", "LEDs turned off");
            }
        }
    }
    server.send(404, "text/plain", "Command not found");
}

void FlashLeds() {
    // Flash LEDs or something then exit this func so that the webserver carries on running.
    Serial.println("LEDs on.");
    int dt = 50;
    for (uint8_t i=0; i<server.args(); i++) {
      if (server.argName(i) == "type") {
        
        switch(server.arg(i)[0]) {
          case 'a':
            colorWipe(pixels.Color(255, 0, 0), dt); // Red
            break;
          case 'b':
            colorWipe(pixels.Color(0, 255, 0), dt); // Green
            break;
          case 'c':
            colorWipe(pixels.Color(0, 0, 255), dt); // Blue
            break;
          case 'd':
            theaterChase(pixels.Color(255, 0, 0), dt); // Red
            break;
          case 'e':
            theaterChase(pixels.Color(0, 255, 0), dt); // Green
            break;
          case 'f':
            theaterChase(pixels.Color(255, 0, 255), dt); // Green
            break;
          case 'g':
            rainbowCycle(dt);
            break;
          case 'h':
            rainbow(dt);
            break;
          case 'i':
            theaterChaseRainbow(dt);
            break;
        }
      }
    }
    for (int i = 0; i < NUM_PIXELS; i++) {
      pixels.setPixelColor(i, HSLToRGB((HSL) {i * 360 / NUM_PIXELS, 0.75f, 0.8f}));
      pixels.show();
      delay(dt);
    }
}

void TurnOffLeds() {
    // Flash LEDs or something then exit this func so that the webserver carries on running.
    for (int i = NUM_PIXELS - 1; i >= 0; i--) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      pixels.show();
    }
    Serial.println("LEDs off.");
}

// If the route is not found then default to printing out what was sent to this server to aid in debug.
void HandleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);}

void setup(void) {
    Serial.begin(115200);
    pixels.begin();
    WiFi.begin(ssid, password);
    Serial.println("Setting up.");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(".");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");
    }

    /* 
    //I left this in commented out just to show this method as well.
    server.on("/inline", []() {
        server.send(200, "text/plain", "this works as well");
    });
    */
 
    // Setup server routes.
    server.on("/", HandleRoot);
    server.on("/command", HandleCommand);
    server.onNotFound(HandleNotFound);
    server.begin();
    Serial.println("HTTP server started");
}

void loop(void) {
    server.handleClient();
}

void changeColor(uint32_t c) {
  for(uint16_t i=0; i<pixels.numPixels(); i++) {
    pixels.setPixelColor(i, c);
  }
  pixels.show();
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<pixels.numPixels(); i++) {
    pixels.setPixelColor(i, c);
    delay(wait);
    pixels.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel((i+j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, c);    //turn every third pixel on
      }
      pixels.show();

      delay(wait);

      for (uint16_t i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      pixels.show();

      delay(wait);

      for (uint16_t i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

float HueToRGB(float v1, float v2, float vH)
{
  if (vH < 0)
    vH += 1;

  if (vH > 1)
    vH -= 1;

  if ((6 * vH) < 1)
    return (v1 + (v2 - v1) * 6 * vH);

  if ((2 * vH) < 1)
    return v2;

  if ((3 * vH) < 2)
    return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

  return v1;
}

uint8_t HSLToRGB(struct HSL hsl) {
  struct RGB rgb;

  if (hsl.S == 0)
  {
    rgb.R = rgb.G = rgb.B = (unsigned char)(hsl.L * 255);
  }
  else
  {
    float v1, v2;
    float hue = (float)hsl.H / 360;

    v2 = (hsl.L < 0.5) ? (hsl.L * (1 + hsl.S)) : ((hsl.L + hsl.S) - (hsl.L * hsl.S));
    v1 = 2 * hsl.L - v2;

    rgb.R = (unsigned char)(255 * HueToRGB(v1, v2, hue + (1.0f / 3)));
    rgb.G = (unsigned char)(255 * HueToRGB(v1, v2, hue));
    rgb.B = (unsigned char)(255 * HueToRGB(v1, v2, hue - (1.0f / 3)));
  }

  return pixels.Color(rgb.R, rgb.G, rgb.B);
}
