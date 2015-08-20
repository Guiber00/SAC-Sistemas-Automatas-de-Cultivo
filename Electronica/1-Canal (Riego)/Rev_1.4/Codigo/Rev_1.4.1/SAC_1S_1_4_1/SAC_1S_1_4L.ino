/*******************************************************************************************************

  S.A.C. Project (Automatic Cropping Systems) http://sacultivo.com started originally by Adrian Navarro.
 
  This is the main file of the software to run the funtionalities of the system (Smart Irrigation System), embebed into and Arduino Pro Mini board.
  
  *** Funtionality:

  *** Parameters Name:
 
  SM=Soil Moisture.
  SMOp=Soil Moisture Optimun.
  SMmin=Soil Moisture minimun.
  SMcalib=Soil Moisture Reading for calibrating the sensor.
  FC=Field Capacity.
  STMax=Soil Temperature Maximun.
  STmin=Soil Temperature minimun.
  TICicle=Total Irrigation Cicle (seconds).
  PICicle=Percentaje Irrigation Cicle (%).
 
  *** Version History:
 
  0.9) First prototype of the SAC System and first code aproach, by Victor Amo (@) in (-> Github).
  1.0) Second prototype with first sensors and LCD, coded by Andres Orencio (andy@orencio.org) in  (-> Github).
  1.1) First 3 channel version multirole prototype. Code written, by Øyvind Kolås (pippin@gimp.org) in June 2013  (-> Github).
  1.2 & 1.3) Some Changes for more readablity, improved all the functionality for 20x4 screen (PCB 1.3), by Victor Suarez (suarez.garcia.victor@gmail.com) and David Cuevas (mr.cavern@gmail.com) in March 2014.
  1.4) Optimized, debugged, cleaned, and improved the code from the previous version (1.3), to run the agronomical funtionalities implemented in PCB 1.4.1, by Adrian Navarro in May 2015

  [All PCBs designed by Adrian Navarro]
  
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

*******************************************************************************************************/

// Libraries.

#include <SoftwareSerial.h>
#include <SerLCD.h>
#include <EEPROM.h>
#include <OneWire.h>
#include "EEPROM.h"
#include "Languages.h"
#include "Sensors.h"

// Versions of Software and Hardware.

#define FIRMWARE 1.4
#define HARDWARE 1.42

// Global Variables.

Configuration current_config;
cached_sensors sensor_value;
State current_state;
State previous_state;
int current_menu;
int select_language;
byte current_menu_state;
byte current_screen;
boolean refresh_LCD;
boolean isEditing;
boolean SM_flag = 0;
boolean cerrojo_center = 1;
long time1;
long time2;
long time1I;
long time2I;
long time3I;

// LCD Configuration.

#define LCD_PIN 11
#define NUM_COLS 20
#define NUM_ROWS 4
#define MAXMENUITEMS 4
#define printTitle(lcd,m){lcd.print("---");lcd.print(m);lcd.print(":");}
SoftwareSerial SSerial(0, LCD_PIN);
SerLCD mylcd(SSerial, NUM_COLS, NUM_ROWS);

// RELAY Configuration.

#define RELAY_PIN 13

void relay_pin()
{
	pinMode(RELAY_PIN, OUTPUT);
}

void relay_on()
{
  digitalWrite(RELAY_PIN, HIGH);
}

void relay_off()
{
	digitalWrite(RELAY_PIN, LOW);
	LEDpump_off();
}

// LEDs Configuration.

#define LEDPump_PIN 4
#define LEDNoWater_PIN 6
#define LEDFieldC_PIN 5
int ledState_pumping = LOW;
int ledState_pump_waiting = LOW;
unsigned int previousMillis_LED_Pump = 0;
unsigned int previousMillis_LED_Pump_Waiting = 0;

void led_pins()
{
	pinMode(LEDPump_PIN, OUTPUT);
	pinMode(LEDNoWater_PIN, OUTPUT);
	pinMode(LEDFieldC_PIN, OUTPUT);
}

/*      Pumping - LED Flashing       */

