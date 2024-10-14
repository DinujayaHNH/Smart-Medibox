#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels 
#define OLED_RESET -1// Reset pin (or -1 if sharing Arduino reset p
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x30 for 128x6m

#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     0
#define UTC_OFFSET_DST 0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;


int days =0;
int hours=0;
int minutes=0;
int seconds=0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0,1,2};
int alarm_minutes[] = {1,10,20};
bool alarm_triggered[] = {false, false, false};

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set Time Zone", "2 - Set Alarm 1", "3 - Set Alarm 2", " 4 - Set Alarm 3", " 5 - disable alarm"};

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);


  Serial.begin(9600);
  // put your setup code here, to run once:
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
     Serial.println(F("SS01306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 0.5 seconds
  // Clear the buffer
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0, 0, 2);
  }
  delay(300);
  display.clearDisplay();
  print_line("Connected to wifi",0,0,2);
  delay(500);

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);


  display.clearDisplay(); 
  print_line("Welcome to Medibox!",10,20,2);
  display.clearDisplay();
  
}

void loop() {
  //put your main code here, to run repeatedly:
  //update_time();
  //print_time_now();
  update_time_with_check_alarm();
  if (digitalRead(PB_OK) == LOW) {
    delay(200);
    go_to_menu();
  }
  check_temp();
}

void print_line(String text,int column, int row,int text_size){
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column,row); // Normal 1:1 pixel scale // Draw white text // Start at top-left corner 
  display.println(text);
  display.display();
}

void print_time_now(void) {
  display.clearDisplay(); 
  print_line(String(days), 0, 0, 2);
  print_line(":", 20, 0, 2);
  print_line(String(hours), 30, 0, 2);
  print_line(":", 50, 0, 2);
  print_line(String(minutes), 60, 0, 2);
  print_line(":", 80, 0, 2);
  print_line(String(seconds), 90, 0, 2);
}

void update_time(void){
  /*timeNow = millis() / 1000; // seconds passed from bootup
  seconds = timeNow - timeLast; // number of seconds passed 
  // if a minute has passed
  if (seconds >= 60) {
    minutes += 1;
    timeLast += 60;
  }
  // if a hour has passed
  if (minutes == 60) {
    hours += 1;
    minutes = 0;
  }
  // if a day has passed
  if (hours == 24) {
    days += 1;
    hours = 0;
  }*/
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timehour[3];
  strftime(timehour,3, "%H", &timeinfo);
  hours = atoi(timehour);

  char timeminute[3];
  strftime(timeminute,3, "%M", &timeinfo);
  minutes = atoi(timeminute);

  char timesecond[3];
  strftime(timesecond,3, "%S", &timeinfo);
  seconds = atoi(timesecond);

  char timeDay[3];
  strftime(timeDay,3, "%d", &timeinfo);
  days = atoi(timeDay);

}

void ring_alarm() {
  // Show message on display
  display.clearDisplay();
  print_line("MEDICINE TIME!", 2, 0, 0);
  digitalWrite(LED_1, HIGH);
  
  bool break_happened = false;
  // Ring the buzzer
  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH) {
    for (int i=0; i<n_notes; i++){
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break_happened = true;
        break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }
  
  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}

void update_time_with_check_alarm(void) {
  update_time();
  print_time_now();

  if (alarm_enabled == true) {
    for (int i = 0; i < n_alarms ; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes){
        ring_alarm(); // call the ringing function
        alarm_triggered[i] = true;
      }
    }
  }
}


int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    }
    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    }
    else if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    }
    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }
    update_time();
  }
}

void go_to_menu(){
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2); 
      
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;

    }
    else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode<0) {
        current_mode = max_modes - 1;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      //Serial.println(current_mode);
      run_mode(current_mode);
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}

void set_time_zone() {
  
  int temp_hour = 0;
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      if (temp_hour>14) {
        temp_hour = -12;
      }
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour<-12) {
        temp_hour = 14;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      temp_hour = temp_hour*3600;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = 0;
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 30;
      temp_minute = temp_minute % 60;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 30;
      if (temp_minute<0) {
        temp_minute = 30;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      temp_minute = temp_minute*60;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
  configTime(temp_hour+temp_minute, UTC_OFFSET_DST, NTP_SERVER);
  display.clearDisplay();
  print_line("Time is set", 0, 0, 2);
  delay(1000);
}


void set_alarm(int alarm) {
  int temp_hour = alarm_hours[alarm];
  alarm_enabled = true;
  alarm_triggered[alarm] == false;
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour<0) {
        temp_hour = 23;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute<0) {
        temp_minute = 59;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  delay(1000);
}

void run_mode(int mode) {
  if (mode == 0) {
    set_time_zone();
  }

  else if (mode == 1 || mode == 2 || mode ==3) {
    set_alarm(mode - 1);
  }

  else if (mode == 4) {
    alarm_enabled = false;
    display.clearDisplay();
    print_line("All alarms disabled",0,0,2);
  }
}

void check_temp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (data.temperature > 32) {
    display.clearDisplay();
    print_line("TEMP HIGH", 0, 40, 1);
  }
  else if (data.temperature < 26) {
    display.clearDisplay();
    print_line("TEMP LOW", 0, 40, 1);
  }

  if (data.humidity > 80) {
    display.clearDisplay();
    print_line("Humidtiy HIGH", 0, 50, 1);
  }
  else if (data.humidity < 60) {
    display.clearDisplay();
    print_line("TEMP LOW", 0, 50, 1);
  }
}



