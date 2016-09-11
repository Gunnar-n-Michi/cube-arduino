// #define STRIP_PIN 53
#define PIXELSPERCUBE 8
//These are the speeds of the animations
#define COPYSPEED 100
#define FADESPEED 32

// #define irMaxDistance 60

///DIAGNOSIS or PRINT_IR_INFO
#define NORMAL_MODE                   0
#define DIAGNOSIS_MODE                1
#define IDLE_MODE                     2
#define SET_AMP_MODE                  3
#define PRINT_IR_INFO_MODE            4
#define PLOT_PIEZOS_MODE              5
#define PLOT_IR_MODE                  6
#define PLOT_IR_WITH_RAINBOW_MODE     7
#define SCREENSAVER_MODE              8
#define FAKE_CUBES_MODE               9

#define MODE NORMAL_MODE

#define ACTIVATE_IR true   //Applies to both diagnosis and normal mode

#define DEBUG

///////////////////////////////////////////////////////////////////////////////////////////DON*T FORGET TO SET GRID SIZES!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define GRID_SIZE_X 2
#define GRID_SIZE_Y 2
#define UP    1
#define RIGHT 2
#define DOWN  3
#define LEFT  0

#define REED1DIRECTION RIGHT
#define REED2DIRECTION UP

#include <Adafruit_NeoPixel.h>//Order matters here. Since the arduino IDE (1.0.6) doesn't allow including libraries in libraries we have to include the neopixels here, before the cube_class.
#include "cube_class.h"

const int NUMBEROFCUBES = GRID_SIZE_X * GRID_SIZE_Y;
const int NUMBEROFPIXELS = NUMBEROFCUBES * PIXELSPERCUBE;

const int piezoThreshold = 400;
const int dimScaleFactor = 4;
const int IRthreshold = 250;
const int irMaxDistance = 60;

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
// Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBEROFPIXELS, STRIP_PIN, NEO_GRB + NEO_KHZ800);

Cube_class cubes[] = {
      //(cubeNumber, ir, piezo, reed, reed, ledPin, tiltSwitch, IRthreshold,) invertedTilt, stripOffset
  
#if MODE == FAKE_CUBES_MODE
  //Fake layout. This is since we might use different boards with less pins. And we don't use the pins so just pick some.
  Cube_class(0, A0, A1, 1, 2, 3, 4, IRthreshold),
  Cube_class(1, A0, A1, 1, 2, 3, 4, IRthreshold),
  Cube_class(2, A0, A1, 1, 2, 3, 4, IRthreshold),
  Cube_class(3, A0, A1, 1, 2, 3, 4, IRthreshold),
  Cube_class(4, A0, A1, 1, 2, 3, 4, IRthreshold),
  Cube_class(5, A0, A1, 1, 2, 3, 4, IRthreshold),
  Cube_class(6, A0, A1, 1, 2, 3, 4, IRthreshold),
  Cube_class(7, A0, A1, 1, 2, 3, 4, IRthreshold)
#else

  //Handmade layout
  // Cube_class(0, A8, A0, 2, 3, 46, 22, 55),
  // Cube_class(1, A9, A1, 4, 5, 47, 23, 55),
  // Cube_class(2, A10, A2, 6, 7, 48, 24, 55),
  // Cube_class(3, A11, A3, 8, 9, 49, 25, 55),
  // Cube_class(4, A12, A4, 10, 11, 50, 26, 55),
  // Cube_class(5, A13, A5, 12, 13, 51, 27, 55),
  // Cube_class(5, A5, 27, 25, 23, 40, 60),
  // Cube_class(6, A6, 32, 30, 28, 48, 60, true)

  //Shield layout!!!
  // Cube_class(0, A8, A9, 13, 12, 6, 9, IRthreshold),
  // Cube_class(1, A10, A11, 11, 10, 7, 8, IRthreshold),
  // Cube_class(2, A12, A13, 4, 3, 2, 17, IRthreshold),
  // Cube_class(3, A14, A15, 15, 14, 16, 18, IRthreshold),
  Cube_class(0, A0, A1, 24, 26, 53, 51, IRthreshold),
  Cube_class(1, A2, A3, 30, 28, 40, 42, IRthreshold),
  Cube_class(2, A4, A5, 38, 32, 34, 50, IRthreshold),
  Cube_class(3, A6, A7, 48, 46, 52, 49, IRthreshold)
#endif
};


