#ifndef CURRENTSENSE_H
#define CURRENTSENSE_H

#define RESISTANCE 82.0
#define SAMPLING_MSEC 333
#define ZERO_CROSSING -126.2
#define SCALE_FACTOR .5  //compensate for the quadruple-wired coil

float getVPP();
float getCurrentFlowInAmps();

#endif
