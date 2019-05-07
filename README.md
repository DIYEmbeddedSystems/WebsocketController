# WebsocketController

This repo contains both the source code for a Wall-E inspired robot, and the web browser-based remote controller for the robot. Communication between the robot and the remote is based on the Websocket protocol.

# The Robot
The robot is inspired from Pixar's *Wall-E* robot. The purpose of this project is to mimic as many of Wall-E's movements, controlled remotely via a ligthweight smartphone-based remote.

## Mechanics

### Body
The robot body is a box made of hand sawn 5mm plywood.

### Tracks
The tracks are from a cheap kit from Geekcreit brand, named "Plastic Track + Driving Wheel + Bearing Wheel Set Accessory For Robot Car Chassis - G".

### Head & neck
The head is made from a glued stack of 10mm plywood, routed on my homebrew CNC machine.
Head/body articulation is made of 2 servos and a 10mm plywood neck.

### Arms
Shoulders, arms, wrists and hands are assembled from hand sawn 2mm-thick aluminium board, 25x25mm aluminium square profile, 20mm-diamater wood stick, and 5mm-thick *Dibond*-style aluminium/plastic composite board.

### Electro-mechanical actuators
Moving parts are actuated by:
- a geared motor (continuous tracks)
- a servomotor, for all other joints: head pan/tilt, shoulders, wrist, hand
    + MG996R for higher torque (shoulders, head pan)
    + MG90S for smaller & light-torque joints


## Electronics
The robot is composed of the following electronics modules:
- power management
    + 18650 Li-Ion battery
    + Battery charging & voltage regulation module
        * provide regulated 5V supply to actuators and 3V supply to microcontroller
- main controller
    + Wemos board with an ESP8266 (80MHz, 32bit) microcontroller, onboard 4MB SPI flash
- peripherals
    + Wemos Motor shield, based on the TB6612FNG H-bridge controller (at default address 0x30)
        * drives 2 geared motors (CHIHAI, 6V DC, GM25-370)


    + PCA9685 16-channel PWM controller (at default address 0x40)
        * head pan (MG996R) and tilt (MG90S),
        * 2 arms x (shoulder elevation (MG996R), shoulder rotation (MG90S), hand open/close (MG90S))

## Software
### Code
The ESP8266 runs the WebsocketControl.ino Arduino sketch. In brief, this sketch runs the following tasks in parallel (cooperative, run-to-completion scheduling):
- connect to Wifi access point; if no AP found, open a new closed-network access point
- accept and handle OTA update requests; 
- upon HTTP request, serve webpage and other files from SPI flash;
- accept incoming websocket connections; while a connection is up:
    + periodically report current robot state (telemetry);
    + whenever a telecommand is received, parse command message, update accordingly robot commanded state;
- update control loops, and command actuators accordingly.

### Data 
The SPI flash on the Wemos board stores the (compiled) Arduino sketch, as well as several data files (version controlled in the data/ folder).

Use the ESP8266FS tool (https://www.instructables.com/id/Using-ESP8266-SPIFFS/) to upload these files to the Wemos' SPI flash.

Content of these files is detailed in the next chapter.

# UI

The user interface runs on a smartphone, tablet, or PC, in a browser. This is why it is developed in HTML / Javascript.

The UI is designed as a single web page, featuring:
- configuration parameters
- joysticks

# Interactions & protocol
TBD

## Message sequence
TBD

## Payload structure
TBD

# To-do list

- validate Websocket interaction between remote & robot
- add motor shield control
- let the children play