int OnTime_LED_Pumping = 2000;
int OffTime_LED_Pumping = 200;

void LEDpumping()
{
	unsigned int currentMillis_LED_Pump = millis();
	if((ledState_pumping == HIGH) && (currentMillis_LED_Pump - previousMillis_LED_Pump > OnTime_LED_Pumping))
	{
		ledState_pumping = LOW;
		previousMillis_LED_Pump = currentMillis_LED_Pump;
		digitalWrite(LEDPump_PIN, ledState_pumping);
	}
	else if ((ledState_pumping == LOW) && (currentMillis_LED_Pump - previousMillis_LED_Pump > OffTime_LED_Pumping))
	{
		ledState_pumping = HIGH;
		previousMillis_LED_Pump = currentMillis_LED_Pump;
		digitalWrite(LEDPump_PIN, ledState_pumping);
	}
}

void LEDpump_off()
{
	digitalWrite(LEDPump_PIN, LOW);
}

/*      Pump Waiting - LED Flashing      */

int OnTime_LED_Pump_Waiting = 75;
int OffTime_LED_Pump_Waiting = 2500;

void LEDpump_waiting()
{
	unsigned int currentMillis_LED_Pump_Waiting = millis();
	if((ledState_pump_waiting == HIGH) && (currentMillis_LED_Pump_Waiting - previousMillis_LED_Pump_Waiting > OnTime_LED_Pump_Waiting))
	{
		ledState_pump_waiting = LOW;
		previousMillis_LED_Pump_Waiting = currentMillis_LED_Pump_Waiting;
		digitalWrite(LEDPump_PIN, ledState_pump_waiting);
	}
	else if ((ledState_pump_waiting == LOW) && (currentMillis_LED_Pump_Waiting - previousMillis_LED_Pump_Waiting > OffTime_LED_Pump_Waiting))
	{
		ledState_pump_waiting = HIGH;
		previousMillis_LED_Pump_Waiting = currentMillis_LED_Pump_Waiting;
		digitalWrite(LEDPump_PIN, ledState_pump_waiting);
	}
}

/*      No Water LED      */

void LEDNo_Water_on()
{
	digitalWrite(LEDNoWater_PIN, HIGH);
}

void LEDNo_Water_off()
{
	digitalWrite(LEDNoWater_PIN, LOW);
}

/*      Field Capacity LED     */

void LEDFieldC_Reached()
{
	digitalWrite(LEDFieldC_PIN, HIGH);
}

void LEDFieldC_off()
{
	digitalWrite(LEDFieldC_PIN, LOW);
}

/*      LEDs Alerts     */

void LEDs_Alerts()
{
	if ((current_state.FC >= sensor_value.cached_SMOp) && (current_state.SM <= current_state.SMmin && digitalRead(NoWater_PIN) == LOW)
  || (current_state.SM <= current_state.SMmin && digitalRead(NoWater_PIN) == HIGH)
  || ((current_state.SM <= current_state.SMmin && current_state.ST > current_state.STMax)) || ((current_state.SM <= current_state.SMmin && current_state.ST < current_state.STmin)))
  {
		LEDpump_waiting();
	}
	
	if (current_state.FC >= current_state.SMOp)
	{
		LEDFieldC_Reached();
	}
	else
	{
		LEDFieldC_off();
	}

  if (digitalRead(NoWater_PIN) == HIGH)
	{
		LEDNo_Water_on();
	}
	else
	{
		LEDNo_Water_off();
	}
}

// Buttons Configuration.

#define BUTTON_UP_PIN 9
#define BUTTON_CENTER_PIN 8
#define BUTTON_DOWN_PIN 7
#define BUTTONUP 0
#define BUTTONDOWN 1
#define BUTTONCENTER 2
#define BUTTONCENTERLONG 3
#define TIMEOUT 10
#define TOTALTIMEOUT 20000
#define IDLE -1

void button_pins()
{
	pinMode(BUTTON_UP_PIN, INPUT);
	pinMode(BUTTON_DOWN_PIN, INPUT);
	pinMode(BUTTON_CENTER_PIN, INPUT);
}

