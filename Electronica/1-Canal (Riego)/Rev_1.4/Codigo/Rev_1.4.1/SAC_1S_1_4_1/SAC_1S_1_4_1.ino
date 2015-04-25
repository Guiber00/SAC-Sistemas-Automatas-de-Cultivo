/****************************************************************
 * SAC: This is the Main File of SAC Project:
 * http://sacultivo.com
 * 
 * In this file we can see all the functions for the correct functionality of the SAC Unit for 1 Output Channel
 * 
 * Author: Victor Suarez Garcia<suarez.garcia.victor@gmail.com>
 * Co-Author: David Cuevas Lopez<mr.cavern@gmail.com>
 * 
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
 * 1.1. Initial Version. Written by Øyvind Kolås pippin@gimp.org in 2013.
 * 1.2. Some Changes for more readablity. written by victor suarez suarez.garcia.victor@gmail.com and David Cuevas mr.cavern@gmail.com in March 2014.
 * 1.3. Improved all the Functionality for 20x4 Screen and improved for 1.3 PCB Version. written by victor suarez suarez.garcia.victor@gmail.com and David Cuevas mr.cavern@gmail.com in April 2014
 * 
 * 
 * Current Version: 1.4.1
 *************************************************************************/

/* PARAMETERS

  SM=Soil Moisture
  SMOp=Soil Moisture Optimun
  SMmin=Soil Moisture minimun
  FC=Field Capacity
  STMax=Soil Temperature Maximun
  STmin=Soil Temperature minimun
  TICicle=Total Irrigation Cicle (seconds)
  PICicle=Percentaje Irrigation Cicle (%)
  
 */

#include <SoftwareSerial.h>
#include <SerLCD.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DS1307RTC.h>
#include <Wire.h>
#include <Time.h>
#include "RTCUtils.h"
#include "EEPROMUtils.h"
#include "Languages.h"
#include "SACSensors.h"
#include "Relay.h"

// LCD CONFIG & PINS
 
#define LCD_PIN 11
#define NUM_COLS 20
#define NUM_ROWS 4
#define MAXMENUITEMS 6
#define magia(lcd,x) {  lcd.setPosition(1,0); lcd.print(x);delay(5000);}
#define printTitle(lcd,m){lcd.print("***");lcd.print(m);lcd.print("***");}

// BUTTONS PINS

#define BUTTON_UP_PIN 9
#define BUTTON_CENTER_PIN 8
#define BUTTON_DOWN_PIN 7
#define BUTTONUP 0
#define BUTTONDOWN 1
#define BUTTONCENTER 2
#define BUTTONCENTERLONG 3

#define TIMEOUT 6
#define IDLE -1
#define TOTALTIMEOUTTIME 10000
#define VERSION 1.4.1

// DIFFERENT STATES TO MOVE THROUGH MENU
 
enum States
{
  INICIO,
  CALIB_SAT,
  ESTADO,
  EDICION,
  CONFIG_MENU,

};

// DIFFERENT STATES IN SELECTION MODE
 
enum CONFIG_MENU_States
{
  MENU,
  CALIBRACION_SAT,
  RESET_CONFIG,
  ABOUT,
  END_SELECTION
};

enum s_selectStatus
{
  S_SMOp,
  S_SMmin,
  S_TICicle,
  S_PICicle,
  S_TSMAX,
  
};

// CONFIGURATION STRUCTURE 
 
typedef struct MenuItem
{
  int label;
  int  state; 
};
MenuItem main_menu[] ={
  {
    S_SATCALIBRATION,CALIBRACION_SAT          }
  ,
  {
    S_RESET,RESET_CONFIG          }
  ,{
    S_ABOUT,ABOUT      }
  ,{
    S_RETURN_TO,END_SELECTION          }

};

int current_menu;

int select_language;


// LCD PIN SETUP & CONFIG

SoftwareSerial SSerial(0,LCD_PIN);
SerLCD mylcd(SSerial,NUM_COLS,NUM_ROWS);

