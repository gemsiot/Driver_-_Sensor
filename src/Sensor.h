//© 2023 Regents of the University of Minnesota. All rights reserved.

#ifndef Sensor_h
#define Sensor_h

#include <Particle.h>
#include "ITimeProvider.h"

namespace BusType {
	constexpr uint8_t NONE = 0; 
	constexpr uint8_t I2C = 1;
	constexpr uint8_t SDI12 = 2;
	constexpr uint8_t CORE = 3; ///<You don't exist; you were never even born. Anonymity is your name. Silence your native tongue. You're no longer part of the System.
	constexpr uint8_t ANALOG = 4;
};

namespace PowerSaveModes {
	constexpr uint8_t PERFORMANCE = 0;
	constexpr uint8_t BALANCED = 1; 
	constexpr uint8_t LOW_POWER = 2;
	constexpr uint8_t ULTRA_LOW_POWER = 3;
	constexpr uint8_t EMERGENCY = 255; 
};

class Sensor
{
	
	public:
		Sensor() {}
		virtual String begin(time_t time, bool &criticalFault, bool &fault) {
			return ""; //DEBUG! Return empty string if function is not implemented by device
		};
		
		virtual String getErrors() {
			return ""; //DEBUG!
		}
		virtual uint8_t totalErrors() {
			// return 0; //DEBUG!
			return numErrors;
		}
		virtual String getData(time_t time) {
			return ""; //Return empty string if function is not implemented by device 
		};
		virtual String getMetadata() {
			return ""; //return empty string if function is not implemented by device 
		};
		virtual String selfDiagnostic(uint8_t diagnosticLevel, time_t time) {
			if(getSensorPort() == 0) throwError(FIND_FAIL); //If no port found, report failure
			return ""; //return empty string if function is not implemented by device 
		};
		// virtual bool isTalon() {
		// 	return false; //Default to not
		// };
		uint8_t getTalonPort() {
			if(talonPort < 255) return talonPort + 1;
			else return 0;
		};
		uint8_t getSensorPort() {
			if(sensorPort < 255) return sensorPort + 1;
			else return 0;
		};
		// virtual void setTalonPort(uint8_t port){};
		// virtual void setSensorPort(uint8_t pos){};
		void setTalonPort(uint8_t port)
		{
			// if(port_ > numPorts || port_ == 0) throwError(PORT_RANGE_ERROR | portErrorCode); //If commanded value is out of range, throw error 
			if(port > 4 || port == 0) throwError(TALON_PORT_RANGE_FAIL | talonPortErrorCode | sensorPortErrorCode); //If commanded value is out of range, throw error //FIX! How to deal with magic number? This is the number of ports on KESTREL, how do we know that??
			else { //If in range, update the port values
				talonPort = port - 1; //Set global port value in index counting
				talonPortErrorCode = (talonPort + 1) << 4; //Set port error code in rational counting 
			}
		}
		void setSensorPort(uint8_t port)
		{
			// if(port_ > numPorts || port_ == 0) throwError(PORT_RANGE_ERROR | portErrorCode); //If commanded value is out of range, throw error 
			if(port > 4 || port == 0) throwError(SENSOR_PORT_RANGE_FAIL | talonPortErrorCode | sensorPortErrorCode); //If commanded value is out of range, throw error //FIX! How to deal with magic number? This is the number of ports on KESTREL, how do we know that??
			else { //If in range, update the port values
				sensorPort = port - 1; //Set global port value in index counting
				sensorPortErrorCode = (sensorPort + 1); //Set port error code in rational counting 
			}
		}
		String getSensorPortString()
		{
			if(sensorPort >= 0 && sensorPort < 255) return String(sensorPort + 1); //If sensor port has been set //FIX max value
			else return "null";
		}

		String getTalonPortString()
		{
			if(talonPort >= 0 && talonPort < 255) return String(talonPort + 1); //If sensor port has been set //FIX max value
			else return "null";
		}
		virtual bool isPresent() {
			return false;
		}; 

		virtual int sleep() {
			return -1; //DEBUG!
		};
		virtual int wake(ITimeProvider& timeProvider) {
			// bool dummy1;
			// bool dummy2;
			// begin(0, dummy1, dummy2); //DEBUG!
			wakeTime = timeProvider.millis(); 
			return -1; //DEBUG!
		}; 
		
		uint8_t sensorInterface = BusType::NONE; ///<Defines which type of interface a given sensor uses
		bool keepPowered = false; ///<Specify if power should be retained for a sensor when shutting the system down
		int powerSaveMode = 0; ///<Specify what power save mode should be used
	protected:
		constexpr static int MAX_NUM_ERRORS = 10; ///<Maximum number of errors to log before overwriting previous errors in buffer
		const uint32_t SENSOR_PORT_RANGE_FAIL = 0x90010100; //FIX! 
		const uint32_t TALON_PORT_RANGE_FAIL = 0x90010200; //FIX! 
		const uint32_t FIND_FAIL = 0xFF000000; ///<Fail to locate sensor in system
		const uint32_t DETECT_FAIL = 0xED000000; ///<Fail to detect sensor at specified location
		const uint32_t EXCEED_COLLECT_TIME = 0xE0040000; ///<Expected time for data/diagnostic/metadata collection exceeded
		const uint32_t SENSOR_TIMEOUT = 0xF1000000; ///<Attempt to read sensor timed out
		const uint32_t I2C_SENSOR_COM_FAIL = 0xF12C0000; ///<Error has occored in comunication with I2C sensor
		const uint32_t REPEATED_READ_ATTEMPT = 0xF1010000; ///<It took more than one read attempt to get a value from this sensor
		const unsigned long collectMax = 60000; //Allow for a max of 60 seconds for collecting info from sensor itself (1 = data, 2 = diagnostic, 3 = metadata)
		uint8_t talonPort = 255; //Used to keep track of which port the Talon is connected to on Kestrel
		uint8_t sensorPort = 255; //Used to keep track of which port the sensor is connected to on associated Talon
		uint32_t talonPortErrorCode = 0; //Used to easily OR with error codes to add the Talon port
		uint32_t sensorPortErrorCode = 0; //Used to easily OR with error codes to add the Sensor port
		uint32_t errors[MAX_NUM_ERRORS] = {0};
		uint8_t numErrors = 0; //Used to track the index of errors array
		time_t wakeTime = 0; //Used to keep track of when the sensor woke up last
		bool errorOverwrite = false; //Used to track if errors have been overwritten in time since last report
		bool timeBaseGood = false; //Used to keep track of the valitity of the current timebase
		bool initDone = false; //Used to keep track if the initaliztion has run - used by hasReset() 
		int throwError(uint32_t error)
		{
			bool errorExists = false; //Check if error has already been reported 
			for(int i = 0; i < numErrors; i++) {
				if(errors[i] == error) {
					errorExists = true;
					break; //Exit loop if instance found
				}
			}
			if(!errorExists) { //Only add to error list if not already reported 
				errors[(numErrors++) % MAX_NUM_ERRORS] = error; //Write error to the specified location in the error array
				if(numErrors > MAX_NUM_ERRORS) errorOverwrite = true; //Set flag if looping over previous errors 
			}
			return numErrors;
		}
};

#endif