// General Setup.

void setup()
{
//Serial.begin(9600);
	SSerial.begin(9600);
	mylcd.begin();
	relay_pin();
	button_pins();
	led_pins();
	sm_power_pin();
	No_Water();
	refresh_LCD = true;
	if(!load_Settings(current_config))
	{
		current_config = reset_Settings();
	}
	global_variables();
}

// Initialize Global Variables.

void global_variables()
{
	active_language = current_config.active_languaje;
	sensor_value.cached_SMOp = (current_config.SMOp != 0) ? current_config.SMOp : 70;
	sensor_value.cached_SMmin = (current_config.SMmin != 0) ? current_config.SMmin : 50;
  sensor_value.cached_SMcalib = (current_config.SMcalib != 0) ? current_config.SMcalib : 500;
	sensor_value.cached_STMax = (current_config.STMax != 0) ? current_config.STMax : 32;
  sensor_value.cached_STmin = (current_config.STmin != 0) ? current_config.STmin : 4;
	sensor_value.cached_TICicle = (current_config.TICicle != 0) ? current_config.TICicle : 6;
	sensor_value.cached_PICicle = (current_config.PICicle != 0) ? current_config.PICicle : 50;
	time1 = millis();
}

// Core Logic (Irrigation System).

void core_logic_irrigation (void)

{
int No_Water = digitalRead(NoWater_PIN) == HIGH;
int No_ST_Sensor = current_state.ST == -1000;
int Dangerous_Soil_Temperature = current_state.ST >= current_state.STMax || current_state.ST <= current_state.STmin;
int Field_Capacity_Reached = current_state.FC >= current_state.SMOp;
int Dry_Soil = current_state.SM <= current_state.SMmin;
int Safety_Moisture_Range = current_state.SM > current_state.SMmin && current_state.SM < current_state.SMOp;
int Wet_Soil = current_state.SM >= current_state.SMOp;

	      if (Field_Capacity_Reached || No_Water || Dangerous_Soil_Temperature)
        {
        relay_off();
        }

        if (Dry_Soil)
	      {
       	Pump_pulses();
        LEDpumping();
        SM_flag = 0;
        }
      
        if (Safety_Moisture_Range && SM_flag == 0)
        {
       	Pump_pulses();
        LEDpumping();
        }
                      
        if (Wet_Soil)
        {
        relay_off();
        SM_flag = 1;
        } 
}
  
// Pump Pulses (Duty Cicle).

int Pump_State = LOW;
unsigned long previousMillis_Pump = 0;

void Pump_pulses()
{
	long Time_Pumping = sensor_value.cached_TICicle * sensor_value.cached_PICicle * 10;
	long Time_Waiting_Pumping = sensor_value.cached_TICicle * (1000 - sensor_value.cached_PICicle * 10);
	unsigned long currentMillis_Pump = millis();

	if ((Pump_State == HIGH) && (currentMillis_Pump - previousMillis_Pump > Time_Pumping))
	{
		Pump_State = LOW;
		previousMillis_Pump = currentMillis_Pump;
		digitalWrite(RELAY_PIN, Pump_State);
	}

	else if ((Pump_State == LOW) && (currentMillis_Pump - previousMillis_Pump > Time_Waiting_Pumping))
	{
		Pump_State = HIGH;
		previousMillis_Pump = currentMillis_Pump;
		digitalWrite(RELAY_PIN, Pump_State);
  }

  if ((Pump_State == LOW) && sensor_value.cached_PICicle == 100)
  {  
    Pump_State = HIGH;
    previousMillis_Pump = currentMillis_Pump;
    digitalWrite(RELAY_PIN, Pump_State);
  } 
}

// Sustituir todas las funcionalidades (lineas 416 a 529) de los botones, con la libreria button.h (Ver archivo .ino)

/*Gets the current Button Event
 * returns: the button that is pressed.
 *  0: button UP is Pressed.
 *  1: button DOWN is Pressed.
 *  2: button CENTER is Pressed minor than 3 seconds.
 *  3: button CENTER is Pressed more than 3 seconds.
 */

