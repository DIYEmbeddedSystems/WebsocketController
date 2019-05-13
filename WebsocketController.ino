/**
 * @file WebsocketController.ino
 * 
 * @brief The Arduino main program for HTML & websocket-based robot telecommand
 * 
 * @author Etienne Hamelin (etienne.hamelin@gmail.com ; www.github.com/etiennehamelin)
 * 
 * @licence 
 * You may freely use, compile, link, copy, adapt, distribute, this software,
 * as long as you keep this author & license header.
 * Happy hacking!
 * 
 */


#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WebSocketsServer.h>

#include "SlowServoPWMExpander.h"

#include "credentials.h"


ESP8266WiFiMulti wifiMulti;        // ESP8266WiFiMulti simplifies handling of multiple access points
ESP8266WebServer server(80);       // web server on port 80, serves files
WebSocketsServer webSocket(81);    // websocket server on port 81, handles telecommand/telemetry

File fsUploadFile;                 // a File variable to temporarily store the received file


const char *ssid = "AP-WallE"; // The name of the Wi-Fi network that will be created
const char *password = "eve";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "walle";           // A name and a password for the OTA service
const char *OTAPassword = "walle";
const char* mdnsName = "walle"; // Domain name for the mDNS responder


SlowServo servos[] = {
  SlowServo(0, 5), // Right shoulder elevation
  SlowServo(1, 5), // Right shoulder extension
  SlowServo(3, 15), // Right hand
  SlowServo(8, 15), // Head pan
  SlowServo(9, 15), // Head tilt
  SlowServo(12, 15), // Left hand
  SlowServo(14, 5), // Left shoulder extension
  SlowServo(15, 5), // Left shoulder elevation
};

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");
  Serial.println("Application " __FILE__ " compiled " __DATE__ " @ " __TIME__ ".\n");

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  Serial.println("Wifi ready");
  startOTA();                  // Start the OTA service
  Serial.println("OTA ready");
  startSPIFFS();               // Start the SPIFFS and list all contents
  Serial.println("SPIFFS ready");
  startWebSocket();            // Start a WebSocket server
  Serial.println("Websocket server ready");
  startMDNS();                 // Start the mDNS responder
  Serial.println("mDNS responder ready");
  startServer();               // Start a HTTP server with a file read handler and an upload handler
  Serial.println("HTTP server ready");

  Serial.println("\n\n\n\nGo!\n\n\n");
}

/*__________________________________________________________LOOP__________________________________________________________*/

void loop() {
  webSocket.loop();                           // check for incoming websocket events
  server.handleClient();                      // check for http server requests
  ArduinoOTA.handle();                        // check incoming over-the-air updates
  serialCommand();                            // check serial comm commands
  report();                                   // periodically report state
  webSocketReport();
  controlLoop();
}

void controlLoop() {
  static long int next_ms = 0;
  const long int period_ms = 10;
  
  if (millis() >= next_ms) {
    next_ms += period_ms;
    for (int i = 0; i < sizeof(servos)/sizeof(servos[0]); i++) {
      servos[i].update();
    }
  }
}

/*__________________________________________________________SERIAL__________________________________________________________*/

void serialCommand() {
  static String str = "";

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      parseCommand((uint8_t *)str.c_str(), str.length());
      str = "";
    } else {
      str += c;
    }
  }
}


void parseCommand(uint8_t *payload, size_t len) {
  payload[len] = '\0'; /* hack to ensure strtol does not parse beyond packet length  */
  char *s = (char *)payload;
  long int x;
  /* handle commands of the form '@123,45,-67,8,90,12,-34' */
  if (*s++ != '@') return;
  for (int i = 0; i < sizeof(servos) / sizeof(servos[0]); i++) {
    x = strtol(s, &s, 10);
    Serial.printf("[%d] := %d, ", i, x);
    servos[i].write(x);
    if (*s == ',') {
      ++s;
    } else {
      break;
    }
  }
  Serial.printf("\n");
}

void report() {
  static long int next_ms = 0;
  const long int period_ms = 3000;
  
  if (millis() >= next_ms) {
    next_ms += period_ms;

    String s = "#";
    for (int i = 0; i < sizeof(servos) / sizeof(servos[0]); i++) {
      s += servos[i].read();
      s += ",";
    }
    s += "t:";
    s += millis();
    s += "\r\n";
    Serial.print(s);
  }
}

void webSocketReport() {
  static long int next_ms = 0;
  const long int period_ms = 100;
  
  if (millis() >= next_ms) {
    next_ms += period_ms;
    
    String s = "#";
    for (int i = 0; i < sizeof(servos) / sizeof(servos[0]); i++) {
      s += servos[i].read();
      s += ",";
    }
    s += "t:";
    s += millis();
    s += "\r\n";
    webSocket.broadcastTXT(s.c_str(), s.length());
  }
}

/*__________________________________________________________SETUP_FUNCTIONS__________________________________________________________*/

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started\r\n");

  wifiMulti.addAP(AP_SSID, AP_PASSWORD);   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("Samuel Barber", "litu7816");   // add Wi-Fi networks you want to connect to

  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  if (WiFi.softAPgetStationNum() == 0) {     // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("with local IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s [%s]\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", "");
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists
  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

/*__________________________________________________________SERVER_HANDLERS__________________________________________________________*/

void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/" + path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
      if (SPIFFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
        SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
      hexdump((unsigned char *)payload, len);
      parseCommand(payload, len);
      break;
    default:
      Serial.printf("Unhandled websocket event [%d].\n", type);
      break; 
  }
}



/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "kB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
