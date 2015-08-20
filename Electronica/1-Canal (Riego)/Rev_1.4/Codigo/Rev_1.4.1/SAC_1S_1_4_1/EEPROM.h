/*******************************************************************************************************  
  
  S.A.C. Project (Automatic Cropping Systems) http://sacultivo.com started originally by Adrian Navarro.
  
  This file contains all the information about the functions for use the EEPROM and store the configuration.

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
  0.2) Change the Configuration System by using an struct for store the configuration in the EEPROM.

*******************************************************************************************************/

#define CONFIG_START 25 // Initial Address in the EEPROM.

// Structure

typedef struct
{ 
  int SMOp; // Soil Moisture Optimum.
  int SMmin; // Soil Moisture minimum.
  int STMax; // Maximum Soil temperature.
  int STmin; // Minimum Soil Temperature.
  int SMcalib; // SM calibration.
  int PICicle; // Percentaje Irrigation Cicle.
  long TICicle; // Total Irrigation Cicle (seconds).
  int active_languaje; // Active Languaje.
} 

Configuration;

// Get the Default Configuration.

Configuration Default_Config()
{
	Configuration default_config;
	default_config.SMOp = 70;
	default_config.SMmin = 50;
	default_config.SMcalib = 500;
	default_config.STMax = 32;
	default_config.STmin = 4;
	default_config.TICicle = 6;
	default_config.PICicle = 50;
	default_config.active_languaje = 0;
	return default_config;
}

// Store the Settings in the EEPROM.

int store_Settings(Configuration & settings)
{
	int e = CONFIG_START;
	int i = 0;
	byte* data = (byte *)&settings;
	for(i = 1; i < sizeof(settings); i++)
	{
		byte b = data[i];
		EEPROM.write(e + i, b);
	}
	return i;
}

// Loads the Settings from the EEPROM.

int load_Settings(Configuration & settings)
{
	int e = CONFIG_START;
	int i = 0;
	byte* data = (byte *)&settings;
	for(i = 1; i < sizeof(settings); i++)
	{
		data[i] = EEPROM.read(e + i);
	}
	memcpy(&settings, data, sizeof(settings));
	return i;
}

// Loads the Default Configuration, and Store it in the EEPROM.

Configuration reset_Settings()
{
	Configuration current_config = Default_Config();
	store_Settings(current_config);
	return current_config;
}
