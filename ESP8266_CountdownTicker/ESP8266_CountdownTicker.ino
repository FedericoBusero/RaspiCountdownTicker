// Voor doit ESP8266 board: compileren met Board NodeMU 1.0
// Om nieuwe firmware te flashen: flash ingedrukt houden en gelijktijdig op reset drukken, dan zit hij in flash mode

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>

//LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x3F for a 16 chars and 2 line display

// 3.3V
LiquidCrystal_I2C lcd(0x38, 16, 2); // set the LCD address to 0x3F for a 16 chars and 2 line display


// 5V
// LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x3F for a 16 chars and 2 line display
// Opgelet, hoewel module wel werkt op 3.3V, heeft het toch 5V nodig om voldoende hard te licht te geven

// SDA=D2=GPIO4, SCL=D1=GPIO5 op NodeMCU

#include "Button.h"
/*
  TODO:
  - check if string class causes memory leaks
  - hidden switchoff (very long press stop = shutdown)?
*/
enum
{
  BUTTON_RESET = 1,
  BUTTON_PAUSEPLAY,
  BUTTON_STOP,
  BUTTON_STOP_LONG,
  BUTTON_MINUTES_PLUS,
  BUTTON_MINUTES_MIN,
  BUTTON_SECONDS_PLUS,
  BUTTON_SECONDS_MIN,
};

enum
{
  MODE_BOOTING = 0,
  MODE_RUNNING,
  MODE_PAUSED,
  MODE_STOPPED,
  MODE_QUIT,
};

ButtonRepeat button_D3(D3, BUTTON_MINUTES_PLUS); // GPIO0
ButtonRepeat button_D4(D4, BUTTON_MINUTES_MIN);  // GPIO2
ButtonRepeat button_D6(D6, BUTTON_SECONDS_PLUS); // GPIO12
ButtonRepeat button_D7(D7, BUTTON_SECONDS_MIN);  // GPIO13
Button button_D5(D5, BUTTON_RESET);              // GPIO14
Button button_D0(D0, BUTTON_PAUSEPLAY);          // GPIO16
//ButtonLong button_GPIO10(10, BUTTON_STOP, BUTTON_STOP_LONG);
ButtonLong button_GPIO10(10, BUTTON_STOP, BUTTON_STOP);

String inputString = "";         // a String to hold incoming data

int nexttimer_hour = 0;
int nexttimer_minutes = 0;
int nexttimer_seconds = 0;
int currentmode = MODE_BOOTING;

void nexttimer_init()
{
  nexttimer_hour = 0;
  nexttimer_minutes = 0;
  nexttimer_seconds = 0;
}

void nexttimer_minutes_plus()
{
  nexttimer_minutes += nexttimer_hour * 60;
  nexttimer_hour = 0;

  if (nexttimer_minutes < 9 * 60 + 59) // max 9u59 min (1 digit hour)
  {
    ++nexttimer_minutes;
  }

  nexttimer_hour = nexttimer_minutes / 60;
  nexttimer_minutes = nexttimer_minutes % 60;
}

void nexttimer_minutes_min()
{
  nexttimer_minutes += nexttimer_hour * 60;
  nexttimer_hour = 0;

  if (nexttimer_minutes > 0)
  {
    --nexttimer_minutes;
  }

  nexttimer_hour = nexttimer_minutes / 60;
  nexttimer_minutes = nexttimer_minutes % 60;
}

void nexttimer_seconds_plus()
{
  nexttimer_seconds = (nexttimer_seconds + 1) % 60;
}

void nexttimer_seconds_min()
{
  nexttimer_seconds = (nexttimer_seconds + 60 - 1) % 60;
}

void ticker_StartCountdown(int _hour, int _minute, int _second) {
  Serial.print("StartCountdown ");
  Serial.print(_hour);
  Serial.print(' ');
  Serial.print(_minute);
  Serial.print(' ');
  Serial.print(_second);
  Serial.print("\r\n");
}

void ticker_ResetCountdown(int _hour, int _minute, int _second) {
  Serial.print("ResetCountdown ");
  Serial.print(_hour);
  Serial.print(' ');
  Serial.print(_minute);
  Serial.print(' ');
  Serial.print(_second);
  Serial.print("\r\n");
}


