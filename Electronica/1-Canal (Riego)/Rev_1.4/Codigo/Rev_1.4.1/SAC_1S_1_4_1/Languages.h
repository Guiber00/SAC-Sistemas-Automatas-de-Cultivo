/*******************************************************************************************************   

  S.A.C. Project (Automatic Cropping Systems) http://sacultivo.com started originally by Adrian Navarro.
  
  This file contains all the Language Options.

  Created on: 16/03/2014
  Author: David Cuevas <mr.cavern@gmail.com>
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
        
*******************************************************************************************************/

int active_language = 1;

enum
{
    S_SM,
    S_RETURN_TO,
    S_CONFIGURATION,
    S_RESET,
    S_ON,
    S_STMAX,
    S_STMIN,
    S_CONSUMPTION,
    S_CICLE,
    S_SATCALIBRATION,
    S_CURRENTVALUE,
    S_ABOUT,
    S_SAC,
    S_SMm,
};

#define MAX_LANGUAGE 1
typedef struct TranslatedString
{
	PROGMEM const char *languages[MAX_LANGUAGE];
} TranslatedString;

TranslatedString string_db[] =
{

// For languages missing translations, English will be used instead.

	{{"HSO:"}},
	{{"INICIO"}},
	{{"CONFIGURACION"}},
	{{"RESET"}},
	{{"ON:"}},
	{{"TSMAX:"}},
	{{"MIN:"}},
	{{"CONSUMO:"}},
	{{"PULSOS:"}},
	{{"CALIBRACION SAT"}},
	{{"VALOR ACTUAL"}},
	{{"INFO"}},
	{{"--- By AISur.org ---"}},
	{{"MIN:"}},
};

// Returns a translated string (If no translation found then return the original string).

static const char *translate(int stringno)
{
	if (active_language < 0)
		active_language = 0;
	if (active_language >= MAX_LANGUAGE)
		active_language = MAX_LANGUAGE - 1;

	if (string_db[stringno].languages[active_language])
		return string_db[stringno].languages[active_language];
	else
		return string_db[stringno].languages[0];
}