int debugColorCycleProgress = 0;
int debugColorCycleCurrentColor = 0;
int irReading = 0;
char inByte;
unsigned long startRequestSendTime;
bool shouldSendStartRequest = false;

uint8_t rainbowCounter = 0;

void setup() {
  Serial.begin(9600);
  // strip.begin();
  // strip.show(); // Initialize all pixels to 'off'

  // THINGS TO BE AWARE OF:
  // The wiring and electronics in this project leaves a lot to ask for. I, Gunnar, take full responsibility for this...
  // The ethernet cables doesn't seem to allow enough current to keep the vcc from doing a voltage drop when lighting the LEDs.
  // This means that the ir sensors will get confused. When the voltage drop on the power line occurs, the analog value 
  // from the sensors will make a jump to a higher value.
  //
  // The data line on the led strips results in noise for the piezo. It's observable if you measure with oscillocope
  // and turn on/off the "Continuously dim the leds" part of the main loop.
  //
  // The piezos seem to make a weird spike in the start of the sketch. Now this is handled by ignoring the piezos until a
  // few seconds has passed since startup. It would be nice to know the reason to this behaviour though. The piezos should also be connected
  // through a voltage division circuit which is not the case.

  //if fake mode we have no cubes connected, so don't try to interface them.
  if(MODE != FAKE_CUBES_MODE){
    for (int i = 0; i < NUMBEROFCUBES; ++i)
    {
      cubes[i].init();
    }
  }
  delay(500);

  if(true || MODE){//Don't print if normal mode
    sendMessage(F("Starting Musical Cubes Arduino Firmware.")); 
  }else{
    // delay(2000);
    // printNeighbours();
   // showCalibrationValues();
  }
}
void loop() {

  if(MODE == SET_AMP_MODE){
    set_amp();
    return;
  }

  if(MODE == FAKE_CUBES_MODE){
    fakeSerial();
  }else{
    handleSerial();
  }

  //CHECK SENSORS!!!!

  //Modes (functions) that dosn't loop the cubes "internally"
  for (int i = 0; i < NUMBEROFCUBES; ++i)
  {
    switch(MODE){
      case IDLE_MODE:
        break;
      case NORMAL_MODE:
        readCube(i);
        dimLeds(i);
        break;
      case DIAGNOSIS_MODE:
        cubeDiagnosis(i);
        dimLeds(i);
        break;
      case PRINT_IR_INFO_MODE:
        measureIR(i);
        break;
      case SCREENSAVER_MODE:
        cubes[i].setColorFromRainbowByte(rainbowCounter++);
        delay(10);
        break;
    }
  }
  // Modes that loops the cubes internally
  switch (MODE){
    case PLOT_PIEZOS_MODE:
      plotPiezos();
      break;
    case PLOT_IR_MODE:
      plotIr();
      break;
    case PLOT_IR_WITH_RAINBOW_MODE:
      plotIr();
      debugColorCycle();
  }


  if(shouldSendStartRequest){
    if(millis() > startRequestSendTime){
      shouldSendStartRequest = false;
      sendStartRequest();
    }
  }

}

