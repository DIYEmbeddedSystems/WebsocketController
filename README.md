# WebsocketController

This repo contains both the source code for a Wall-E inspired robot, and the web browser-based remote controller for the robot. Communication between the robot and the remote is based on the Websocket protocol.

A build log and more documentation about the project is available at [Hackaday.io](https://hackaday.io/project/165989-wall-e).

# The Robot
The robot is inspired from Pixar's *Wall-E* robot. The purpose of this project is to mimic as many of Wall-E's movements, controlled remotely via a ligthweight smartphone-based remote.

## Mechanics

### Body
A first version of the robot body was made of hand sawn 5mm plywood.
A second version was laser cut in 5mm plywood.

### Tracks
The tracks are from a cheap kit from Geekcreit brand, named "Plastic Track + Driving Wheel + Bearing Wheel Set Accessory For Robot Car Chassis - G".

### Head & neck
The first head was made from a glued stack of 10mm plywood, routed on my homebrew CNC machine ; second revision made of laser cut 5mm plywood.

### Arms
Shoulders, arms, wrists and hands are assembled from hand sawn 2mm-thick aluminium board, 25x25mm aluminium square profile, 20mm-diameter aluminium tube, 18mm-diameter wood stick, and the hand is made of 5mm *Dibond*-style aluminium/plastic composite board.

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
 - front camera controller: a ESP32-CAM module

## Software
### Code
The ESP8266 runs the WebsocketControl.ino Arduino sketch. In brief, this sketch runs the following tasks in parallel (cooperative, run-to-completion scheduling):
- connect to Wifi access point; if no AP found, open a new closed-network access point
- accept and handle OTA (over-the-air) firmware update requests; 
- upon HTTP request, serve the webpage and other files from SPI flash file system;
- accept incoming websocket connections; then while a connection is up:
    + periodically report current robot state (telemetry);
    + whenever a telecommand is received, parse command message, and update accordingly the robot commanded state;
- update actuator control loops, and command actuators accordingly.

#### SlowServo library
Fast servo movements at certain joints may cause a large torque which would certainly damage the somehow weak gears & motors. Moreover, if collision avoidance or range limits are not well implemented (which is likely during the debugging phase), collisions may occur and cause mechanical damage. Moreover, high speed/high torque movements cause current surges, and may cause brown-outs or battery current limit protections.
For these reasons, I have implemented a "slow servo" library: a library for controlling a servo via a classic PWM interface, but with additional open-loop control that limits its range of motion and angular speed.

### Data 
The SPI flash on the Wemos board stores the (compiled) Arduino sketch, as well as several data files (version controlled in the data/ folder).
Larger files are compressed with gz, in order to spare both flash space (currently nearly 1MB is used) and UI load time. Your browser transparently unzips the files for you.
Use the ESP8266FS tool (https://www.instructables.com/id/Using-ESP8266-SPIFFS/) to upload these files to the Wemos' SPI flash.

Content of these files is detailed in the next chapter.

# UI
The user interface runs on a smartphone, tablet, or PC, in a browser. This is why it is developed in HTML / Javascript.

The UI is designed as a single web page, featuring:
- configuration parameters
- joysticks

# Interactions & protocol
...To be continued...

![UML interaction diagram](https://www.plantuml.com/plantuml/img/PP1D4i8m24RtEGKNy09TEDrswqmF40csqHewG7HwUrF7_gXBXiVxW5hDY-Nxu5oh970uGjjKO9onXojFQX5lhcsM1d9waDW7K1IY12DhjCXfuOge0ktvkHf-aHEVsGf3AS0GrO0lfR0LK_ScTkWIlAVA5k0ncpSM49ywsiRcPR_qc4KexIEsg_8Ol17452BECYrQP0anTSFqVzN6ELUNZFA-5m00)
(diagram generated with https://www.planttext.com/)

## Message sequence
...To be continued...

## Message payload structure
...To be continued...

