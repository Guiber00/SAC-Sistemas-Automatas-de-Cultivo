/*******************************************************************************************************   
 
  S.A.C. Project (Automatic Cropping Systems) http://sacultivo.com started originally by Adrian Navarro.
  
  This file contains all the functions about the sensors.

  Author: Victor Suarez Garcia<suarez.garcia.victor@gmail.com>
  Co-Author: David Cuevas Lopez<mr.cavern@gmail.com>

  *** License:
  
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
  
  *** Version History:
  
  0.1) Initial Version.
  0.2) Added cache struct and state struct.
  0.3) Added Field Capacity Sensor Read.

*******************************************************************************************************/

#define DS19S20_PIN 17 // Soil Temperature PIN
#define SM_POWER_PIN 2 // Soil Moisture AND Field Capacity POWER PIN (ELECTRODES)
#define SM_PIN A1 // Soil Moisture PIN
#define NoWater_PIN 3 // Water available PIN (Water Level Tank or Presostate).
#define WaterFlow_PIN 16 // Water Flow PIN
#define FC_PIN A0 // Field Capacity PIN

// This structure Store the Last Cached Data Sensor

typedef struct
{
	float cached_ST; // Last Soil Temperature
	int cached_STmin; // Last Minimum Temperature
	int cached_STMax; // Last Maximum Temperature
	int cached_SM; // Last Soil Moisture Value
	int cached_SMmin; // Last Minimum Soil Moisture
	int cached_SMOp; // Last Soil Moisture Optimum
	int cached_SMcalib; // Last Soil Moisture Calibration
	int cached_FC; // Last Field capacity
	int cached_PICicle;
	int cached_TICicle;
	
} cached_sensors;

// This Structure Store all the Information for the Current State

typedef struct
{
	float SMOp; // Soil Moisture Optimum
	float SMmin; //Soil Moisture minimum
	float current_SM; // current Soil Moisture
	float STMax; // Max Soil Temperature
	float STmin; // Min Soil Temperature
	float current_ST; // Current Soil Temperature
	float current_FC; // Current Field Capacity
	float SMcalib; // Soil Moisture Calibration

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
	ds.write(0x44, 1); // Start Conversion, with Parasite Power on at the End
	byte present = ds.reset();
	ds.select(addr);
	ds.write(0xBE); // Read Scratchpad
	for (int i = 0; i < 9; i++)   // We need 9 bytes
	{
		data[i] = ds.read();
	}
	ds.reset_search();
	byte MSB = data[1];
	byte LSB = data[0];
	float tempRead = ((MSB << 8) | LSB); // Using two's compliment
	float TemperatureSum = tempRead / 16;
	return TemperatureSum;
}

// Read the Soil Moisture.

void sm_power_pin()
{
	pinMode(SM_POWER_PIN, OUTPUT);
}

float read_SM(int sm_calib)
{
	digitalWrite(SM_POWER_PIN, HIGH);
	float cached_SM = analogRead(SM_PIN);
	digitalWrite(SM_POWER_PIN, LOW);      // INTRODUCIR Pulsos a los electrodos (2 opciones: a) Continuo y b) intervalo de 1' a 240') (Cuando esta en estado relay_on() pasa siempre a pulsos electr. en modo continuo)
	cached_SM =  map(cached_SM, 0, sm_calib, 0, 100);
	if(cached_SM >= 100)
		cached_SM = 100;
	float SM = cached_SM * 100;  // Revisar EN LA ECUACION? (current_config.SMcalib) float soil_moisture = cached_moisture * 100 / moisture_calib;
	return cached_SM;
}

// Read the Field Capacity.REVISAR. MISMA FUNCION Read the Soil Moisture, PERO A OTRA PROFUNDIDAD, LA CALIB ES LA MISMA.

float read_FC(int sm_calib)
{
	digitalWrite(SM_POWER_PIN, HIGH);
	float cached_FC = analogRead(FC_PIN);
	digitalWrite(SM_POWER_PIN, LOW);
	cached_FC =  map(cached_FC, 0, sm_calib, 0, 100);
	if(cached_FC >= 100)
		cached_FC = 100;
	float FC = cached_FC * 100;  // Revisar EN LA ECUACION? 
	return cached_FC;
}

// Read Available Water

void No_Water()
{
	pinMode(NoWater_PIN, INPUT);
        digitalRead(NoWater_PIN)==HIGH;
}

// Update the Sensors Cache with the Current State

void update_State(cached_sensors & last_values, int SM)
{

        float curr_SM;
	curr_SM = read_SM(SM);
	last_values.cached_SM = curr_SM;

        float curr_FC;
	curr_FC = read_FC(SM); //???
	last_values.cached_FC = curr_FC;

      	float curr_ST = read_ST();
	last_values.cached_ST = curr_ST;
	long time_aux = millis();
}

// Get the Current State from the Cached Values

State read_sensors(cached_sensors & last_values)
{
	State current_state;
	current_state.SMOp = last_values.cached_SMOp;
        current_state.SMmin = last_values.cached_SMmin;
	current_state.SMcalib = last_values.cached_SMcalib;
	current_state.current_SM = last_values.cached_SM;
        current_state.current_FC = last_values.cached_FC;
	current_state.current_ST = last_values.cached_ST;
	current_state.STMax = last_values.cached_STMax;
	current_state.STmin = last_values.cached_STmin;
	return current_state;
}

cached_sensors initSensorsCache()
{
	cached_sensors current_sensors;
	current_sensors.cached_SM = 0;
        current_sensors.cached_FC = 0;
	current_sensors.cached_SMcalib = 0;
	current_sensors.cached_STmin = 0;
	current_sensors.cached_STMax = 0;
	current_sensors.cached_SMmin = 0;
	current_sensors.cached_SMOp = 0;
	return current_sensors;
}

// Reading SM Sensor for calibration (Sample Soil Moisture 100% Saturation)

int readSMcalib()
{
	digitalWrite(SM_POWER_PIN, HIGH);
	int cached_SMcalib = analogRead(SM_PIN);
	digitalWrite(SM_POWER_PIN, LOW);
	return cached_SMcalib;
}

// Revisar cambio de estado en la lectura de los sensores ???

boolean state_changed(State state1, State state2)
{
	if(state1.current_SM != state2.current_SM ) return true;
	if(state1.current_ST != state2.current_ST ) return true;
	if(state1.current_FC != state2.current_FC ) return true;
	return false;
}
