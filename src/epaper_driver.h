#ifndef EPAPER_DRIVER_H
#define EPAPER_DRIVER_H

#include <Arduino.h>
#include <GxEPD2_3C.h>
#include "GxEPD2_750c_Z08_Custom.h"

typedef GxEPD2_3C<GxEPD2_750c_Z08_Custom, GxEPD2_750c_Z08_Custom::HEIGHT / 2> EPaperDisplayType;

class EPaperDriver {
public:
    EPaperDriver();
    void wakeHardware();
    void initDisplay();
    void sleep();
    
    EPaperDisplayType& getDisplay();

private:
    EPaperDisplayType display;
};

#endif // EPAPER_DRIVER_H