// VALUES, STATES & CONFIG

boolean refresh_LCD;
Configuration current_config;
tmElements_t tm;
cached_sensors current_sensorsvalues;
State current_state;
State previous_state;

// RELAY CONFIG
 
Relay relay[MAX_RELAYS]={
  {
    RELAY1_PIN,RELAY_OFF       }
};

// GLOBAL VARIABLES

tmElements_t lastUpdate;
boolean irrigating;
byte current_mstate;
byte current_selectionstate;
byte current_selectionDateState;
byte button_up_state=LOW;
byte button_down_state=LOW;
byte button_center_state=LOW;
byte center_pressed_state=0;
byte up_pressed_state=0;
int selectionStatus=S_SMOp;
byte interval_mode=I_INTERVAL;
int current_rele=0;
boolean isEditing;
long time1;
long time2;
long time1I;
long time2I;
long time3I;
long IntervalTime;
long lastEvent;

boolean cerrojo_up=1;
boolean cerrojo_center=1;
boolean cerrojo_down=1;
boolean cerrojo_intervalo=HIGH;

//SAC LCD saclcd(mylcd);

void setup_pins();
void static drawSelectStatus(State & state);

void setup()
{
  Serial.begin(9600);
  SSerial.begin(9600);
  mylcd.begin();
  setup_pins();
  refresh_LCD=true;

// setupFlowRate();

  irrigating=false;
  current_selectionstate=MENU;
  if(!load_Settings(current_config)){
    current_config=reset_Settings();
    current_mstate=CONFIG_MENU;
  }
  else{
    current_mstate=ESTADO; 
  }
  initializeGlobalVars();
}
// Initialize Global Variables.

void initializeGlobalVars(){
  active_language =current_config.active_languaje;
  current_sensorsvalues.cached_STMax=(current_config.STMax!=0)?current_config.STMax:36;
  current_sensorsvalues.cached_STmin=(current_config.STmin!=0)?current_config.STmin:4;
  current_sensorsvalues.cached_SMmin=(current_config.SMmin!=0)?current_config.SMmin:40;
  current_sensorsvalues.cached_SMOp=(current_config.SMOp!=0)?current_config.SMOp:60;
  current_sensorsvalues.cached_TICicle=(current_config.TICicle!=0)?current_config.TICicle:6;
  current_sensorsvalues.cached_PICicle=(current_config.PICicle!=0)?current_config.PICicle:50;
  time1=millis();
}

//setup SAC Pins.

void setup_pins(){
  pinMode(BUTTON_UP_PIN,INPUT);
  pinMode(BUTTON_DOWN_PIN,INPUT);
  pinMode(BUTTON_CENTER_PIN,INPUT);

  pinMode(SM_POWER_PIN, OUTPUT);
  for(int i=0;i<MAX_RELAYS;i++)
  {
    pinMode(relay[i].gpio_pin,OUTPUT);

  }
  current_config.interval_time=15;
  isEditing=false;
}
void loop(){
  RTCread(tm);

// ALWAYS UPDATE SCREEN WHEN STATE CHANGES

  update_State(current_sensorsvalues,tm,current_config.calib_FC,interval_mode, current_config.interval_time, cerrojo_intervalo, IntervalTime);
  current_state=read_sensors(current_sensorsvalues);
  if(current_mstate==ESTADO){
    if(state_changed(current_state,previous_state) || time_between(lastUpdate,tm)>1){
      refresh_LCD=true; 
      previous_state=current_state;
      lastUpdate=tm;
    }
    
  }

  drawUI(current_state);
  int event=get_event();
  handleEvent(event);
  update_relay_state();
  
}

/*Gets the current Button Event
 * returns: the button that is pressed.
 *  0: button UP is Pressed.
 *  1: button DOWN is Pressed.
 *  2: button CENTER is Pressed minor than 3 seconds.
 *  3: button CENTER is Pressed more than 3 seconds.
 */

