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

// SOIL TEMPERATURE PIN

#define DS19S20_PIN 13

// SM POWER PIN
 
#define SM_POWER_PIN 5

//SM PIN

#define SM_PIN A3

// WATER AVAILABLE PIN (WATER LEVEL TANK OR PRESOSTATE)
 
#define AWater_PIN A7

// WATER FLOW PIN

#define WaterFlow_PIN 16

//FIELD CAPACITY PIN

#define FC_PIN A0


#define SM_SMOOTHING 85
#define SM_CALIB 500



volatile int NbTopsFan;
int Calc;
/**
* Read the field capacity in function of the previous calibration.
* Parameters:
* FC_calib: field capacity calibration; the maximum value where the field is saturated.
* returns true if the field is saturated or false otherwise
*/
boolean readFieldCapacity(int FC_calib){
         
         digitalWrite(SM_POWER_PIN, HIGH);
  
  int cached_fc = analogRead(FC_PIN);
  digitalWrite(SM_POWER_PIN, LOW);
          int FC=map(cached_fc,0,FC_calib,0,100);
   return FC>=100;
}

void npm();

// this struct store the last cached data sensor.

typedef struct {
  int cached_AWater;//Last AWater
  float cached_flowvolume; //Water Flow
  int cached_STmin;//Last Minimum Temperature
  int cached_STMax;//Last Maximum Temperature
  int cached_SM;//Last Soil Moisture
  int cached_SMmin;//Last Minimum Temperature
  int cached_SMOp;//Last Maximum Temperature
  int cached_waterFlowdiameter;//Water Flow Diameter
  boolean cached_fieldCapacity;//last result for the field capacity
  tmElements_t cached_lastWaterEvent;//Last Time and Date for Pumping.
  int cached_PICicle;
  int cached_TICicle;
  

} cached_sensors;

// This Struct store all the information for the current state.

typedef struct {

	// Soil Moisture Optimum
	
	float SMOp;
	
        //Soil Moisture minimum
	 
	float SMmin;
	
        // current Soil Moisture
	 
	float current_SM;
	
        // Max Soil Temperature
	
	float STMax;
	
        // Min Soil Temperature
	
	float STmin;
	
        //Current Soil Temperature
	
	float current_ST;
	
        //Current Water Consumption in m3
	
	float consumption;
        boolean FC;
        

}State;


enum Interval_state{
  I_CONTINOUS=0,
  I_INTERVAL
};
/**
 *
 *read the Soilt Temperature
 * returns: the Soil Temperature in Celsius.
*/ 
float read_ST()
{
  OneWire ds(DS19S20_PIN);  // on pin A1
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;
}
 /*
 *
 * Read the Soil SM.
 * returns Soil SM in PICicle.
 */
 float read_SM(int hs_calib){
   
  digitalWrite(SM_POWER_PIN, HIGH);
  
  float cached_SM = analogRead(SM_PIN);
  digitalWrite(SM_POWER_PIN, LOW);
  cached_SM=  map(cached_SM,0,hs_calib,0,99);
  if(cached_SM>=100)
    cached_SM=99;
   static float kept=0;
   float soil_SM= cached_SM *100/ SM_CALIB;
   kept=(kept* SM_SMOOTHING/100.0);
   kept=kept+soil_SM*(100-SM_SMOOTHING)/100.0;
   
  return cached_SM;
 }
 
 


 
 
 /*
 *
 * Read the AWater Tank
 * return true if tank have water or false if not.
 */
 boolean getAWater(){

	int AWater=analogRead(AWater_PIN);
	int WaterUp=map(AWater,0,602,0,1);
	//Note When We Want analog Water results, change map function
	return WaterUp>0;
}
 void setupFlowRate()
 {
   NbTopsFan=0;
   pinMode(WaterFlow_PIN, INPUT);
   attachInterrupt(0,npm,RISING);
 }
 
 void npm()
{
  NbTopsFan++;
}
float getWaterFlowRate ()
{
    
  sei();      //Enables interrupts
  delay (1000);   //Wait 1 second
  cli();      //Disable interrupts
  Calc = (NbTopsFan * 7 / 60); //(Pulse frequency x 60) / 7Q, = flow rate in L/min
  return Calc;
  /*  Serial.print (Calc, DEC); //Prints the number calculated above*/
}




/*
 *
 * update the sensors cache with the current state.
 * Parameters:
 * last_values : last sensors values.
 * tmElements: current Time.
 * fcCapacityCalib: field Capacity calibration.
 */

void update_State(cached_sensors & last_values,tmElements_t current_event,int FCCalib,byte mode,long lastInterval, boolean& cerrojo_intervalo, long& IntervalTime)
{
            float curr_SM;
            
            if( mode==I_INTERVAL){         
            long time_aux=millis();

            if(cerrojo_intervalo==HIGH ){
            curr_SM=read_SM(FCCalib);
            last_values.cached_SM=curr_SM;
            cerrojo_intervalo=LOW;
            IntervalTime=millis();
            }
            if(cerrojo_intervalo==LOW && ( time_aux - IntervalTime>=15000*60)){
            cerrojo_intervalo=HIGH;
            } 
            }          
            if(mode==I_CONTINOUS){
            curr_SM=read_SM(FCCalib);
            last_values.cached_SM=curr_SM;
            }
            
	    float curr_ST= read_ST();
            
            
	    float curr_flowrate=0.0;//getWaterFlowRate();
    
    
            

	    last_values.cached_AWater = 0.0;//getAWater(); // Boolean indicates if we have water or not.
	    last_values.cached_flowvolume+=curr_flowrate/60000;//FlowRate(L/m) to FlowRate(m3/s).
	    last_values.cached_fieldCapacity=readFieldCapacity(FCCalib);

}
/*
*
* get the current state from the cached values.
* Parameters:
* last_values: cache for sensors values.
* returns: current state.
*/
State read_sensors(cached_sensors & last_values)
{
 State current_state;


	/* set the state */
	current_state.SMOp=last_values.cached_SMOp;
	current_state.SMmin=last_values.cached_SMmin;

        current_state.current_SM=last_values.cached_SM;
	current_state.consumption=last_values.cached_flowvolume;
	current_state.SMOp=last_values.cached_SMOp;
	current_state.STMax=last_values.cached_STMax;
	current_state.STmin=last_values.cached_STmin;
	current_state.FC=last_values.cached_fieldCapacity;

        
        
       
	return current_state; 
  
}

cached_sensors initSensorsCache(){
  cached_sensors current_sensors;
   current_sensors.cached_SM=0.0;
   current_sensors.cached_AWater = 0.0;//getAWater(); // Boolean indicates if we have water or not.
   current_sensors.cached_flowvolume+=0.0;//FlowRate(L/m) to FlowRate(m3/s).
   current_sensors.cached_fieldCapacity=true;
   current_sensors.cached_STmin=0;
   current_sensors.cached_STMax=0;
   current_sensors.cached_SMmin=0;
   current_sensors.cached_SMOp=0;
   

   return current_sensors;
}
int readFCValue()
{
  digitalWrite(SM_POWER_PIN, HIGH);
  
  int cached_fc = analogRead(SM_PIN);
  digitalWrite(SM_POWER_PIN, LOW);
  
  return cached_fc;
}
boolean state_changed(State state1,State state2)
{
   if(state1.current_SM!= state2.current_SM ) return true;
   if(state1.current_ST!= state2.current_ST ) return true;
   if(state1.FC!= state2.FC ) return true;
 
   return false;
}