void fakeSerial(){
  //Have we received any stuff?????? If so, fake connected cubes!
  if(Serial.available()>= 2){
    if(Serial.read() == '#'){
      uint8_t newLine;
      uint8_t command = readChar();
      uint8_t affectedCube = readChar();
      if(command == '/'){//Trigger cube
        if(Cube_class::someCubeIsBusy){//Don't do stuff when some cube is busy
          return;
        }
        // delay(6);
        uint8_t effect = readChar();
        newLine = readChar();
        if(newLine == '\n'){
          cubes[affectedCube].triggerStamp = millis();
        }
      }else if(command == 92){//Cubeoffverified
        newLine = readChar();
        if(newLine == '\n'){
          cubes[affectedCube].cubeOffVerified = true;
        }
      }else if(command == '['){//Recording started
        //someCubeIsBusy is already switched in sendRecordRequest
        newLine = readChar();
        if(newLine == '\n'){
          cubes[affectedCube].isWaitingToRecord = false;
          cubes[affectedCube].isRecording = true;
          cubes[affectedCube].recordStamp = millis();
        }
      }else if(command == ']'){//Recording finished
        newLine = readChar();
        if(newLine == '\n'){
          cubes[affectedCube].isRecording = false;
          Cube_class::someCubeIsBusy = false;
        }
      }else if(command == '!'){//Recording timeout
        newLine = readChar();
        if(newLine == '\n'){
          cubes[affectedCube].isWaitingToRecord = false;
          cubes[affectedCube].isRecording = false;//Just in case!!
          Cube_class::someCubeIsBusy = false;
        }
      }else if(command == '*'){//Copying confirmed
        uint8_t affectedCube2 = readChar();
        newLine = readChar();
        if(newLine == '\n'){
          // cubes[affectedCube].copyingFinished = true;
          // cubes[affectedCube2].copyingFinished = true;
          cubes[affectedCube].setCopyingState(COPYINGCONFIRMED);
          cubes[affectedCube2].setCopyingState(COPYINGCONFIRMED);
          cubes[affectedCube2].recordStamp = millis(); //Update the stamp of the receiving cube
          // Serial.print("copying finished: "); Serial.print(affectedCube); Serial.println(affectedCube2);
        }
        // sendStartRequest();
      }else if(command == '?'){//Setting color
        // delay(6);
        if(Cube_class::someCubeIsBusy){//Don't do stuff when some cube is busy (copying)
          return;
        }
        uint8_t effect = readChar();
        newLine = readChar();
        if(newLine == '\n'){
        }
      }else if(command == 't'){//Handshake
        newLine == readChar();
          if(affectedCube == 'a' || affectedCube == 'A'){
            sendMessage("a");
            resetToInitState();
            // Should we perhaps clear the serial buffer?
            // Serial.clear();
          }
      }
    }
  }
}

void handleSerial(){
  //Have we received any stuff?????? If so, do cool shit!
  if(Serial.available()>= 2){
    if(Serial.read() == '#'){
      uint8_t newLine;
      uint8_t command = readChar();
      uint8_t affectedCube = readChar();
      if(command == '/'){//Trigger cube
        if(Cube_class::someCubeIsBusy){//Don't do stuff when some cube is busy
          return;
        }
        uint8_t effect = readChar();
        newLine = readChar();
        if(newLine == '\n'){
          uint32_t color = Wheel(effect);
          // cubes[affectedCube].setCubeColor(color);
          // Trigger white
          cubes[affectedCube].setCubeColor(255,255,255);
          //Depack the colors
          int
          r = (uint8_t)(color >> 16),
          g = (uint8_t)(color >>  8),
          b = (uint8_t)color;
          cubes[affectedCube].setMyColor(r/dimScaleFactor, g/dimScaleFactor, b/dimScaleFactor);
          cubes[affectedCube].triggerStamp = millis();
        }
      }else if(command == 92){//Cubeoffverified
        newLine = readChar();
        if(newLine == '\n'){
          cubes[affectedCube].cubeOffVerified = true;
          cubes[affectedCube].setMyColor(0, 0, 0);
        }
      }else if(command == '['){//Recording started
        newLine = readChar();
        if(newLine == '\n'){
          cubes[affectedCube].isWaitingToRecord = false;
          cubes[affectedCube].isRecording = true; 
          cubes[affectedCube].recordStamp = millis();
        }
      }else if(command == ']'){//Recording finished
        newLine = readChar();
        if(newLine == '\n'){
          cubes[affectedCube].isRecording = false;  
          cubes[affectedCube].setCubeColor(0,255,0); 
          Cube_class::someCubeIsBusy = false;
        }
      }else if(command == '!'){//Recording timeout
        newLine = readChar();
        if(newLine == '\n'){
          cubes[affectedCube].isWaitingToRecord = false;
          cubes[affectedCube].isRecording = false;//Just in case!!
          Cube_class::someCubeIsBusy = false;
        }
      }else if(command == '*'){//Copying confirmed
        uint8_t affectedCube2 = readChar();
        newLine = readChar();
        if(newLine == '\n'){
          cubes[affectedCube].setCopyingState(COPYINGCONFIRMED);
          cubes[affectedCube2].setCopyingState(COPYINGCONFIRMED);
          cubes[affectedCube2].recordStamp = millis(); //Update the stamp of the receiving cube
        }
        // sendStartRequest();
      }else if(command == '?'){//Setting color
        if(Cube_class::someCubeIsBusy){//Don't do stuff when some cube is busy (copying)
          return;
        }
        uint8_t effect = readChar();
        newLine = readChar();
        if(newLine == '\n'){
          uint32_t color = Wheel(effect);
          //Depack the colors
          int
          r = (uint8_t)(color >> 16),
          g = (uint8_t)(color >>  8),
          b = (uint8_t)color;
          cubes[affectedCube].setMyColor(r/dimScaleFactor, g/dimScaleFactor, b/dimScaleFactor);
          // cubes[affectedCube].setCubeColor(r/dimScaleFactor, g/dimScaleFactor, b/dimScaleFactor);
        }
      }else if(command == 't' || command == 'A'){//Handshake
        newLine == readChar();
          if(affectedCube == 'a' || affectedCube == 'A'){
            sendMessage("shake that ass!");
            resetToInitState();
            // Should we perhaps clear the serial buffer?
            // Serial.clear();
          }
      }
    }
  }
}

