/*
  Emon.cpp - Library for openenergymonitor
  Created by Trystan Lea, April 27 2010
  GNU GPL
  modified to use up to 12 bits ADC resolution (ex. Arduino Due)
  by boredman@boredomprojects.net 26.12.2013
  Low Pass filter for offset removal replaces HP filter 1/1/2015 - RW
*/

// Proboscide99 10/08/2016 - Added ADMUX settings for ATmega1284 e 1284P (644 / 644P also, but not tested) in readVcc function

// #include "WProgram.h" un-comment for use on older versions of Arduino IDE
#include "EmonLib.h"
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "user_adc.h"
//--------------------------------------------------------------------------------------
// Sets the pins to be used for voltage and current sensors
//--------------------------------------------------------------------------------------
void EnergyMonitor::current(double _ICAL)
{
    ICAL = _ICAL;
    offsetI = ADC_COUNTS >> 1;
}
//--------------------------------------------------------------------------------------
// Sets the pins to be used for voltage and current sensors based on emontx pin map
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
double EnergyMonitor::calcIrms(unsigned int Number_of_Samples)
{

#if defined emonTxV3
    int SupplyVoltage = 3300;
#else
    int SupplyVoltage = readVcc();
#endif

    for (unsigned int n = 0; n < Number_of_Samples; n++)
    {
        sampleI = adc_get_voltage_value();

        // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset,
        //  then subtract this - signal is now centered on 0 counts.
        offsetI = (offsetI + (sampleI - offsetI) / 1024);
        filteredI = sampleI - offsetI;

        // Root-mean-square method current
        // 1) square current values
        sqI = filteredI * filteredI;
        // 2) sum
        sumI += sqI;
    }

    double I_RATIO = ICAL * ((SupplyVoltage / 1000.0) / (ADC_COUNTS));
    Irms = I_RATIO * sqrt(sumI / Number_of_Samples);

    // Reset accumulators
    sumI = 0;
    //--------------------------------------------------------------------------------------

    return Irms;
}

// thanks to http://hacking.majenko.co.uk/making-accurate-adc-readings-on-arduino
// and Jérôme who alerted us to http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/

long EnergyMonitor::readVcc()
{
    long result;

    return 3300;
}