int get_event()
{
	int event = IDLE;
	event = button_up_pressed();

	if(event != IDLE)
	{
		return event;
	}
	event = button_down_pressed();

	if(event != IDLE)
	{
		return event;
	}
	time2 = millis();
	event = button_center_pressed();
	
        if(event >= 0)
	{
		time1 = millis();
	}
	
        if((time2 - time1) >= TOTALTIMEOUT)
	{
		time1 = millis();
		event = TIMEOUT;
	}
	return event;
}

/*
* Check if button up is pressed. The Event only ocurrs when the button is released; not
 * when button is pressed.
 returns HIGH when button Up is released or LOW otherwise.
 */

int button_up_pressed()
{
	boolean current_state = digitalRead(BUTTON_UP_PIN);

	if(current_state == HIGH)
	{
		time1I = millis();
		return BUTTONUP;
	}
	if(current_state == LOW)
	{
		time1I = millis();
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
	boolean current_state = digitalRead(BUTTON_DOWN_PIN);

	if(current_state == HIGH)
	{
		time3I = millis();
		return BUTTONDOWN;
	}

	if(current_state == LOW)
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
	boolean current_state = digitalRead(BUTTON_CENTER_PIN);

	if(current_state == HIGH && cerrojo_center == 1)
	{
		cerrojo_center = 0;
		time2I = millis();
		return BUTTONCENTER;
	}
	
        if(current_state == LOW && cerrojo_center == 0)
	{
		cerrojo_center = 1;
		time2I = millis();
	}

	if(current_state == HIGH)
	{
		long TIME_center = millis();
		if(TIME_center - time2I >= 3000)
		{
			time2I = millis();
			return BUTTONCENTERLONG;
		}
	}
	return IDLE;
}

/*      USER LCD INTERFACE      */

// Configuration Structure Menu.

typedef struct MenuItem
{
  int label;
  int state;
};

// Screen MODES.

enum SCREEN_MODES
{
    HOME,
    SELECT,
    CONFIG_MENU,
};

// States in SELECT Screen.

enum SELECT_SCREEN
{
    S_SMOp,
    S_SMmin,
    S_TICicle,
    S_PICicle,
    S_STMax,
    S_STmin,
};

int home_screen = S_SMOp;

// Comportamiento del boton central para acceder al menu de configuracion si la Pulsacion es >3", o en caso de pulsacion corta:

// a) Entra en Modo Seleccion desde el menu HOME.
// b) Entra en Modo Edicion si se esta en Modo Seleccion.

void handleEvent(int event)
{
  if(event == BUTTONCENTERLONG)
  {
    mylcd.clear();
    current_menu_state = CONFIG_MENU;
    current_menu = 0;
    refresh_LCD = true;
    mylcd.underlineCursorOff();
    mylcd.boxCursorOff();
    return;
  }
  switch(current_menu_state)
  {
    case HOME:
      if(event == BUTTONCENTER)
      {
        mylcd.clear();
        current_menu_state = SELECT;
        refresh_LCD = true;
        home_screen = S_SMOp;
      }
      break;

    case SELECT:
      EventSELECT(event);
      break;

    case CONFIG_MENU:
      EventCONFIG_MENU(event);
      break;
  }
}

// Comportamiento del boton central en Modo Seleccion que pasa a Modo Edicion (Pantalla de Inicio).

void EventSELECT(int event)
{
  if(event == BUTTONCENTER)
  {
    isEditing = !isEditing;
    refresh_LCD = true;
  }

// Comportamiento de los botones UP y DOWN en Modo Seleccion (Pantalla de Inicio).

  if(!isEditing)
  {
    if(event == BUTTONUP)
    {
      home_screen--;
      if(home_screen < 0)
      {
        home_screen = 0;
      }
      refresh_LCD = true;
    }

    if(event == BUTTONDOWN)
    {
      home_screen++;
      if(home_screen > 5)
      {
        home_screen = 5;
      }
      refresh_LCD = true;
    }

    if(event == TIMEOUT)
    {
      home_screen = S_SMOp;
      current_menu_state = HOME;
      refresh_LCD = true;
      mylcd.clear();
      mylcd.boxCursorOff();

      // Store current state settings

      current_config.SMOp = sensor_value.cached_SMOp;
      current_config.SMmin = sensor_value.cached_SMmin;
      current_config.SMcalib = sensor_value.cached_SMcalib;
      current_config.STMax = sensor_value.cached_STMax;
      current_config.STmin = sensor_value.cached_STmin;
      current_config.PICicle = sensor_value.cached_PICicle;
      current_config.TICicle = sensor_value.cached_TICicle;
      store_Settings(current_config);
    }
  }

// Comportamiento de los botones UP y DOWN en Modo Edicion segun los diferentes casos (Pantalla de Inicio).

  else
  {
    switch(home_screen)
    {
      case S_SMOp:

        if(event == BUTTONUP)
        {
          sensor_value.cached_SMOp++;
          if(sensor_value.cached_SMOp > 99)
          {
            sensor_value.cached_SMOp = 99;
          }
          refresh_LCD = true;
        }

        if(event == BUTTONDOWN)
        {
          sensor_value.cached_SMOp--;
          if(sensor_value.cached_SMOp <= sensor_value.cached_SMmin)
          {
            sensor_value.cached_SMOp = sensor_value.cached_SMmin + 1;
          }
          refresh_LCD = true;
        }

        if(event == TIMEOUT)
        {
          home_screen = S_SMOp;
          current_menu_state = HOME;
          refresh_LCD = true;
          mylcd.clear();
          mylcd.boxCursorOff();
        }
        break;

      case S_SMmin:

        if(event == BUTTONUP)
        {
          sensor_value.cached_SMmin++;
          if(sensor_value.cached_SMmin >= sensor_value.cached_SMOp)
          {
            sensor_value.cached_SMmin = sensor_value.cached_SMOp - 1;
          }
          refresh_LCD = true;
        }

        if(event == BUTTONDOWN)
        {
          sensor_value.cached_SMmin--;
          if(sensor_value.cached_SMmin < 0)
          {
            sensor_value.cached_SMmin = 0;
          }
          refresh_LCD = true;
        }

        if(event == TIMEOUT)
        {
          home_screen = S_SMOp;
          current_menu_state = HOME;
          refresh_LCD = true;
          mylcd.clear();
          mylcd.boxCursorOff();
        }
        break;

      case S_TICicle:

        if(event == BUTTONUP)
        {
          sensor_value.cached_TICicle++;
          if(sensor_value.cached_TICicle > 300)
          {
            sensor_value.cached_TICicle = 300;
          }
          refresh_LCD = true;
        }

        if(event == BUTTONDOWN)
        {
          sensor_value.cached_TICicle--;
          if(sensor_value.cached_TICicle < 1)
          {
            sensor_value.cached_TICicle = 1;
          }
          refresh_LCD = true;
        }

        if(event == TIMEOUT)
        {
          home_screen = S_SMOp;
          current_menu_state = HOME;
          refresh_LCD = true;
          mylcd.clear();
          mylcd.boxCursorOff();
        }
        break;

      case S_PICicle:

        if(event == BUTTONUP)
        {
          sensor_value.cached_PICicle++;
          if(sensor_value.cached_PICicle > 100)
          {
            sensor_value.cached_PICicle = 100;
          }
          refresh_LCD = true;
        }

        if(event == BUTTONDOWN)
        {
          sensor_value.cached_PICicle--;
          if(sensor_value.cached_PICicle < 1)
          {
            sensor_value.cached_PICicle = 1;
          }
          refresh_LCD = true;
        }

        if(event == TIMEOUT)
        {
          home_screen = S_SMOp;
          current_menu_state = HOME;
          refresh_LCD = true;
          mylcd.clear();
          mylcd.boxCursorOff();
        }
        break;

      case S_STMax:

        if(event == BUTTONUP)
        {
          sensor_value.cached_STMax++;
          if(sensor_value.cached_STMax > 60)
          {
            sensor_value.cached_STMax = 60;
          }
          refresh_LCD = true;
        }

        if(event == BUTTONDOWN)
        {
          sensor_value.cached_STMax--;
          if(sensor_value.cached_STMax <= sensor_value.cached_STmin)
          {
            sensor_value.cached_STMax = sensor_value.cached_STmin + 1;
          }
          refresh_LCD = true;
        }

        if(event == TIMEOUT)
        {
          home_screen = S_SMOp;
          current_menu_state = HOME;
          refresh_LCD = true;
          mylcd.clear();
          mylcd.boxCursorOff();
        }
        break;

      case S_STmin:

        if(event == BUTTONUP)
        {
          sensor_value.cached_STmin++;
          if(sensor_value.cached_STmin >= sensor_value.cached_STMax)
          {
            sensor_value.cached_STmin = sensor_value.cached_STMax - 1;
          }
          refresh_LCD = true;
        }

        if(event == BUTTONDOWN)
        {
          sensor_value.cached_STmin--;
          if(sensor_value.cached_STMax <= sensor_value.cached_STmin)
          {
            sensor_value.cached_STMax = sensor_value.cached_STmin + 1;
          }
          if(sensor_value.cached_STmin < 0)
          {
            sensor_value.cached_STmin = 0;
          }
          refresh_LCD = true;
        }

        if(event == TIMEOUT)
        {
          home_screen = S_SMOp;
          current_menu_state = HOME;
          refresh_LCD = true;
          mylcd.clear();
          mylcd.boxCursorOff();
        }
        break;
    }
  }
}

/*      SCREEN VIEWS      */

// Draws Interface in Status MODE and SELECTION MODE

void DrawUI(State & state)
{
  if(refresh_LCD)
  {
    switch(current_menu_state)
    {
      case HOME:
        DrawHOME(state);
        break;

      case SELECT:
        DrawSELECT(state);
        break;

      case CONFIG_MENU:
        DrawCONFIG_MENU();
        break;
    }
    refresh_LCD = false;
  }
}

// Draws HOME SCREEN.

void DrawHOME(State & state)
{

// Line 1.

  mylcd.setPosition(1, 0);
  mylcd.print(translate(S_SM));
  if(state.SMOp < 10)
    mylcd.print("0");
  mylcd.print((int)state.SMOp);
  mylcd.print(" ");
  mylcd.print(translate(S_SMm));
  if(state.SMmin < 10)
    mylcd.print("0");
  mylcd.print((int)state.SMmin);
  mylcd.setPosition(1, 16);
  if(state.SM < 10)
    mylcd.print("00");
  if(state.SM <= 99 && state.SM >= 10)
    mylcd.print("0");
  mylcd.print((int)state.SM);
  mylcd.print("%");

// Line 2.

  mylcd.setPosition(2, 0);
  mylcd.print(translate(S_CICLE));
  int seconds = sensor_value.cached_TICicle;
  if(seconds < 10)
    mylcd.print("00");
  if(seconds < 100 && seconds >= 10)
    mylcd.print("0");
  mylcd.print(seconds);
  mylcd.print("\42"); //Simbolo dobles comillas
  mylcd.setPosition(2, 13);
  mylcd.print(translate(S_ON));
  if(sensor_value.cached_PICicle < 10)
    mylcd.print("00");
  if(sensor_value.cached_PICicle < 100 && sensor_value.cached_PICicle >= 10)
    mylcd.print("0");
  mylcd.print(sensor_value.cached_PICicle);
  mylcd.print("%");

// Line 3.

  int ST = state.ST;
  if(ST != -1000)
{
    mylcd.setPosition(3, 0);
  mylcd.print(translate(S_STMAX));
  if(state.STMax < 10)
    mylcd.print("0");
  mylcd.print((int)state.STMax);
  mylcd.print(" ");
  mylcd.print(translate(S_STMIN));
  if(state.STmin < 10)
    mylcd.print("0");
  mylcd.print((int)state.STmin);
  mylcd.print(" ");
  if(ST >= 0)
  {
    mylcd.print("+");
  }
  else
  {
    if(ST == -1000)
    {
      mylcd.print("-");
    }
  }
    mylcd.print(ST);
    mylcd.print("\337"); // Simbolo grados
}
  else
  {
    mylcd.print("                    ");
  }
  
// Line 4.

  mylcd.setPosition(4, 0);
  mylcd.print(translate(S_CONSUMPTION));
  mylcd.setPosition(4, 13);
  mylcd.print(",");
  mylcd.setPosition(4, 18);
  mylcd.print("M3");
}

// Draws SELECT/EDIT Mode Screen.

void static DrawSELECT(State & state)
{

// Line 1.

  mylcd.setPosition(1, 0);
  mylcd.print(translate(S_SM));
  if(state.SMOp < 10)
    mylcd.print("0");
  mylcd.print((int)state.SMOp);
  mylcd.print(" ");
  mylcd.print("MIN:");
  if(state.SMmin < 10)
    mylcd.print("0");
  mylcd.print((int)state.SMmin);

// Line 2.

  mylcd.setPosition(2, 0);
  mylcd.print(translate(S_CICLE));
  int seconds = sensor_value.cached_TICicle;
  if(seconds < 10)
    mylcd.print("00");
  if(seconds < 100 && seconds >= 10)
    mylcd.print("0");
  mylcd.print(seconds);
  mylcd.print("\42"); // Simbolo dobles comillas
  mylcd.setPosition(2, 13);
  mylcd.print(translate(S_ON));
  if(sensor_value.cached_PICicle < 10)
    mylcd.print("00");
  if(sensor_value.cached_PICicle < 100 && sensor_value.cached_PICicle >= 10)
    mylcd.print("0");
  mylcd.print(sensor_value.cached_PICicle);
  mylcd.print("%");

// Line 3.

if(state.ST != -1000)
{
  mylcd.setPosition(3, 0);
  mylcd.print(translate(S_STMAX));
  if(state.STMax < 10)
    mylcd.print("0");
  mylcd.print((int)state.STMax);
  mylcd.print(" ");
  mylcd.print(translate(S_STMIN));
  if(state.STmin < 10)
    mylcd.print("0");
  mylcd.print((int)state.STmin);
  mylcd.setPosition(3, 16);
  mylcd.print("\337C"); // Simbolo grados
  mylcd.boxCursorOff();
  mylcd.underlineCursorOff();
}
  else
  {
    mylcd.print("                    ");
  }

/*     Cursor SELECT/EDIT Mode Screen     */

  switch(home_screen)
  {
    case S_SMOp:

      if(!isEditing)
      {
        mylcd.setPosition(1, 2);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(1, 5);
        mylcd.underlineCursorOn();
      }
      break;

    case S_SMmin:

      if(!isEditing)
      {
        mylcd.setPosition(1, 9);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(1, 12);
        mylcd.underlineCursorOn();
      }
      break;

    case S_TICicle:

      if(!isEditing)
      {
        mylcd.setPosition(2, 5);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(2, 9);
        mylcd.underlineCursorOn();
      }
      break;

    case S_PICicle:

      if(!isEditing)
      {
        mylcd.setPosition(2, 14);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(2, 18);
        mylcd.underlineCursorOn();
      }
      break;

    case S_STMax:

      if(!isEditing)
      {
        mylcd.setPosition(3, 4);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(3, 7);
        mylcd.underlineCursorOn();
      }
      break;

    case S_STmin:

      if(!isEditing)
      {
        mylcd.setPosition(3, 11);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(3, 14);
        mylcd.underlineCursorOn();
      }
      break;
  }
}

// States in Configuration MENU MODE.

enum CONFIG_MENU
{
    HOME_Config_Menu,
    CALIBRACION_SAT,
    RESET_CONFIG,
    ABOUT,
    END_SELECTION
};

MenuItem config_menu[] =
{
  {S_SATCALIBRATION, CALIBRACION_SAT},
  {S_RESET, RESET_CONFIG},
  {S_ABOUT, ABOUT},
  {S_RETURN_TO, END_SELECTION}
};

// Draws Interface for the Configuration Menu.

void DrawCONFIG_MENU()
{
  switch(current_screen)
  {
    case HOME_Config_Menu:
      DrawMenu();
      break;

    case CALIBRACION_SAT:
      DrawCalibrationSat();
      break;

    case ABOUT:
      DrawAbout();
      break;
  }
}

// Comportamiento de los botones en Modo Configuracion segun los diferentes casos.

void EventCONFIG_MENU(int event)
{
  switch(current_screen)
  {
    case HOME_Config_Menu:

      if(event == BUTTONCENTER)
      {
        mylcd.clear();
        current_screen = config_menu[current_menu].state;
        select_language = 0;
        refresh_LCD = true;
        isEditing = false;
      }

      if(event == BUTTONDOWN)
      {
        mylcd.clear();
        current_menu++;
        current_menu = current_menu % MAXMENUITEMS;
        refresh_LCD = true;
      }

      if(event == BUTTONUP)
      {
        mylcd.clear();
        current_menu = (current_menu == 0) ? MAXMENUITEMS - 1 : current_menu - 1;
        current_menu = current_menu % MAXMENUITEMS;
        refresh_LCD = true;
      }
      break;

    case CALIBRACION_SAT:

      if(event == BUTTONCENTER)
      {
        int calib = readSMcalib();
        current_config.SMcalib = calib;
        store_Settings(current_config);
        current_screen = HOME_Config_Menu;
        mylcd.clear();
      }
      refresh_LCD = true;
      break;

    case RESET_CONFIG:

      current_config = reset_Settings();
      global_variables();
      current_menu_state = CONFIG_MENU;
      current_screen = HOME_Config_Menu;
      refresh_LCD = true;
      break;

    case ABOUT:

      if(event == BUTTONCENTER)
      {
        current_screen = HOME_Config_Menu;
        refresh_LCD = true;
        mylcd.clear();
      }
      break;

    case END_SELECTION:

      current_menu_state = HOME;
      current_screen = HOME_Config_Menu;
      mylcd.clear();
      refresh_LCD = true;
      break;
  }
}

// Draws Configuration Submenus.

void DrawMenu()
{
  mylcd.setPosition(1, 0);
  printTitle(mylcd, translate(S_CONFIGURATION));
  int position = 2;
  for(int i = current_menu; i < (current_menu + 3) && i < MAXMENUITEMS; i++)
  {
    mylcd.setPosition(position, 0);
    if(i == current_menu)
      mylcd.print("\176"); //Simbolo seleccion o \245
    mylcd.print(translate(config_menu[i].label));
    position++;
  }
}

void DrawCalibrationSat()
{
  mylcd.setPosition(1, 0);
  printTitle(mylcd, translate(S_SATCALIBRATION));
  mylcd.setPosition(2, 0);
  mylcd.print(translate(S_CURRENTVALUE));
  mylcd.print(F(": "));
  mylcd.print(readSMcalib());
//mylcd.setPosition(3, 0);
//mylcd.print(current_state.SMcalib);
  refresh_LCD = true;
}

void static DrawAbout()
{
  mylcd.setPosition(1, 0);
  mylcd.print(translate(S_SAC));
  mylcd.setPosition(2, 0);
  mylcd.print(F("http://sacultivo.com"));
  mylcd.setPosition(3, 0);
  mylcd.print(F("FIRMWARE: "));
  mylcd.print(FIRMWARE);
  mylcd.setPosition(4, 0);
  mylcd.print(F("HARDWARE: "));
  mylcd.print(HARDWARE);
}

// Void Loop (Update Screen when any state changes).

void loop()
{
  update_State(sensor_value, current_config.SMcalib);
  current_state = read_sensors(sensor_value);

  if(current_menu_state == HOME)
  {
    if(state_changed(current_state, previous_state))
    {
      refresh_LCD = true;
      previous_state = current_state;
    }
  }

  DrawUI(current_state);
  int event = get_event();
  handleEvent(event);
  LEDs_Alerts();
  core_logic_irrigation();
//Serial.println(current_state.SM);
}