void ticker_pause() {
  Serial.print("Pause");
  Serial.print("\r\n");
}

void ticker_stop() {
  Serial.print("Stop");
  Serial.print("\r\n");
}

void ticker_quit() {
  Serial.print("Quit");
  Serial.print("\r\n");
}

void updatedisplay()
{
  String str = "Next: ";
  lcd.setCursor(0, 0);
  if (nexttimer_hour)
  {
    str += (char)('0' + (nexttimer_hour % 10));
    str += ':';
  }
  else
  {
    str += "  ";
  }
  str += (char)('0' + (nexttimer_minutes / 10));
  str += (char)('0' + (nexttimer_minutes % 10));
  str += ':';
  str += (char)('0' + (nexttimer_seconds / 10));
  str += (char)('0' + (nexttimer_seconds % 10));
  str += "  ";
  lcd.print(str);
}


void onButtonPressed(int id)
{
  switch (id)
  {
    case BUTTON_RESET:
      if (currentmode != MODE_BOOTING)
      {
        ticker_ResetCountdown(nexttimer_hour, nexttimer_minutes, nexttimer_seconds);
      }
      break;

    case BUTTON_PAUSEPLAY:
      switch (currentmode)
      {
        case MODE_BOOTING:
          break;

        case MODE_PAUSED:
        case MODE_RUNNING:
          ticker_pause();
          break;

        case MODE_STOPPED:
          if ((nexttimer_hour > 0) || (nexttimer_minutes > 0) || (nexttimer_seconds > 0))
          {
            ticker_StartCountdown(nexttimer_hour, nexttimer_minutes, nexttimer_seconds);
          }
          break;

      }
      break;

    case BUTTON_STOP:
      if (currentmode != MODE_BOOTING)
      {
        ticker_stop();
      }
      break;

    case BUTTON_STOP_LONG:
      if (currentmode != MODE_BOOTING)
      {
        ticker_quit();
      }
      break;

    case BUTTON_MINUTES_PLUS:
      nexttimer_minutes_plus();
      updatedisplay();
      break;

    case BUTTON_MINUTES_MIN:
      nexttimer_minutes_min();
      updatedisplay();
      break;

    case BUTTON_SECONDS_PLUS:
      nexttimer_seconds_plus();
      updatedisplay();
      break;

    case BUTTON_SECONDS_MIN:
      nexttimer_seconds_min();
      updatedisplay();
      break;
  }
}

void onButtonReleased(int id)
{

}

void setup() {
  Serial.begin(115200);
  inputString.reserve(200);

  WiFi.mode(WIFI_OFF);

  nexttimer_init();

  lcd.init();                      // initialize the lcd
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Next:   00:00   ");
  lcd.setCursor(0, 1);
  lcd.print("Booting ...     ");
}

void loop() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      //      lcd.setCursor(0, 1);
      //      lcd.print("                ");
      //      Serial.println(inputString);
      inputString.trim();
      int pos = inputString.indexOf(',');
      if (pos >= 0)
      {
        if (inputString.length() >= pos + 2)
        {
          char mode_char = inputString.charAt(pos + 1);
          if ((mode_char >= '0') && (mode_char <= '5'))
          {
            currentmode = mode_char - '0';
            if (pos == 0)
            {
              inputString = "";
            }
            else
            {
              inputString = inputString.substring(0, pos);
            }

            switch (currentmode)
            {
              case MODE_BOOTING:
                break;

              case MODE_RUNNING:
                break;

              case MODE_PAUSED:
                break;

              case MODE_STOPPED:
                updatedisplay();
                break;

              case MODE_QUIT:
                lcd.setCursor(0, 0);
                lcd.print("Shutdown        ");
                lcd.setCursor(0, 1);
                lcd.print("                ");
                break;
            }
          }
        }
      }
      lcd.setCursor(0, 1);
      lcd.print(inputString + "                ");
      // clear the string:
      inputString = "";
    }
  }

  button_D0.checkStatus();
  button_D3.checkStatus();
  button_D4.checkStatus();
  button_D5.checkStatus();
  button_D6.checkStatus();
  button_D7.checkStatus();
  button_GPIO10.checkStatus();
}