void readCube(int i){

  if(ACTIVATE_IR){
    if(!Cube_class::someCubeIsBusy 
      && millis() - cubes[i].triggerStamp > 300
      && millis() - cubes[i].lastIrRead > 40
      ){
      // if(i == 0){
      //   sendMessage(String(cubes[i].irValue));
      // }
      if(cubes[i].irTriggered() && !Cube_class::someCubeIsBusy){
        
        cubes[i].cubeOffVerified = false;
        // sendTrigger(i, convertToByte(cubes[i].irValue, 15, irMaxDistance));
        if(cubes[i].scalePosition != cubes[i].lastScalePosition
          || millis() - cubes[i].lastIrMessage > 150 ){
          cubes[i].lastIrMessage = millis();
          sendTrigger(i, cubes[i].scalePosition);
        }
      }else if(!cubes[i].cubeOffVerified){
        cubes[i].lastIrMessage = millis();
        sendTurnOffCube(i);
      }
    }
  }

  //Reed switch stuff. Redundant code, but since only two reeds that's ok.
  cubes[i].updateReedStates();//Call this function only once per loop to keep track of reed states

  // int reed1 = cubes[i].getReedOneState();
  // if(reed1 == REED_RISING || reed1 == REED_TOUCHING){//rising edge or high
  //   int pair[2];
  //   getCopyPair(pair, i, RIGHT); // Different direction depending on which reed switch. C-style function that writes to the sent parameter pair.
  //   if(cubes[i].getCopyingState() == IDLE && reed1 == REED_RISING){//only send copyrequest on rising edge

  //     sendCopyRequest(pair[0], pair[1]);
  //   }
  //   if(cubes[i].getCopyingState() == AWAITINGCONFIRMATION){
  //     touchAnimation(pair[0], pair[1]);
  //   }
  // }

  for(int j = 0; j < 2; j++){
    int direction;
    if(j == 0){direction = REED1DIRECTION;}else{direction = REED2DIRECTION;} // Set direction for current reed.
    int pair[2]; // this pair will represent the indexes for the touching cubes for the rest of this loop
    if(!getCopyPair(pair, i, direction)){//Retrieve the pair. Also, We should never handle the reeds on the edges. Hence the if statement.
      if(cubes[i].getReedState(j) == REED_TOUCHING){//Just some test code to visualize when edge reed is triggered
        cubes[i].setCubeColor(150,150,150);
      }
      // if(cubes[i].getReedState(0)){
      //   String msg = "Failed getCopyPair, with reed1: ";
      //   msg += String(pair[0]) + ", " + String(pair[1]) + ", j is " + String(j);
      //   sendMessage(msg);
      // }
      // if(cubes[i].getReedState(1)){
      //   String msg = "Failed getCopyPair, with reed2: ";
      //   msg += String(pair[0]) + ", " + String(pair[1]) + ", j is " + String(j);
      //   sendMessage(msg);
      // }
      continue; //Jump to next loop iteration if this is an edge reed
    }



    switch (cubes[i].getReedState(j)){
      case REED_IDLE:{
        if(cubes[pair[0]].getCopyingState() == COPYINGCONFIRMED && cubes[pair[1]].getCopyingState() == COPYINGCONFIRMED){
          cubes[pair[0]].setCubeColor(0, 255, 0);
          cubes[pair[1]].setCubeColor(0, 255, 0);
          // sendMessage(String("Cubes detached and copying finished"));
          sendStartRequestIn(1500);
          cubes[pair[0]].setCopyingState(IDLE);
          cubes[pair[1]].setCopyingState(IDLE);
        }
      }break;
      case REED_RISING:{
      }//Dont break here. we want same behaviour for both rising and touching
      case REED_TOUCHING:{
        if(cubes[pair[0]].getCopyingState() == IDLE && cubes[pair[1]].getCopyingState() == IDLE && !Cube_class::someCubeIsBusy){//Only send copyrequest if both cubes are idle
          turnOffAllCubes(); // fade out all cubes when sequence is not running
          sendCopyRequest(pair[0], pair[1]);//This function also changes the state of the cubes after the message is sent
        }else if(cubes[pair[0]].getCopyingState() == AWAITINGCONFIRMATION && cubes[pair[1]].getCopyingState() == AWAITINGCONFIRMATION){
          turnOffAllCubes(); // fade out all cubes when sequence is not running
          touchAnimation(pair[0], pair[1]);
        }else if(cubes[pair[0]].getCopyingState() == COPYINGCONFIRMED && cubes[pair[1]].getCopyingState() == COPYINGCONFIRMED){
          cubes[pair[0]].setCubeColor(0, 255, 0);
          cubes[pair[1]].setCubeColor(0, 255, 0);
        }
      }break;
      case REED_FALLING:{
        turnOffAllCubes(); // fade out all cubes when sequence is not running
        // if(reed1 == REED_IDLE && reed2 == REED_IDLE){
        // }
      }break;
    }
  }

  // int reed2 = cubes[i].getReedTwoState();
  // if(reed2 == REED_RISING || reed2 == REED_TOUCHING){
  //   int pair[2];
  //   getCopyPair(pair, i, UP);
  //   if(cubes[i].getCopyingState() == IDLE && reed2 == REED_RISING){
  //     sendCopyRequest(pair[0], pair[1]);
  //   }
  //   if(cubes[i].getCopyingState() == AWAITINGCONFIRMATION){
  //     touchAnimation(pair[0], pair[1]);
  //   }
  // }

  // switch (cubes[i].getCopyingState()){

  //   case IDLE:{

  //   }break;
  //   case AWAITINGCONFIRMATION:{

  //   }break;
  //   case COPYINGCONFIRMED:{//Stay in this state until we "untouch" the cubes
  //     cubes[i].setMyColor(0);
  //     cubes[i].setCubeColor(0, 255, 0);
  //     if(reed1 == REED_IDLE && reed2 == REED_IDLE){
  //       sendMessage("Cubes detached and copying finished");
  //       sendStartRequest();
  //       cubes[i].setCopyingState(IDLE);
  //     }
  //   }break;
  //   // cubes[i].copyingFinished = false;
  //   // cubes[i].copyRequestSent = false;
  // }

  // Piezo stuff
  // Should we record?
  if(/// In some cases we don't want to record even if it's triggered.
      cubes[i].piezoTriggered(piezoThreshold)
      // &&  cubes[i].shaking()
      && !cubes[i].isWaitingToRecord 
      && !cubes[i].isRecording
      && !cubes[i].getReedState(0)//provide against false tap trigger when touching two cubes.
      && !cubes[i].getReedState(1)
      // && !Cube_class::sharedIsWaitingToRecord
      // && !Cube_class::sharedIsRecording
      && !Cube_class::someCubeIsBusy
      && millis() > 10000
      //TODO: Check if two piezo are triggered close in time. Then this means cubes are touching.
    ){
    cubes[i].isWaitingToRecord = true;
    // Cube_class::sharedIsWaitingToRecord = true;
    turnOffAllCubes();
    sendRecordRequest(i);
    // delay(4000);
  }

  if(cubes[i].isWaitingToRecord){
    cubes[i].recordAnimation();
  }
  if(cubes[i].isRecording){
    cubes[i].setCubeColor(255, 0, 0);
  }

}

