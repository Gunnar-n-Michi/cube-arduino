#ifndef CUBE_H
#define CUBE_H

#include <Arduino.h>
#include "helpers.h"
#define SMOOTHINGSIZE 3

// class Adafruit_NeoPixel;

class Cube_class{

private:
	int _irReadings[SMOOTHINGSIZE];
	int _irIndex;
	int _total;
	int _cubeNumber;
	bool _invertedTiltSwitch;
	int _stripOffset;
	int _firstPixel;
	// int _piezoRestValue;
	// int _lastPiezoValue;
	int _filteredPiezo;
	static const float _piezoFilterConstant = 1.0f;

	int _irPin, _piezo , _reed1, _reed2, _ledPin, _threshold;

	// int _reed1State, _reed2State;
	int _reedStates[2];
	int _copyingState;


public:
	int myColor[3];
	int irValue;
	unsigned long recordStamp;
	unsigned long lastIrRead;
	unsigned long lastIrMessage;
	unsigned long triggerStamp;

	static bool sharedIsWaitingToRecord;
	static bool sharedIsRecording;
	static bool someCubeIsBusy;

	bool cubeOffVerified;
	bool isWaitingToRecord;
	bool isRecording;
	// bool copyRequestSent;
	// bool copyingFinished;

	Adafruit_NeoPixel strip;

	Cube_class(int cubeNumber, int irPin, int piezo, int reed1, int reed2, int ledPin, int threshold, bool invertedTiltSwitch = false, int stripOffset = 0): _cubeNumber(cubeNumber), _irPin(irPin), _piezo(piezo), _reed1(reed1), _reed2(reed2), _ledPin(ledPin), _threshold(threshold), _invertedTiltSwitch(invertedTiltSwitch), _stripOffset(stripOffset), irValue(0), _irIndex(0), _total(0), strip(Adafruit_NeoPixel(8, ledPin, NEO_GRB + NEO_KHZ800)){}


	void init(){
		strip.begin();
  		strip.show(); // Initialize all pixels to 'off'
		pinMode(_reed1, INPUT);
		digitalWrite(_reed1, HIGH);

		pinMode(_reed2, INPUT);
		digitalWrite(_reed2, HIGH);

		myColor[0] = 0;
		myColor[1] = 0;
		myColor[2] = 0;

		_firstPixel = 0;

		recordStamp = millis();
		lastIrRead = millis();
		lastIrMessage = millis();
		triggerStamp = millis();

		cubeOffVerified = true;
		isWaitingToRecord = false;
		isRecording = false;
		// copyRequestSent = false;

		// setupPiezoSensitivity();

	}

	int getNeighbourCube(int direction){//Returns -1 if there is no neighbour on this side.
		if(direction == UP){
			if(_cubeNumber < GRID_SIZE_X){//TESTING FOR TOP EDGE
				return -1;
			}
			return _cubeNumber-GRID_SIZE_X;
		}else if(direction == RIGHT){
			if(!((_cubeNumber+1)%GRID_SIZE_X)){//TESTING FOR RIGHT EDGE
				return -1;
			}
			return _cubeNumber+1;
		}else if(direction == DOWN){
			if(_cubeNumber + 1 > GRID_SIZE_X * (GRID_SIZE_Y - 1)){//TESTING FOR BOTTOM EDGE
				return -1;
			}
			return _cubeNumber+GRID_SIZE_X;
		}else if(direction == LEFT){
			// Serial.print("_cubeNumber is: ");
			// Serial.println(_cubeNumber);
			// Serial.print("GRID_SIZE_X is: ");
			// Serial.println(GRID_SIZE_X);
			// Serial.print("_cubeNumber % GRID_SIZE_X is:");
			// Serial.println(_cubeNumber%GRID_SIZE_X);
			if(!(_cubeNumber%GRID_SIZE_X)){//TESTING FOR LEFT EDGE
				return -1;
			}
			return _cubeNumber-1;
		}
	}

	//////SENSOR FUNCTIONS

	// bool shaking(){
	// 	return _invertedTiltSwitch xor !digitalRead(_tiltSwitch);
	// }

	bool piezoTriggered(int threshold){
		analogRead(_piezo);
		delay(1);
		int newPiezoReading = analogRead(_piezo);
		bool result;
		if(newPiezoReading - _filteredPiezo > threshold){
			result = true;
		}else{
			_filteredPiezo = _piezoFilterConstant * newPiezoReading + (1 - _piezoFilterConstant) * _filteredPiezo;
			result = false;
		}
		return result;

		// return analogRead(_piezo)>50;
	}

