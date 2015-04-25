#include <EEPROM.h>
/****************************************************************
EEPROMUtils: this file contains all the information about the Configuration
 for the SAC Project:
  http://sacultivo.com
  
  IN this file there is the functions for use the EEPROM for store the 
  configuration.
  
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
 * 0.2. Change the Configuration System by using an struct for store the configuration
 in the EEPROM.
 * Current Version: 0.2.
*************************************************************************/
  
 #define CONFIG_START 25 //Initial Address in the EEPROM.
/*Struct with the configuration */
typedef struct{
  int Relay;//Relay
  float SMOp; //Soil Moisture Optimum
  float SMmin;//Soil Moisture minimum
  float STMax;//Maximum Soil temperature
  float STmin;//Minimum Soil Temperature
  int PICicle;//Percentaje Irrigation Cicle.
  float SM_calib;//SM calibration
  int check_interval; //Interval Check for update the Sensors
  int flow_diameter;//Flow Diameter for Flow Size Sensor
  int active_languaje;//Active Languaje
  int interval_time;//IntervalTime
  int TICicle;
  
}Configuration;

/*
 get the default configuration.
  returns: Default Configuration.
 */
Configuration getDefaultConfig(){
 Configuration default_config;
  default_config.Relay=0;
  default_config.SMOp=60;
  default_config.SMmin=40;
  default_config.STMax=36.0;
  default_config.STmin=6.0;
  default_config.TICicle=6;
  default_config.PICicle=50;
  default_config.check_interval=15;
  default_config.flow_diameter=16;
  default_config.active_languaje=0;
  default_config.SM_calib=500;

  return default_config;
}
/*
 * store the Settings in the EEPROM
 * Parameters:
 * Settings: The Settings to Store.
 * Returns: The number of bytes writed*/  
int store_Settings(Configuration & settings)
{
   int e=CONFIG_START;
   int i=0;
   byte* data=(byte *)&settings;
   for(i=1;i<sizeof(settings);i++){
     byte b=data[i];
     EEPROM.write(e+i,b);
   }
   return i;
}
/*
* loads the Settins form The EEPROM.
* Parameters
* settings: The settings struct for load the information from the EEPROM.
* returns: number of bytes readed.*/
int load_Settings(Configuration & settings)
{
  int e=CONFIG_START;
   int i=0;
   byte* data=(byte *)&settings;
   for(i=1;i<sizeof(settings);i++){
     data[i]=EEPROM.read(e+i);
   }
   memcpy(&settings,data,sizeof(settings));
   return i;
}
/**
* loads the default configuration, and store it in the EERPROM
* Returns: default configuration settings.
*/
Configuration reset_Settings(){
  Configuration current_config=getDefaultConfig();
  store_Settings(current_config);
  return current_config;
}