void cubeDiagnosis(int i){
  if(ACTIVATE_IR && cubes[i].irTriggered()){
    // int bbb = map(cubes[i].smoothedIrValue, 550, IRthreshold, 0, 6);
    cubes[i].setCubeColor(Wheel(convertToByte(cubes[i].scalePosition, 0, 5)));
    Serial.print("ir on ");
    Serial.print(i);
    // Serial.print(" is triggered with a scaleposition of ");
    // Serial.print(cubes[i].scalePosition);
    // Serial.print(", a cm value of ");
    // Serial.print(cubes[i].calculatedDistance);
    Serial.print(", and a irValue of ");
    Serial.print(cubes[i].smoothedIrValue);
    Serial.println();
    // sendTrigger(i, cubes[i].irValue);
    // delay(2000);
  }else{
    // sendTurnOffCube(i);
  }

  if(cubes[i].piezoTriggered(piezoThreshold)){
    cubes[i].setCubeColor(255,255,255); //White if tapped

    Serial.print("Cube ");
    Serial.print(i);
    Serial.print(" is tapped: ");
    Serial.print(cubes[i].piezoDifference);
    if(cubes[i].getReedState(0) || cubes[i].getReedState(1)){
      cubes[i].setCubeColor(0,0,255); //Blue if reed is active during tap
      Serial.print(" but reed switch was active");
    }
    Serial.println();
  }

  // if(cubes[i].shaking()){
  //   cubes[i].setCubeColor(0,255,255);

  //   Serial.print("Cube ");
  //   Serial.print(i);
  //   Serial.print(" had the tiltSwitch excited.");
  //   Serial.println();
  // }

  // if(cubes[i].shaking() && cubes[i].piezoTriggered(piezoThreshold)){
  //   cubes[i].setCubeColor(255,0,255); //
  // }

  cubes[i].updateReedStates();
  
  int reed1 = cubes[i].getReedState(0);
  if(reed1){
    Serial.print("Reed 1 on cube ");
    Serial.print(i);
    Serial.print(" has state: ");
    Serial.print(reed1);
    Serial.print(" getCopypair returned: ");
    int pair[2]; // this pair will represent the indexes for the touching cubes for the rest of this loop
    getCopyPair(pair, i, REED1DIRECTION);
    Serial.print(pair[0]);
    Serial.println(pair[1]);

    // Serial.print(". Direction is RIGHT");

    //touchAnimation(pair[0], pair[1]);
    cubes[pair[0]].pullAnimation(RIGHT);
    cubes[pair[1]].pullAnimation(RIGHT);
  }
  int reed2 = cubes[i].getReedState(1);
  if(reed2){
    Serial.print("Reed 2 on cube ");
    Serial.print(i);
    Serial.print(" has state: ");
    Serial.print(reed2);
    Serial.print(" getCopypair returned: ");
    int pair[2]; // this pair will represent the indexes for the touching cubes for the rest of this loop
    getCopyPair(pair, i, REED2DIRECTION);
    Serial.print(pair[0]);
    Serial.println(pair[1]);

    // Serial.print(". Direction is UP");

    cubes[pair[0]].pullAnimation(UP);
    cubes[pair[1]].pullAnimation(UP);
    //touchAnimation(pair[0], pair[1]);
  }
    
}

