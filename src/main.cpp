#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Preferences.h>
#include <ezButton.h>
#include <time.h>
#include "mywifi.h"
#include "cStatusLed.h"
#include "defines.h"
#include "ota.h"
#include "stepper.h"
#include "ntp.h"

#define GPIO_LED        LED_BUILTIN   //defines the IO pin for tue status LED

#define UPDATESECONDS   4  //this sets the second that will trigger the motor to spin, to match with the new minute
                           //my clock takes 3 seconds to spin the minute wheel


//!< this disables the ESP32 brownout detection. Don't use in production
//!< remove it from the platformio.ini in the final build
#ifdef DISABLE_BROWNOUT   
  #warning BROWNOUT DISABLED
 #include "soc/soc.h"
 #include "soc/rtc_cntl_reg.h"
#endif 


long gPulsesPerSpin = 0;

cStatusLed  gLed(GPIO_LED, true);
Preferences preferences;

ezButton button_minus(KEY_MINUS);
ezButton button_plus(KEY_PLUS);
ezButton button_advance_one(KEY_ADVANCE_ONE);
//ezButton button_sensor(KEY_BUTTON);


void buttons_setup(){
  button_minus.setDebounceTime(50);
  button_plus.setDebounceTime(50);
  button_advance_one.setDebounceTime(50);
  //button_sensor.setDebounceTime(50);
}


void setup(){
#ifdef DISABLE_BROWNOUT
   WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
#endif
  Serial.begin(9600);
  //delay(5000);
  preferences.begin("clock", false); 
  gPulsesPerSpin = 0; //preferences.getLong("pulses");
  gLed.SetState(LED_STATUS_OFFLINE);
  stepper_setup();
  buttons_setup();
  Serial.printf("Pulses:    %ld\n", gPulsesPerSpin);
  Serial.printf("Lash Hour: %d\n", preferences.getInt("hour"));
  Serial.printf("Last min:  %d\n", preferences.getInt("min"));
}

void save_current_time()
{
  struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        Serial.println("Getlocaltime failed");
        return;
    }
    preferences.putInt("hour", timeinfo.tm_hour);
    preferences.putInt("min", timeinfo.tm_min);
}

uint32_t spin_to_current_time()
{
    int hour = preferences.getInt("hour");
    int minutes = preferences.getInt("min");
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        Serial.println("Getlocaltime failed");
        return 0;
    }
    //calculate time difference
    int last_minutes = ((hour % 12) * 60) + minutes;
    int now_minutes  = ((timeinfo.tm_hour % 12) * 60) + timeinfo.tm_min;
    int delta_minutes = now_minutes - last_minutes;
    if(delta_minutes < 0)
      delta_minutes += 12 * 60;
    
    return delta_minutes;
}

// bool sensor_task()
// {
//   static uint8_t phase = 0;
//   if(phase == 0 && button_sensor.isPressed()){
//     Serial.printf("Sensor pressed\n");
//     phase = 1;
//   }
//   else if(phase == 1 && button_sensor.isReleased()){
//     phase = 0;
//     Serial.printf("Sensor released\n");
//     return true;
//   }
//   return false;
// }


int test_buttons()
{
  if(button_minus.isPressed())
    return -100;
  else if(button_plus.isPressed())
    return 100;  
  else if(button_advance_one.isPressed())
    return PULSES_PER_MINUTE;  
    
  return 0;
}

void button_task()
{
  button_minus.loop();
  button_plus.loop();
  button_advance_one.loop();
  //button_sensor.loop();
}


void loop() {
  button_task();
  bool online = wifi_loop();
  int adjust_level = ota_loop();
  stepper_loop(false);
  gLed.Task();
  
  if(online && gLed.GetState() == LED_STATUS_OFFLINE)
  {
#ifdef UPDATE_ON_START
    stepper_move(spin_to_current_time() * PULSES_PER_MINUTE);
#endif
    gLed.SetState(LED_STATUS_ONLINE);
  }

  if(online)
    update_time();

  
  if(minute_changed(UPDATESECONDS)){
    save_current_time();
    stepper_move(PULSES_PER_MINUTE);
  }
  else{
    int value = test_buttons();
    if(value)
      stepper_move(value);
    else if(adjust_level != 0)
      stepper_move(adjust_level);
  }
}


/***
 *  18:00 - 8:26  --> 8:28   (1548)
 *  626 min girou 628  --> 1543 
 * 
 */

//eof main