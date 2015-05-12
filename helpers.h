#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>


byte convertToByte(int value, int scaleBottom, int scaleTop){
	return map(constrain(value, min(scaleBottom, scaleTop), max(scaleBottom, scaleTop)), scaleBottom, scaleTop, 0, 255);
}



#endif