int get_event(){
  int event=IDLE;
  event=button_up_pressed();

  if(event!=IDLE){
  
//  Serial.println(event); 

    return event;
  }
  event=button_down_pressed();

//Serial.println(event);

  if(event!=IDLE){ 

// Serial.println(event); 

    return event;
  }
  time2=millis();
  event=button_center_pressed();
  if(event>=0){
    time1=millis(); 
  }
  if((time2-time1)>=TOTALTIMEOUTTIME){
    time1=millis();
    event= TIMEOUT;
  }
  Serial.println(event);
  return event;

}

/*
* Check if button up is pressed. The Event only ocurrs when the button is released; not
 * when button is pressed.
 returns HIGH when button Up is released or LOW otherwise.
 */

int button_up_pressed()
{
  byte current_state=digitalRead(BUTTON_UP_PIN);

  if(current_state==HIGH && cerrojo_up==1)
  {  
    cerrojo_up=0;
    time1I=millis();
    return BUTTONUP;
  }
  if(current_state==LOW && cerrojo_up==0)
  {
    cerrojo_up=1;
    time1I=millis();
  }

  return IDLE;

}

/*
* check if button down is pressed. The Event only ocurrs when the Button is released. NOt when
 * the button is Pressed.
 * returns HIGH when the Button is released or LOW otherwise.
 */

int button_down_pressed()
{
  boolean current_state=digitalRead(BUTTON_DOWN_PIN);

  if(current_state==HIGH && cerrojo_down==1){
    time3I=millis();
    cerrojo_down=0;
    return BUTTONDOWN;
  }

  if(current_state==LOW && cerrojo_down==0 ){
    cerrojo_down=1;   
  }

    return IDLE;
}

/*
* check if button center is pressed with two states. When is pressed minor than 3 secods or if its pressed
 more than 3 seconds. The event only ocurr when the button is released. not when is pressed.
 returns: -1 if is not released, 2 if button is pressed minor than three seconds or 3 if  button is pressed
 more than 3 seconds.
 */

int button_center_pressed()
{
  boolean current_state=digitalRead(BUTTON_CENTER_PIN);

  if(current_state==HIGH && cerrojo_center ==1)
  {  
    cerrojo_center=0;
    time2I=millis();
    return BUTTONCENTER;
  }
  if(current_state==LOW && cerrojo_center==0)
  {
    cerrojo_center=1;
    time2I=millis();

  }

  if(current_state==HIGH && cerrojo_center==0){
    long TIME_center=millis();
    if(TIME_center-time2I>= 3000){
      time2I=millis();
      return BUTTONCENTERLONG;
    }
  }
  return IDLE;

}

/*
 * HANDLE EVENTS FOR STATE SCREEN & SELECT MODE
 * CASE ESTADO: STATUS SCREEN, DEFAULT VIEW.
 * CASE CONFIG_MENU: SELECTION STATE, CALLS THE EVENT HANDLER FOR THIS STATE
 */

void handleEvent(int event)
{
  if(event==BUTTONCENTERLONG){
    mylcd.clear();
    current_mstate=CONFIG_MENU;
    current_menu=0; 
    refresh_LCD=true;
    mylcd.underlineCursorOff();
    mylcd.boxCursorOff();
    return;
  }
  switch(current_mstate)
  {
  case ESTADO:

    if(event==BUTTONCENTER)
    {
      mylcd.clear();
      current_mstate=EDICION;
      refresh_LCD=true;
      selectionStatus=S_SMOp;
    }

    break; 
  case CONFIG_MENU:

    handleEventSelection(event);

    break;
    }
}