	// void setupPiezoSensitivity(){
	// 	int sum = 0;
	// 	for(int i = 0; i < 50; i++){
	// 		sum += analogRead(_piezo);
	// 		delay(2);
	// 	}
	// 	sum /= 50;
	// 	_piezoRestValue = sum;
	// }

	bool irTriggered(){
		// if(millis()-lastIrRead > 0){
			lastIrRead = millis();

			_total -= _irReadings[_irIndex];
			analogRead(_irPin);
			delayMicroseconds(50);
			// _irReadings[_irIndex] = analogRead(_irPin); //(int) (527.41 * 9.5 * 3/(analogRead(_irPin)))-15;
			int value = analogRead(_irPin);
			_irReadings[_irIndex] = 0.00004106960272 *value*value - 0.1409362699 * value + 79.87598879;
			// 6.673596372·10-5 x2 - 1.592576997·10-1 x + 82.88834375
			
			////TESTCODE
			// if(_cubeNumber == 0){
			// 	Serial.println(_irReadings[_irIndex]);
			// }
			//*****************
			_total += _irReadings[_irIndex];
			_irIndex++;

			if(_irIndex >= SMOOTHINGSIZE)
				_irIndex = 0;

			irValue = _total/SMOOTHINGSIZE;
			if(irValue < _threshold){
				return true;
			}
			return false;
		// }
	}

	#define REED_IDLE 0
	#define REED_RISING 1
	#define REED_TOUCHING 2
	#define REED_FALLING 3
	void updateReedStates(){
		bool active;
		//TODO: Make sure this state machine is safe. Not sure it is now.
		for(int i = 0; i < 2; i++){
			if(i == 0){
				active = !digitalRead(_reed1);
			}else{
				active = !digitalRead(_reed2);
			}
			if(_reedStates[i] == REED_IDLE && active){//Rising Edge!
				_reedStates[i] = REED_RISING;
			}else if(_reedStates[i] == REED_RISING && active){//High without edge. Go from rising to touching
				_reedStates[i] = REED_TOUCHING;
			}else if(_reedStates[i] == REED_TOUCHING && !active){//falling edge
				_reedStates[i] = REED_FALLING;
			}else if(_reedStates[i] == REED_FALLING && !active){//low without edge
				_reedStates[i] = REED_IDLE;
			}
		}
		// active = !digitalRead(_reed1);
		// if(_reed1State == REED_IDLE && active){//Rising Edge!
		// 	_reed1State = REED_RISING;
		// }else if(_reed1State == REED_RISING && active){//High without edge. Go from rising to touching
		// 	_reed1State = REED_TOUCHING;
		// }else if(_reed1State == REED_TOUCHING && !active){//falling edge
		// 	_reed1State = REED_FALLING;
		// }else if(_reed1State == REED_FALLING && !active){//low without edge
		// 	_reed1State = REED_IDLE;
		// }
		
		// active = !digitalRead(_reed2);
		// if(_reed2State == REED_IDLE && active){//Rising Edge!
		// 	_reed2State = REED_RISING;
		// }else if(_reed2State == REED_RISING && active){//High without edge
		// 	_reed2State = REED_TOUCHING;
		// }else if(_reed2State == REED_TOUCHING && !active){//falling edge
		// 	_reed2State = REED_FALLING;
		// }else if(_reed2State == REED_FALLING && !active){//low without edge
		// 	_reed2State = REED_IDLE;
		// }
	}

	int getReedState(int i){
		return _reedStates[i];
	}

	// int getReedOneState(){
	// 	return _reed1State;
	// }

	// int getReedTwoState(){
	// 	return _reed2State;
	// }

	// copyingState state names
	#define IDLE 0
	#define AWAITINGCONFIRMATION 1
	#define COPYINGCONFIRMED 2

	void setCopyingState(int state){
		if(state == IDLE){
			someCubeIsBusy = false;
		}else{
			someCubeIsBusy = true;
		}
		_copyingState = state;
	}

	int getCopyingState(){
		return _copyingState;
	}

	/////COLORING FUNCTIONS
	void setMyColor(uint32_t color){
		int
		r = (uint8_t)(color >> 16),
		g = (uint8_t)(color >>  8),
		b = (uint8_t)color;
		myColor[0] = r;
		myColor[1] = g;
		myColor[2] = b;
	}

