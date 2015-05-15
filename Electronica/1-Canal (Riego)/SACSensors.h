/****************************************************************
SACSensors: this file contains all the functions for use the sensors and use the sensor cache.
 for the SAC Project:
  http://sacultivo.com

   In this file we have the functions for use de cache sensors

  Author: Victor Suarez Garcia<suarez.garcia.victor@gmail.com>
  Co-Author: David Cuevas Lopez<mr.cavern@gmail.com>

  * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * Version History:
 * 0.1. Initial Version
 * 0.2. Added cache struct and state struct.
 * 0.3. Added Field Capacity Sensor Read.
 * Current Version: 0.3.
*************************************************************************/

#include <Time.h>
#include <OneWire.h>
#define DS19S20_PIN 13 // SOIL TEMPERATURE PIN
#define SM_POWER_PIN 5 // SOIL MOISTURE AND FIELD CAPACITY POWER PIN (ELECTRODES)
#define SM_PIN A3 //SM PIN
#define NoWater_PIN A7 // WATER AVAILABLE PIN (WATER LEVEL TANK OR PRESOSTATE) HAS TO BE DIGITAL INPUT (A7 to ?)
#define WaterFlow_PIN 16 // WATER FLOW PIN
#define FC_PIN A0 //FIELD CAPACITY PIN
#define SM_SMOOTHING 85

volatile int NbTopsFan;
int Calc;

// This struct store the last cached data sensor.

typedef struct
{
	float cached_ST;//Last Soil Temperature
	int cached_STmin;//Last Minimum Temperature
	int cached_STMax;//Last Maximum Temperature
	int cached_SM;//Last Soil Moisture Value
	int cached_SMmin;//Last Minimum Soil Moisture
	int cached_SMOp;//Last Soil Moisture Optimum
	int cached_SMcalib;//Last Soil Moisture Calibration
	int cached_FC;//Last Field capacity
	int cached_PICicle;
	int cached_TICicle;
	tmElements_t cached_lastWaterEvent;//Last Time and Date for Pumping.

} cached_sensors;

// This Struct store all the information for the current state.

typedef struct
{
	float SMOp; // Soil Moisture Optimum
	float SMmin; //Soil Moisture minimum
	float current_SM; // current Soil Moisture
	float STMax; // Max Soil Temperature
	float STmin; // Min Soil Temperature
	float current_ST; //Current Soil Temperature
	float FC; //Field Capacity
	float SMcalib;

} State;

// Read the Soil Temperature (Celsius).

float read_ST()
{
	OneWire ds(DS19S20_PIN);
	byte data[12];
	byte addr[8];

	if ( !ds.search(addr))
	{
		ds.reset_search();
		return -1000;
	}
	if ( OneWire::crc8( addr, 7) != addr[7])
	{
		return -1000;
	}
	if ( addr[0] != 0x10 && addr[0] != 0x28)
	{
		return -1000;
	}

	ds.reset();
	ds.select(addr);
	ds.write(0x44, 1); // start conversion, with parasite power on at the end
	byte present = ds.reset();
	ds.select(addr);
	ds.write(0xBE); // Read Scratchpad
	for (int i = 0; i < 9; i++)   // we need 9 bytes
	{
		data[i] = ds.read();
	}
	ds.reset_search();
	byte MSB = data[1];
	byte LSB = data[0];
	float tempRead = ((MSB << 8) | LSB); //using two's compliment
	float TemperatureSum = tempRead / 16;
	return TemperatureSum;
}

// Read the Soil Moisture.

float read_SM(int hs_calib)
{
	digitalWrite(SM_POWER_PIN, HIGH);
	float cached_SM = analogRead(SM_PIN);
	digitalWrite(SM_POWER_PIN, LOW);
	cached_SM =  map(cached_SM, 0, hs_calib, 0, 100);
	if(cached_SM >= 100)
		cached_SM = 100;
	static float kept = 0;
	float SM = cached_SM * 100;  //MUY IMP FALTA EN LA ECUACION (current_config.SMcalib) float soil_moisture = cached_moisture * 100 / moisture_calib;
	kept = (kept * SM_SMOOTHING / 100.0);
	kept = kept + SM * (100 - SM_SMOOTHING) / 100.0;
	return cached_SM;
}

// Read the Field Capacity.

float read_FC(int s_fc)
{
	digitalWrite(SM_POWER_PIN, HIGH);
	float cached_FC = analogRead(FC_PIN);
	digitalWrite(SM_POWER_PIN, LOW);
	cached_FC =  map(cached_FC, 0, s_fc, 0, 100);
	if(cached_FC >= 100)
		cached_FC = 100;
	static float kept = 0;
	float soil_FC = cached_FC * 100;  //MUY IMP FALTA EN LA ECUACION
	kept = (kept * SM_SMOOTHING / 100.0);
	kept = kept + soil_FC * (100 - SM_SMOOTHING) / 100.0;
	return cached_FC;
}

// Update the sensors cache with the current state.

void update_State(cached_sensors & last_values, tmElements_t current_event, int SM)
{
	float curr_SM;
	long time_aux = millis();
	curr_SM = read_SM(SM);
	last_values.cached_SM = curr_SM;
	float curr_ST = read_ST();
	last_values.cached_ST = curr_ST;
}

// Get the current state from the cached values.

State read_sensors(cached_sensors & last_values)
{
	State current_state;

	current_state.SMOp = last_values.cached_SMOp;
	current_state.SMmin = last_values.cached_SMmin;
	current_state.SMcalib = last_values.cached_SMcalib;
	current_state.current_SM = last_values.cached_SM;
	current_state.current_ST = last_values.cached_ST;
	current_state.SMOp = last_values.cached_SMOp;
	current_state.STMax = last_values.cached_STMax;
	current_state.STmin = last_values.cached_STmin;
	current_state.FC = last_values.cached_FC;
	return current_state;
}

//

cached_sensors initSensorsCache()
{
	cached_sensors current_sensors;
	current_sensors.cached_SM = 0;
	current_sensors.cached_SMcalib = 0;
	current_sensors.cached_FC = 0;
	current_sensors.cached_STmin = 0;
	current_sensors.cached_STMax = 0;
	current_sensors.cached_SMmin = 0;
	current_sensors.cached_SMOp = 0;
	return current_sensors;
}

// Reading SM Sensor for calibration (Sample 100% Soil Moisture Saturation)

int readSMcalib()
{
	digitalWrite(SM_POWER_PIN, HIGH);
	int cached_SMcalib = analogRead(SM_PIN);
	digitalWrite(SM_POWER_PIN, LOW);
	return cached_SMcalib;
}

// ??

boolean state_changed(State state1, State state2)
{
	if(state1.current_SM != state2.current_SM ) return true;
	if(state1.current_ST != state2.current_ST ) return true;
	if(state1.FC != state2.FC ) return true;
	return false;
}