void handleEventSelectStatus(int event)
{
  if(event==BUTTONCENTER)
  {
    isEditing=!isEditing;
    refresh_LCD=true; 
  }
  if(!isEditing){
    if(event==BUTTONDOWN)
    {
      selectionStatus--;
      if(selectionStatus<0){
        selectionStatus=5;
      }
      refresh_LCD=true;
    } 
    if(event==BUTTONUP)
    {
      selectionStatus++;
      if(selectionStatus>5){
        selectionStatus=0;
      }
      refresh_LCD=true;
    }

    if(event==TIMEOUT)
    {

      selectionStatus=S_SMOp;
      current_mstate=ESTADO;
      refresh_LCD=true;
      mylcd.clear();
      mylcd.boxCursorOff(); 

//Store current state settings

      current_config.SMOp=current_sensorsvalues.cached_SMOp;
      current_config.SMmin=current_sensorsvalues.cached_SMmin;
      current_config.STMax=current_sensorsvalues.cached_STMax;
      current_config.STmin=current_sensorsvalues.cached_STmin;
      current_config.PICicle=current_sensorsvalues.cached_PICicle;
      current_config.TICicle=current_sensorsvalues.cached_TICicle;
      store_Settings(current_config);
    }

  }
  else{

    switch(selectionStatus)
    {
    case S_SMOp:
      if(event==BUTTONDOWN){
        current_sensorsvalues.cached_SMOp--;
        if(current_sensorsvalues.cached_SMOp<current_sensorsvalues.cached_SMmin){
          current_sensorsvalues.cached_SMOp=99;
        }

        refresh_LCD=true;
      }
      if(event==BUTTONUP){
        current_sensorsvalues.cached_SMOp++;
        if(current_sensorsvalues.cached_SMOp>99){
          current_sensorsvalues.cached_SMOp=current_sensorsvalues.cached_SMmin;
        }
      break;

    case S_SMmin:
      if(event==BUTTONDOWN){
        current_sensorsvalues.cached_SMmin--;
        if(current_sensorsvalues.cached_SMmin<0){
          current_sensorsvalues.cached_SMmin=100;
        }
        refresh_LCD=true;
      }
      
      if(event==BUTTONUP){
        current_sensorsvalues.cached_SMmin++;
        if(current_sensorsvalues.cached_SMmin>99){
          current_sensorsvalues.cached_SMmin=0;
        }       
        refresh_LCD=true;
      }
      
      }
      break;

    case S_PICicle:
      if(event==BUTTONDOWN)
      {
        current_sensorsvalues.cached_PICicle--;
        if(current_sensorsvalues.cached_PICicle<0){
          current_sensorsvalues.cached_PICicle=100;
        }   
        refresh_LCD=true;
      }
      if(event==BUTTONUP)
      {
        current_sensorsvalues.cached_PICicle++;
        if(current_sensorsvalues.cached_PICicle>100){
          current_sensorsvalues.cached_PICicle=0;
        }   
        refresh_LCD=true;  
      }
           
      break;
      
    case S_TSMAX:
      if(event==BUTTONUP){
        current_sensorsvalues.cached_STMax++;
        if(current_sensorsvalues.cached_STMax>55){
          current_sensorsvalues.cached_STMax=0;
        } 
        refresh_LCD=true;
      }
      if(event==BUTTONDOWN){
        current_sensorsvalues.cached_STMax--;
        if(current_sensorsvalues.cached_STMax<0){
          current_sensorsvalues.cached_STMax=55;
        } 
        refresh_LCD=true;
      }
           
      break;

   case S_TICicle:
  if(event==BUTTONUP){
        current_sensorsvalues.cached_TICicle++;
        if(current_sensorsvalues.cached_TICicle>59){
          current_sensorsvalues.cached_TICicle=0;
        } 
        refresh_LCD=true;
      }
      if(event==BUTTONDOWN){
        current_sensorsvalues.cached_TICicle--;
        if(current_sensorsvalues.cached_TICicle<0){
          current_sensorsvalues.cached_TICicle=59;
        } 
        refresh_LCD=true;
        
          }
    
  break;
    }
  }

}