	void setMyColor(uint8_t r, uint8_t g, uint8_t b){
		myColor[0] = r;
		myColor[1] = g;
		myColor[2] = b;
	}

	void setCubeColor(uint32_t color){
		for(int i = _firstPixel; i<_firstPixel + PIXELSPERCUBE; i++){
		strip.setPixelColor(i, color);
		}
		// strip.show();
	}

	void setCubeColor(uint8_t r, uint8_t g, uint8_t b){
		for(int i = _firstPixel; i<_firstPixel + PIXELSPERCUBE; i++){
			strip.setPixelColor(i, r, g, b);
		}
		// strip.show();
	}

	void clear(){
		setCubeColor(0);
	}


	//////////ANIMATIONS
	void decreaseBrightness(int fadeSpeed){
		for(int j=_firstPixel; j < _firstPixel + PIXELSPERCUBE; j++) {
			uint32_t color = strip.getPixelColor(j);
	      
			//Depack the colors
			int
			r = (uint8_t)(color >> 16),
			g = (uint8_t)(color >>  8),
			b = (uint8_t)color;

			//Scale down colors
			r -= r/fadeSpeed+2;
			if(r < 0)
			r = 0;
			g -= g/fadeSpeed+2;
			if(g < 0)
			g = 0;
			b -= b/fadeSpeed+2;
			if(b < 0)
			b = 0;

			strip.setPixelColor(j, r, g, b);
		}
	}

	void fadeToMyColor(int fadeSpeed){
		for(int j=_firstPixel; j < _firstPixel + PIXELSPERCUBE; j++) {
			uint32_t extractColor = strip.getPixelColor(j);
			//Depack the colors
			int currentColor[3];
			currentColor[0] = (uint8_t)(extractColor >> 16),
			currentColor[1] = (uint8_t)(extractColor >>  8),
			currentColor[2] = (uint8_t)extractColor;

			for(int i=0; i < 3; i++){
				if(currentColor[i] > myColor[i]){
					currentColor[i] -= currentColor[i]/fadeSpeed+2;
					if(currentColor[i] < myColor[i]){//If we overshoot, then set it to target
						currentColor[i] = myColor[i];
					}
				}else if(currentColor[i] < myColor[i]){
					currentColor[i] += currentColor[i]/fadeSpeed+2;
					if(currentColor[i] > myColor[i]){//If we overshoot, then set it to target
						currentColor[i] = myColor[i];
					}
				}else{
					currentColor[i] = myColor[i];
				}
			}
			strip.setPixelColor(j, currentColor[0], currentColor[1], currentColor[2]);
			// setCubeColor(currentColor[0], currentColor[1], currentColor[2]);
		}
	}

	void pullAnimation(int direction, int shift = 0){//TODO: Make this animation beautiful
		
		//Continuously dim pixels. This part updates every time the function is called
		// decreaseBrightness(64);
		fadeToMyColor(64);

		int side1position, side2position;
		int offset = direction*2-1-_stripOffset;
		unsigned int pixelChooser = (millis()/COPYSPEED + shift)%(PIXELSPERCUBE); // this value increases at an interval defined by COPYSPEED. It represents the state of the animation 
		// int colorChooser = (millis()/10)%255;



		if(pixelChooser >= PIXELSPERCUBE/2){
			return;
		}

		//Transform the chosen pixel to sidepositions. The following part moves the chosen pixel in intervals defined by #COPYSPEED
		side1position = _firstPixel+(pixelChooser-offset)%PIXELSPERCUBE;
		side2position = _firstPixel+((PIXELSPERCUBE-pixelChooser)-1-offset)%PIXELSPERCUBE;

		//Highlight chosen pixel
		strip.setPixelColor(side1position, strip.Color(0, 0, 255));
		strip.setPixelColor(side2position, strip.Color(0, 0, 255));

		strip.show();

	}

	void recordAnimation(){
		// setCubeColor(255,0,255);
		int red = 255.0f * (sin((float) millis()/100.0f)+1)/2;
		setCubeColor(red, 0, 0);
	}



};

bool Cube_class::sharedIsWaitingToRecord = false;
bool Cube_class::sharedIsRecording = false;
bool Cube_class::someCubeIsBusy = false;

#endif