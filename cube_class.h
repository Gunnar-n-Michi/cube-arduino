#ifndef CUBE_H
#define CUBE_H

#include <Arduino.h>
#include "helpers.h"
#define SMOOTHINGSIZE 2
#define REQUIRED_SHAKES 10
#define IRMAX 550

//default value
#ifndef COPYSPEED
#define COPYSPEED 100
#endif

// class Adafruit_NeoPixel;

class Cube_class{

private:
	unsigned long _tiltSwitchTimeStamp[REQUIRED_SHAKES];
	int _currentTiltTimeStampIndex;
	int _irReadings[SMOOTHINGSIZE];
	int _irIndex;
	// int _smoothedIrValue;
	int _total;
	int _cubeNumber;
	bool _invertedTiltSwitch;
	int _stripOffset;
	int _firstPixel;
	int _piezoRestDiff;
	int _lastPiezoValue;
	int _filteredPiezo;
	static constexpr float _piezoFilterConstant = 1.0f;
	static const unsigned long _tappTime = 450;
#define IRTABLESIZE 16
	static int irMeasurements[][IRTABLESIZE];

	int _irPin, _piezo , _reed1, _reed2, _ledPin, _tiltSwitch, _threshold;

	// int _reed1State, _reed2State;
	int _reedStates[2];
	int _copyingState;


public:
	int myColor[3];
	int scalePosition;
	int lastScalePosition;
	int calculatedDistance;
	int smoothedIrValue;
	int colorIntensity;

	int piezoDifference;

	unsigned long recordStamp;
	unsigned long lastIrRead;
	unsigned long lastIrMessage;
	unsigned long triggerStamp;

	static bool sharedIsWaitingToRecord;
	static bool sharedIsRecording;
	static bool someCubeIsBusy;

	bool cubeOffVerified; //Indicates if host computer has received acknowledgement for off message
	bool isWaitingToRecord;
	bool isRecording;
	// bool copyRequestSent;
	// bool copyingFinished;

	Adafruit_NeoPixel strip;

	Cube_class(int cubeNumber, int irPin, int piezo, int reed1, int reed2, int ledPin, int tiltSwitch, int threshold, bool invertedTiltSwitch = true, int stripOffset = 0): _cubeNumber(cubeNumber), _irPin(irPin), _piezo(piezo), _reed1(reed1), _reed2(reed2), _ledPin(ledPin), _tiltSwitch(tiltSwitch), _threshold(threshold), _invertedTiltSwitch(invertedTiltSwitch), _stripOffset(stripOffset), smoothedIrValue(0), _irIndex(0), _total(0), strip(Adafruit_NeoPixel(8, ledPin, NEO_GRB + NEO_KHZ800)){}


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
		

		for(int i = 0; i<SMOOTHINGSIZE*2; i++){//This is to fill the moving average with values.
			irTriggered();
		}

		setupPiezoSensitivity();
		_lastPiezoValue = analogRead(_piezo);

		//Tiltswitch stuff
		// pinMode(_tiltSwitch, INPUT);
		// digitalWrite(_tiltSwitch, HIGH);
		//_currentTiltTimeStampIndex = 0;

		// //An array of the latest shake timestamps.
		// for(int i = 0; i<REQUIRED_SHAKES; i++){
		// 	_tiltSwitchTimeStamp[i] = 0;
		// }

		recordStamp = millis();
		lastIrRead = millis();
		lastIrMessage = millis();
		triggerStamp = millis();

		cubeOffVerified = true;
		isWaitingToRecord = false;
		isRecording = false;
		// copyRequestSent = false;

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