/*
 * EVENT HANDLER FOR EACH SELECTION STATE
 * MENU: MOVE & INTERACT WITH MENU
 * IDIOMA: LANGUAGE SELECTION MENU
 * FECHA: DATE CONFIG MENU
 * HORA: TIME CONFIG MENU
 * END SELECTION: EXITS SELECTION STATE
 */
void handleEventSelection(int event)
{
  switch(current_selectionstate)
  {
  case MENU:

    if(event == BUTTONCENTER)
    {
      mylcd.clear();
      current_selectionstate=main_menu[current_menu].state;
      select_language=0;
      refresh_LCD=true;
      isEditing=false;
          }
    if(event==BUTTONDOWN)
    {
      mylcd.clear();
      current_menu++;
      current_menu= current_menu%MAXMENUITEMS;
      refresh_LCD=true;
    }
    if(event==BUTTONUP)
    {
      mylcd.clear();
      current_menu=(current_menu==0)? MAXMENUITEMS-1: current_menu-1;
      current_menu= current_menu%MAXMENUITEMS;
      refresh_LCD=true;
    }
    break;
  
  case CALIBRACION_SAT:
    if(event==BUTTONCENTER)
    {
      int calib=readFCValue();
      current_config.calib_FC=calib;
      store_Settings(current_config);
      current_selectionstate=MENU;
      interval_mode=I_INTERVAL;
      mylcd.clear();
    }else{
     interval_mode=I_CONTINOUS;
    }
    
    refresh_LCD=true;
    break;
  
   case RESET_CONFIG:

    current_config=reset_Settings();
    initializeGlobalVars();
    current_mstate=CONFIG_MENU;
    current_selectionstate=MENU;
    refresh_LCD=true;
    break;
  
   case ABOUT:
    if(event==BUTTONCENTER)
    {

      current_selectionstate=MENU;
      refresh_LCD=true;
      mylcd.clear();
    }
    break;
  
   case END_SELECTION:
    current_mstate=ESTADO;
    current_selectionstate=MENU;
    mylcd.clear();
    refresh_LCD=true;
    break;
  }
}

// DRAWS  INTERFACE IN STATUS MODE & SELECTION MODE

void drawUI(State & state){
  if(refresh_LCD){
    switch(current_mstate)
    {
    case CONFIG_MENU:
      drawCONFIG_MENU();
      break;
    case  ESTADO:

      drawState(state);
      break;
    case EDICION:
      drawSelectStatus(state);
      break;

    }
    refresh_LCD=false;
  }

}
void drawState(State & state)
{
  switch(current_rele)
  {
  case 0:
    drawIrrigationState(state);
    break;
  }
}

// DRAWS INTERFACE FOR THE DIFFERENT MENUS

void drawCONFIG_MENU()
{

  switch(current_selectionstate)
  {
  case MENU:
    drawMenu();
    break;
  
  case CALIBRACION_SAT:
    drawCalibrationSat();
    break;
  case ABOUT:
    drawAbout();
    break;
  }

}

// DRAWS MENU

void drawMenu()
{
  mylcd.setPosition(1,0);
  printTitle(mylcd,translate(S_CONFIGURATION));
  int position=2;

  for(int i=current_menu; i<(current_menu+3) && i<MAXMENUITEMS;i++)
  {

    mylcd.setPosition(position,0);
    if(i==current_menu)
    mylcd.print("-");
    mylcd.print(translate(main_menu[i].label)); 
    position++;

//  Serial.println(i);

  }
}

/**
 * draws the current state at LCD Screen
 * state: current state of sensors.
 */