void measureIR(int i){
  // Serial.println(cubes[i].readIr());
  // Serial.print(',');


  if(i == 0){
    // Serial.print("Raw IR reading ->    ");
  }
  
  // Serial.print(i);
  // Serial.print(": ");
  Serial.print(cubes[i].readIr());
  // Serial.print('\t');

  Serial.print(',');


  if(i + 1 >= NUMBEROFCUBES ){
    Serial.println();
  }


}

void plotPiezos(){
  for(int i = 0; i < NUMBEROFCUBES; i++){
    
    Serial.print(cubes[i].readPiezo());
    Serial.print(",");
  }
  Serial.println();
}

void plotIr(){
  for(int i = 0; i < NUMBEROFCUBES; i++){
    
    Serial.print(cubes[i].readIr());
    if(i == NUMBEROFCUBES-1){
      break;
    }
    Serial.print(",");
  }
  Serial.println();
  delay(10);
}

void printNeighbours(){
  Serial.println("Neigbours. UP, RIGHT, DOWN, LEFT");
  for(int i = 0; i < NUMBEROFCUBES; i++){
    int neighbours[4];
    neighbours[0] = cubes[i].getNeighbourCube(UP);
    neighbours[1] = cubes[i].getNeighbourCube(RIGHT);
    neighbours[2] = cubes[i].getNeighbourCube(DOWN);
    neighbours[3] = cubes[i].getNeighbourCube(LEFT);
    Serial.print("Cube "); Serial.print(i); Serial.print(": ");
    Serial.print(neighbours[0]);Serial.print(", ");
    Serial.print(neighbours[1]);Serial.print(", ");
    Serial.print(neighbours[2]);Serial.print(", ");
    Serial.print(neighbours[3]);
    Serial.println();
  }
  Serial.println(F("******************"));
}

