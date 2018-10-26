/*The program takes control over 4-wheel (and correspondingly, 4 DC motors) car,
at the heart of which ESP8266 SoC lies. The RobotDyn NodeM development board was
 used in this project.It is worth to note that one side motors are connected 
in parallel. L293N-based board was used to drive the motors. After uploading the compiled sketch,
you have to uppload html-pages, which can be found in 'data' folder. These
files built an interface, containing four buttons and 'range' html-element,
which allows to move the car forward, backward, turn right or left, and,
finally, to adjust the speed of machine. All the features of data exchange are
implemented via sending GET/POST requests by browser, when an user changes the state
of buttons/range element. On the sketch side, http-server catches the url-strings 
of GET/POST requests, and then it makes decision, what to do. In case of POST request,
additionally body of request is analyzed.*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>

//Here, pins controlling the left side motors are defined
#define In1 D5
#define In2 D6
//Corresponding definitions for right side motors
#define In3 D7
#define In4 D8
//And, finally, pins to enable PWM signal for L293N switches
#define EnA D3
#define EnB D4
//The defines printed bellow serve as semaphors to establish events, preceeding stop of engines.
#define locForward 1
#define locBackward 2
#define locTurnLeft 3
#define locTurnRight 4
//Now, you have to enter credentials corresponding your wifi network
const char* ssid = "***";
const char* password = "***";
ESP8266WebServer server(80);//here we use standard port for the http-server
volatile static uint8_t locationCode = 0; // a semaphor variable, which is used to get know previous function

//Here, one should declare functions used within the sketch.
void goForward();
void goBackward();
void turnRight();
void turnLeft();
void stopEngine1();
void stopEngine2();

void setup() {
  Serial.begin(115200);
  pinMode(In1, OUTPUT);//we toggle controlling pins as outputs
  pinMode(In2, OUTPUT);
  pinMode(In3, OUTPUT);
  pinMode(In4, OUTPUT);
  pinMode(EnA,OUTPUT);
  pinMode(EnB,OUTPUT);
  digitalWrite(In1, LOW);//Initially, all the motors are stayed stopped
  digitalWrite(In2, LOW);
  digitalWrite(In3, LOW);
  digitalWrite(In4, LOW);
  digitalWrite(EnA,HIGH);//It enables all the outputs,i.e. duty cycle is of 100%
  digitalWrite(EnB,HIGH);
  delay(10);
  Serial.println('\n');
  WiFi.begin(ssid, password);//The function initializes wi-fi connection
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ... ");
  int i = 0;//Here, every second the sketch prints number of seconds elapsed before connection is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());//it prints ip-address

  SPIFFS.begin();//the method begin of SPIFFS class initializes a filesystem
  //Here, server catches GET/POST requests transmitting a controlling info
  server.on("/FORWARD", HTTP_GET, goForward);
  server.on("/BACKWARD", HTTP_GET, goBackward);
  server.on("/LEFT", HTTP_GET, turnLeft);
  server.on("/RIGHT", HTTP_GET, turnRight);
  server.on("/STOP", HTTP_GET, stopEngines);
  server.on("/PWMDATA", HTTP_POST, setPWM);//To receive a value of duty cycle, we use body of POST-request
  //Bellow, we handle an absence of a file (uri), which can be actually absent
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "404: not found");
  });
  server.begin();//This runs the server
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();//It's neccessary for correct event loop operation
}

String getContentType(String filename)
{
  //the function determines type of a file, which has to be done for correct work of SPIFFS
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg")) return "image/jpeg";

  return "text/plain";
}

bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  else {
    Serial.println("\tFile not found");
    return false;
  }
}

void goForward() {
  //The code bellow switches both L293N-based driver to push the car forward
  digitalWrite(In1, HIGH);
  digitalWrite(In2, LOW);
  digitalWrite(In3, HIGH);
  digitalWrite(In4, LOW);
  server.sendHeader("Location", "/Forward.html");
  server.send(303);
  locationCode = locForward;//here, we acquire the marking of this function, which will be used in stopEngines function
}

void goBackward() {
  digitalWrite(In1, LOW);
  digitalWrite(In2, HIGH);
  digitalWrite(In3, LOW);
  digitalWrite(In4, HIGH);
  server.sendHeader("Location", "/Backward.html");
  server.send(303);
  locationCode = locBackward;
}

void turnLeft() {
  //Here, the car to render turning left, we have to stop left-side motors, and to provide forward rotation of the right-side wheels
  digitalWrite(In1, HIGH);
  digitalWrite(In2, LOW);
  digitalWrite(In3, LOW);
  digitalWrite(In4, LOW);
  server.sendHeader("Location", "/Left.html");
  server.send(303);
  locationCode = locTurnLeft;
}

void turnRight() {
  digitalWrite(In1, LOW);
  digitalWrite(In2, LOW);
  digitalWrite(In3, HIGH);
  digitalWrite(In4, LOW);
  server.sendHeader("Location", "/Right.html");
  server.send(303);
  locationCode = locTurnRight;
}

void stopEngines() {
  digitalWrite(In1, LOW);
  digitalWrite(In2, LOW);
  digitalWrite(In3, LOW);
  digitalWrite(In4, LOW);
  //to resend appropriate html-page, one have to determine, which html-frame to redraw
  switch (locationCode) {
    case locForward:
      server.sendHeader("Location", "Forward.html");
      server.send(303);
      break;
    case locBackward:
      server.sendHeader("Location", "Backward.html");
      server.send(303);
      break;
    case locTurnLeft:
      server.sendHeader("Location", "Left.html");
      server.send(303);
      break;
    case locTurnRight:
      server.sendHeader("Location", "Right.html");
      server.send(303);
      break;
    default:
      server.sendHeader("Location", "index.html");
      server.send(303);
      break;
  }
}

void setPWM() {
  //Here, we try to catch argument 'plain' from request headers
  if (server.hasArg("plain") == false) { //Check if body received
    server.send(200, "text/plain", "Body not received");
    return;
  }
  //then, the body of POST request is received and, then, converted into a number, corresponding duty cycle 
  String message = server.arg("plain");
  int pwmValue = message.toInt();
  pwmValue = map(pwmValue,0,100,0,1023);//10-bit PWM is used in ESP8266 SoC, so we've to use appropriate scaling
  analogWrite(EnA,pwmValue);//finally, we write the read and transformed 'duty cycle' value into control registers of the ESP8266
  analogWrite(EnB,pwmValue);
  server.send(200, "text/plain", message);
  Serial.println(pwmValue);
}