		return -1;
	}

	//////SENSOR FUNCTIONS


	//We don't use the tiltswitches anymore. vibration piezos instead. They are still in the hardware though.
	//If you want to use the tiltswitches, make sure they are configured to use pullup resistors.

	// bool shaking(){
	// 	// Serial.println((_invertedTiltSwitch xor !digitalRead(_tiltSwitch)));
	// 	unsigned long currentTime = millis();
	// 	bool triggered = _invertedTiltSwitch xor !digitalRead(_tiltSwitch);
	// 	if(triggered){
	// 		// Serial.print("shaken at: "); Serial.println(currentTime);
	// 		// setCubeColor(0,0,255);
	// 		_tiltSwitchTimeStamp[_currentTiltTimeStampIndex] = currentTime; 
	// 		_currentTiltTimeStampIndex ++;
	// 		_currentTiltTimeStampIndex %= REQUIRED_SHAKES;

	// 		if(currentTime - _tiltSwitchTimeStamp[_currentTiltTimeStampIndex] <= _tappTime){
	// 			return true;
	// 		}
	// 	}	
	// 	return false;
	// }

	// Original Piezotriggered
	// bool piezoTriggered(int threshold){
	// 	analogRead(_piezo);
	// 	// delay(1);
	// 	int newPiezoReading = analogRead(_piezo);
	// 	bool result;
	// 	if(newPiezoReading - _filteredPiezo > threshold){
	// 		result = true;
	// 	}else{
	// 		_filteredPiezo = _piezoFilterConstant * newPiezoReading + (1 - _piezoFilterConstant) * _filteredPiezo;
	// 		result = false;
	// 	}
	// 	return result;

	// 	// return analogRead(_piezo)>50;
	// }


	// A version that looks for big difference between two subsequent readings. Aims to better detect the spiky voltage variations that are rendered by piezo taps.
	bool piezoTriggered(int threshold){
		// analogRead(_piezo);
		// delay(1);
		int newPiezoReading = analogRead(_piezo);
		bool result;
		piezoDifference = abs(newPiezoReading - _lastPiezoValue);
		if(piezoDifference > threshold + _piezoRestDiff){
			// newPiezoReading > (threshold+_piezoRestDiff)){
			// Serial.print("Tapped with value: "); Serial.println(newPiezoReading);
			result = true;
		}else{
			result = false;
		}
		_lastPiezoValue = newPiezoReading;
		return result;

		// return analogRead(_piezo)>50;
	}

	void setupPiezoSensitivity(){
		int maxDifference = 0;
		analogRead(_piezo);
		int lastValue = analogRead(_piezo);
		for(int i = 0; i < 1000; i++){
			
			analogRead(_piezo);
			int value = analogRead(_piezo);
			int difference = abs(value - lastValue);
			if( difference > maxDifference){
				maxDifference = difference;
			}
			lastValue = value;
			delayMicroseconds(150);
		}
		_piezoRestDiff = maxDifference;
		// Serial.print("piezoRestDiff for cube ");
		// Serial.print(_cubeNumber);
		// Serial.print(" calculated to ");
		// Serial.println(maxDifference);
	}

	int readPiezo(){
		// analogRead(_piezo);
		return analogRead(_piezo);
	}

	bool irTriggered(){
			lastIrRead = millis();

			//Moving average part
			_total -= _irReadings[_irIndex];
			// analogRead(_irPin);//Read one time before to make sure the analogRead has settled before using the value.
			// delayMicroseconds(50);
			int reading = analogRead(_irPin);

			///NOTE: This part tries to deal with the weird measure offset related to the voltage drop when driving the LEDs.
			int voltageBias = map(colorIntensity, 0, 255, 0, 60);
			_irReadings[_irIndex] = reading - voltageBias;
			_total += _irReadings[_irIndex];
			_irIndex++;

			if(_irIndex >= SMOOTHINGSIZE){
				_irIndex = 0;
			}

			smoothedIrValue = _total/SMOOTHINGSIZE;

			// Lookup table part
			// calculates the variable calculatedDistance;
			// First finds the closest values above and below in the table.
			// Then interpolates between those two.
			// Be aware that the table has and inverse relation from measurements to distance (cm)
			// for(int i = 0; i < IRTABLESIZE; i++){
			// 	if(smoothedIrValue > irMeasurements[0][i]){//Is between current and previous index
			// 		// Serial.print("Value in lookup matched: ");
			// 		// Serial.print(value);Serial.print("on index");Serial.print(i); Serial.print(" ");
			// 		// Serial.println(irMeasurements[0][i]);
			// 		if(i == 0){//Is the sensed value greater than the highest premeasured one?
			// 			//Simply pick the first distance value in the table.
			// 			calculatedDistance = irMeasurements[1][0];
			// 			// Serial.println("was at first index in irMeasurements. breaking the loop!");
			// 			break;
			// 		}
			// 		int measureDifference = irMeasurements[0][i-1] - irMeasurements[0][i]; //The difference between the two premeasured values
			// 		// Serial.print("measureDifference is: "); Serial.println( measureDifference);
			// 		int deltaValue = smoothedIrValue - irMeasurements[0][i];//how much higher than the premeasured value below
			// 		// Serial.print("deltaValue is: "); Serial.println( deltaValue);
			// 		float factor = (float) deltaValue/measureDifference; //precentage between the lower and higher premeasured values
			// 		// Serial.print("factor is: "); Serial.println( factor);
			// 		int deltaDistance = factor * (irMeasurements[1][i] - irMeasurements[1][i-1]);
			// 		// Serial.print("deltaDistance is: "); Serial.println( deltaDistance);
			// 		calculatedDistance = irMeasurements[1][i-1] + deltaDistance;
			// 		// Serial.print("calculatedDistance is: "); Serial.println( calculatedDistance);

			// 		break;
			// 	}
			// }
			// _irReadings[_irIndex] = calculatedDistance;

			////TESTCODE
			// if(_cubeNumber == 0){
			// 	Serial.println(_irReadings[_irIndex]);
			// }
			//*****************
			

			
			//What is this?????
			//Shouldn't we map from distance to scale???
			//Apparently not. Since this seems to work better...
			lastScalePosition = scalePosition;
			scalePosition = constrain(map(smoothedIrValue, IRMAX, _threshold+40, 0, 5), 0,5);
			// scalePosition = constrain(map(calculatedDistance, 15, 50, 0, 5), 0, 5);

			unsigned long endStamp = millis();

			// Serial.print("Running time of irTriggered: ");Serial.println(endStamp - lastIrRead);

			if(smoothedIrValue > _threshold){
				return true;
			}
			return false;
	}

	int readIr(){
		// analogRead(_irPin);
		int value = analogRead(_irPin);
		int voltageBias = map(colorIntensity, 0, 255, 0, 60);
		return value - voltageBias;
	}

	int readSmoothedIr(){
		// analogRead(_irPin);
		irTriggered();
		return smoothedIrValue;
		return analogRead(_irPin);
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

		colorIntensity = (r + g + b)/3;
		// strip.show();
	}

	void clear(){
		setCubeColor(0);
	}

	// Input a value 0 to 255 to get a color value.
	// The colours are a transition r - g - b - back to r.
	void setColorFromRainbowByte(uint8_t WheelPos) {
		WheelPos = 255 - WheelPos;
		uint32_t color;

		if(WheelPos < 85) {
			color = strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
		} else if(WheelPos < 170) {
		WheelPos -= 85;
			color = strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
		} else {
		WheelPos -= 170;
			color = strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
		}

		for(int i = _firstPixel; i<_firstPixel + PIXELSPERCUBE; i++){
			strip.setPixelColor(i, color);
		}

		//Assume the intensity is same for each color. So that would mean this function sets the color to a total of 255.
		colorIntensity = 255/3;
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

	//Lower value of fadeSpeed means faster fade.
	void fadeToMyColor(int fadeSpeed){
		int currentColor[3];
		for(int j=_firstPixel; j < _firstPixel + PIXELSPERCUBE; j++) {
			uint32_t extractColor = strip.getPixelColor(j);
			
			//Depack the colors
			currentColor[0] = (uint8_t)(extractColor >> 16),
			currentColor[1] = (uint8_t)(extractColor >>  8),
			currentColor[2] = (uint8_t)extractColor;

			for(int i=0; i < 3; i++){
				if(currentColor[i] > myColor[i]){
					currentColor[i] -= currentColor[i]/fadeSpeed+1;
					if(currentColor[i] < myColor[i]){//If we overshoot, then set it to target
						currentColor[i] = myColor[i];
					}
				}else if(currentColor[i] < myColor[i]){
					currentColor[i] += currentColor[i]/fadeSpeed+1;
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
		colorIntensity = (currentColor[0] + currentColor[1] + currentColor[2])/3;
	}

	void pullAnimation(int direction, int shift = 0){
		
		//Continuously dim pixels. This part updates every time the function is called
		//This runs along the fade in the mainloop, which results in increased fadespeed during copying
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

		colorIntensity = red/3;
	}



};
bool Cube_class::someCubeIsBusy = false;
int Cube_class::irMeasurements[2][IRTABLESIZE] = {
	// 	{556, 549, 542, 532, 525, 514, 401, 304, 248, 0},
	// 	{15,  16,  17,  18,  19,  20,  30,  40,  50, 60}
	// };


	{555, 550, 537, 529, 519, 511, 461, 405, 352, 315, 283, 256, 232, 217, 191, 0},

	{15,  16,  17,  18,  19,  20,  25,  30,  35,  40,  45,  50,  55,  60,  70, 180}
};
#endif