void touchAnimation(int sourceCube, int destinationCube){
  if(destinationCube == -1){//Not thourough test, but we only expect good values, besides getting -1 when an edge reed is triggered.
    cubes[sourceCube].setCubeColor(255, 190, 0);
    return;
  }

  //first we have to calculate direction :-( I know this is basically getNeighbourCube backwards and a lot of extra calculations. But i want all functions to have sensible parameters.
  int direction;
  if(sourceCube + 1 == destinationCube){
    direction = RIGHT;
  }else if(sourceCube - 1 == destinationCube){
    direction = LEFT;
  }else if(sourceCube + GRID_SIZE_X == destinationCube){
    direction = DOWN;
  }else if(sourceCube - GRID_SIZE_X == destinationCube){
    direction = UP;
  }else{
    sendMessage(F("Something's wrongs with the touchanimation. Check Arduino code"));
    return;
  }

  cubes[sourceCube].pullAnimation(direction);
  cubes[destinationCube].pullAnimation(direction, 3);
}

bool getCopyPair(int pair[], int toucher, int direction){ // Different direction depending on which reed switch. C-style function that writes to the sent parameter pair.
  int neighbour = cubes[toucher].getNeighbourCube(direction);
  // int pair[2];
  if(neighbour == -1){
    pair[0] = toucher;
    pair[1] = -1;
    return false;
  }else if(cubes[toucher].recordStamp > cubes[neighbour].recordStamp){
    pair[0] = toucher;
    pair[1] = neighbour;
    return true;
  }else{
    pair[0] = neighbour;
    pair[1] = toucher;
    return true;
  }
  // return pair;
}

void sendStartRequestIn(int duration){
  startRequestSendTime = millis() + duration;
  shouldSendStartRequest = true;
}

////SERIAL STUFF!
// Blocks until another byte is available on serial port
char readChar(){
  while (Serial.available() < 1) { } // Block
  return Serial.read();
}


void sendTrigger(int cubeNumber, int value){
  Serial.write('#');
  Serial.write('/');
  Serial.write(cubeNumber);
  Serial.write(value);
  Serial.write('\n');
}

void sendTurnOffCube(int cubeNumber){
  Serial.write('#');
  Serial.write(92);
  Serial.write(cubeNumber);
  // Serial.write(value);
  Serial.write('\n');
}

void sendRecordRequest(int cubeNumber){
  //Hmmm. Is this the right place to handle the busy flag?
  Cube_class::someCubeIsBusy = true;

  //Make sure that the sequencer is not started by some queued startrequest.
  shouldSendStartRequest = false;

  Serial.write('#');
  Serial.write('[');
  Serial.write(cubeNumber);
  Serial.write('\n');
}