void drawIrrigationState(State & state)
{
 
//Line1

  mylcd.setPosition(1,0);
  mylcd.print(translate(S_SM));
  if(state.SMOp<10)
    mylcd.print("0");
  mylcd.print((int)state.SMOp);
  mylcd.print(" ");
  mylcd.print(translate(S_SMm));
  if(state.SMmin<10)
    mylcd.print("0");
  mylcd.print((int)state.SMmin);
  mylcd.print(" ");
  mylcd.print("[");
  if(state.current_SM<10)
    mylcd.print("0");
  mylcd.print((int)state.current_SM);
  mylcd.print("");
  mylcd.print("]%");
  if(state.current_SM<=99)
    mylcd.print(" ");

//line2

  mylcd.setPosition(2,0);
  mylcd.print(translate(S_CICLE));
  byte seconds= current_sensorsvalues.cached_TICicle;
  if(seconds<10)
    mylcd.print("0");
  mylcd.print(seconds);
  mylcd.print("''");
  mylcd.print("ON");
  if(current_sensorsvalues.cached_PICicle<100)
    mylcd.print("0");
  if(current_sensorsvalues.cached_PICicle<10)
    mylcd.print("0");
  mylcd.print(current_sensorsvalues.cached_PICicle);
  mylcd.print("%");

//line3

  mylcd.setPosition(3,0);
  mylcd.print(translate(S_STMax));
  if(state.STMax<10)
    mylcd.print("0");
  mylcd.print((int)state.STMax);
  mylcd.print(" ");
  int currenttemp=state.current_ST;
  if(currenttemp>=0){
    mylcd.print("+"); 
  }
  else{
    if(currenttemp==-1000){
      mylcd.print("-");
    } 
  }
  if(currenttemp!=-1000){
    mylcd.print(currenttemp);
  }
  else{
    mylcd.print("--"); 
  }
  mylcd.print("C");
  
  
//line4
  
 
}



// ALWAYS LISTENS TO RELAYS STATES IN EACH ROLE

void update_relay_state (void)
{
  byte i;
  boolean relaystate=false;
  for (i=0; i < MAX_RELAYS; i++){


    Relay rele = relay[i];
    
      relaystate=false;
      if(!current_state.FC){

        if (current_state.current_ST==-1000 ||( current_state.current_ST < current_state.STMax))
        {
          if (rele.state==RELAY_OFF && current_state.current_SM <= current_state.SMmin )
          {

            rele.state=RELAY_ON;
            relaystate=true;
            lastEvent=millis();
            interval_mode=I_CONTINOUS;
          }
          else{
            if ((rele.state==RELAY_ON || rele.state==RELAY_WAIT) && (current_state.current_SM <= current_state.SMOp) )
            {
               if(!checkPumpCicle(relaystate,lastEvent)){
                  if(rele.state==RELAY_ON){
                  relaystate=false;
                  rele.state=RELAY_WAIT;

                  }else{
                    relaystate=true;
                  rele.state=RELAY_ON;
                  }
                  lastEvent=millis();
               }else{
               if(rele.state==RELAY_ON){
                  rele.state=RELAY_ON;
                  relaystate=true; 
               }else{
                 relaystate=false;
                  rele.state=RELAY_WAIT;
                  
               }
               }
            }
          }
        }
        if(current_state.current_SM >= current_state.SMOp){
          interval_mode=I_INTERVAL;
        }
        
      }
      else{
        rele.state=RELAY_OFF;
        relaystate=false; 
        interval_mode=I_INTERVAL;
      }
      if(relaystate)
      {
        //rele.state=RELAY_ON;
        relay_on(relay[i].gpio_pin);
      }
      else
      {
       // rele.state=RELAY_OFF;
        relay_off(rele.gpio_pin);

      }
      relay[i]=rele;
      break; 
        
    }
  }


/*
 * CHECK IRRIGATION CICLE DURATION.
 * IRRIGATION IS CONFIGURABLE TO BE ON CONTINOUS MODE OR AT INTERVALS
 * BY INDICATING PUMP PICicleAGE.
 * EXAMPLE: PUMP DURING FOUR MINUTES AT 50%. PUMPING WOULD BE ONE MINUTES ON & ONE MINUTES OFF, UNTIL COMPLETION OF THE CICLE OF FOUR MINUTES;
 */