void sendCopyRequest(int sourceCube,int destinationCube){
  if(destinationCube == -1){//Not thourough test, but we only expect good values, besides getting -1 when an edge reed is triggered.
    return;
  }
  //First. Make sure that the sequencer is not started by some queued startrequest.
  shouldSendStartRequest = false;

  // cubes[sourceCube].copyRequestSent = true;
  // cubes[destinationCube].copyRequestSent = true;
  cubes[sourceCube].setCopyingState(AWAITINGCONFIRMATION);
  cubes[destinationCube].setCopyingState(AWAITINGCONFIRMATION);
  // Serial.print("copyrequest: "); Serial.print(cubeNumber1); Serial.println(cubeNumber2);
  Serial.write('#');
  Serial.write('*');
  Serial.write(sourceCube);
  Serial.write(destinationCube);
  Serial.write('\n');
}

void sendStartRequest(){
  Serial.write('#');
  Serial.write('>');
  Serial.write('\n');
}

void sendMessage(String msg){
  Serial.write('#');
  Serial.write('t');
  Serial.print(msg);
  Serial.write('\n');
}

void dimLeds(int i){
  // Continuously dim all the leds of this cube and show it.
  // for(int i = 0; i<NUMBEROFCUBES; i++){
    cubes[i].fadeToMyColor(64);//parameter is speed. Lower is faster
    cubes[i].strip.show();
  // }
}

//This function is untested. But probably works :-)
void turnOffAllCubes(){
  // for(int i = 0; i<NUMBEROFCUBES*PIXELSPERCUBE; i++){
  //   strip.setPixelColor(i, 0);
  // }
  for(int i = 0; i < NUMBEROFCUBES; i++){
    cubes[i].setMyColor(0);
    // cubes[i].clear();
    // cubes[i].strip.show();
  }
}

void debugColorCycle(){
  debugColorCycleProgress += 2;
  if(debugColorCycleProgress >= 1000){
    debugColorCycleProgress = 0;

    debugColorCycleCurrentColor++;
    debugColorCycleCurrentColor %= 4;
  }

  for(int i = 0; i < NUMBEROFCUBES; i++){
    if(debugColorCycleProgress < 200){
      cubes[i].setCubeColor(0,0,0);
    }else if(debugColorCycleProgress < 400){
      cubes[i].setCubeColor(1,1,1);
    }else if(debugColorCycleProgress < 800){
      uint8_t intensity = map(debugColorCycleProgress, 600, 800, 1, 255);
      cubes[i].setCubeColor(intensity, intensity, intensity);
    }else{
      cubes[i].setCubeColor(255, 255, 255);
    }

    cubes[i].strip.show();
  }
}

void resetToInitState(){
  Cube_class::someCubeIsBusy = false;
  for(int i = 0; i < NUMBEROFCUBES; i++){
    cubes[i].init();
  }
  // turnOffAllCubes(); 
}


// UNTESTED!!!!!!!!!!!!!!!!!!!!!!!!
// void rainbowCycle(int wait) {
//   int j = (millis()/wait)%(256*5);
//   for(int i=0; i< strip.numPixels(); i++) {
//     strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
//   }
//   strip.show();
// }

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return Adafruit_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return Adafruit_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return Adafruit_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

void set_amp(){
  Serial.println("Running amplification setup procedure.");
  while(true){
    for(int i = 0; i < NUMBEROFCUBES; i++){
      int value = cubes[i].readPiezo();
      if(value > 500){
        cubes[i].setCubeColor(255,255,255);
      }else if(value > 400){
        cubes[i].setCubeColor(255,255,0);
      }else if(value > 300){
        cubes[i].setCubeColor(255,0,255);
      }else if(value > 200){
        cubes[i].setCubeColor(0,0,255);
      }else if(value > 100){
        cubes[i].setCubeColor(0,255,0);
      }else if(value > 50){
        cubes[i].setCubeColor(255,0,0);
      }else{
        cubes[i].setCubeColor(0,0,0);
      }
      cubes[i].strip.show();
    }
  }
}