boolean checkPumpCicle(boolean irrigating,long lastEvent){
  double totalSeconds=current_sensorsvalues.cached_TICicle;
  totalSeconds= (totalSeconds * current_sensorsvalues.cached_PICicle)/100;
  if((millis()-lastEvent)<totalSeconds*1000) return true;
  else return false;
  
} 

void drawCalibrationSat()
{
  mylcd.setPosition(1,0);
  printTitle(mylcd,translate(S_SATCALIBRATION));
  mylcd.setPosition(2,0);
  mylcd.print(translate(S_CURRENTVALUE));
  mylcd.print(F(": "));
  mylcd.print(readFCValue());
  refresh_LCD=true;

}

void static drawAbout()
{
  mylcd.setPosition(1,0);
  printTitle(mylcd,translate(S_ABOUT));
  mylcd.setPosition(2,0);
  mylcd.print(translate(S_SAC));
  mylcd.setPosition(3,0);
  mylcd.print(F("http://sacultivo.com"));
  mylcd.setPosition(4,0);
  mylcd.print(F("VERSION "));
  mylcd.print(VERSION);
} 

void static drawSelectStatus(State & state)
{

//Line1

  mylcd.setPosition(1,0);
  mylcd.print(translate(S_SM));
  if(state.SMOp<10)
    mylcd.print("0");
  mylcd.print((int)state.SMOp);
  mylcd.print(" ");
  mylcd.print("MIN:");
  if(state.SMmin<10)
    mylcd.print("0");
  mylcd.print((int)state.SMmin);
  mylcd.print(" ");
  mylcd.print("[");
  if(state.current_SM<10)
    mylcd.print("0");
  mylcd.print((int)state.current_SM);
  mylcd.print("");
  mylcd.print("]%");
  if(state.SMOp<=99)
    mylcd.print(" ");

//line2

  mylcd.setPosition(2,0);
  mylcd.print(translate(S_CICLE));
         
  byte seconds= current_sensorsvalues.cached_TICicle;
  if(seconds<10)
    mylcd.print("0");
  mylcd.print(seconds);
  mylcd.print("''");
  mylcd.print("ON");
  if(current_sensorsvalues.cached_PICicle<100)
    mylcd.print("0");
  if(current_sensorsvalues.cached_PICicle<10)
    mylcd.print("0");
  mylcd.print(current_sensorsvalues.cached_PICicle);
  mylcd.print("%");

//line3

  mylcd.setPosition(3,0);
  mylcd.print(translate(S_STMax));
  if(state.STMax<10)
    mylcd.print("0");
  mylcd.print((int)state.STMax);
  mylcd.print(" ");

  mylcd.print(" ");
  int currenttemp=state.current_ST;
  if(currenttemp>=0){
    mylcd.print("+"); 
  }
  else{
    if(currenttemp==-1000)
      mylcd.print("-"); 
  }
  if(currenttemp!=-1000){
    mylcd.print(currenttemp);
  }
  else{
    mylcd.print("--"); 
  }
  mylcd.print("C");
  mylcd.boxCursorOff();
  mylcd.underlineCursorOff();
  switch(selectionStatus)
  {
  case S_SMOp:

    if(!isEditing){
      mylcd.setPosition(1,2);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(1,5);
      mylcd.underlineCursorOn(); 
    }
    break;

  case S_SMmin:
    if(!isEditing){
      mylcd.setPosition(1,9);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(1,12);
      mylcd.underlineCursorOn(); 
    }
    break;
    
    case S_PICicle:
    if(!isEditing){
      mylcd.setPosition(2,15);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(2,19);
      mylcd.underlineCursorOn(); 
    }
    break;

    case S_TICicle:
  
    if(!isEditing){
      mylcd.setPosition(2,10);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(2,11);
      mylcd.underlineCursorOn(); 
    }
  break;
    
  case S_TSMAX:
    if(!isEditing){
      mylcd.setPosition(3,4);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(3,7);
      mylcd.underlineCursorOn(); 
    }
    break;
  
  }
  
}
