#include <Elegoo_GFX.h>                   // Core graphics library
#include <Elegoo_TFTLCD.h>                // Hardware-specific library
#include <TouchScreen.h>                  // Touch Support
#include <TimerOne.h>


//SETTINGS
const int TIME_WORK = 25;
const int TIME_BREAK = 5;
const int TIME_SET = 20;



#define TS_MINX 920
#define TS_MINY 120
#define TS_MAXX 150
#define TS_MAXY 940
#define YP A3                             // must be an analog pin, use "An" notation!
#define XM A2                             // must be an analog pin, use "An" notation!
#define YM 9                              // can be a digital pin
#define XP 8                              // can be a digital pin

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// macros for color (16 bit)
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define LCD_CS A3                         // Chip Select goes to Analog 3
#define LCD_CD A2                         // Command/Data goes to Analog 2
#define LCD_WR A1                         // LCD Write goes to Analog 1
#define LCD_RD A0                         // LCD Read goes to Analog 0
#define LCD_RESET A4                      // Can alternately just connect to Arduino's reset pin

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);



long ms = 0, segs = 1490, setSegs = 0;

unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;

#define MODE_WORK 0
#define MODE_BREAK 1
#define MODE_PAUSE 2

int mode_current = MODE_PAUSE, mode_last = MODE_WORK;
int setStatus = MODE_WORK;

                        int score;

void setup() {

  //Timer One     
  Timer1.initialize (1000); // siendo el tiempo 1000 = 1 milisegundo
  Timer1.attachInterrupt(timerOne); //siendo timer el nombre de la funcion que se va a crear

  Serial.begin(9600);




  tft.reset();                            // Reset LCD
  tft.begin(0x9341);                      // Initialise LCD
  fillScreen();


  startMillis = millis();  //initial start time
}



void loop() {


  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis - startMillis >= 100)  //test whether the period has elapsed
  {
    iterateEvery100Milis();
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }


}

void machineGeneral() {
  switch (mode_current) {
    case MODE_WORK:
      break;
    case MODE_BREAK:
      break;
    case MODE_PAUSE:
      break;
  }

}

void iterateEverySecond() {
  processIfTimeOut();
  reloadScreen();



}

void reloadScreen() {
  printTime();
  printTimeSet();
  printScore();
  printMode();
}

void addScore(int amount) {
  score += amount;
  reloadScreen();
}

void iterateEvery100Milis() {

  machineGeneral();

  TSPoint p = ts.getPoint();


  boolean led_state = false;

  //  tft.drawRect( 0, 240, btnsWidth, 80, WHITE);
  //  tft.drawRect( btnsWidth, 240, btnsWidth, 80, WHITE);
  //  tft.drawRect( btnsWidth * 2, 240, btnsWidth, 80, WHITE);
  //  tft.drawRect( btnsWidth * 3, 240, btnsWidth, 80, WHITE);
  int btnsWidth = 240 / 4;

  if (p.z > 10 && p.z < 1000)             // Check touch validity
  {
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    p.y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
    if (p.x > 0 && p.x < 240)
    {
      if (p.y > 240 && p.y < 320)
      {
        if (p.x > 0 && p.x < btnsWidth) {
          playPressed();
        }
        else if (p.x > btnsWidth && p.x < 2 * btnsWidth ) {
          resetPressed();
        }
        else if (p.x > 2 * btnsWidth && p.x < 3 * btnsWidth ) {
          plusPressed();
        }
        else if (p.x > 3 * btnsWidth && p.x < 4 * btnsWidth ) {
          minusPressed();
        }
      }

      if (p.y > 160 && p.y < 240)
      {
        if (p.x > 0 && p.x < btnsWidth) {
          playPressed();
        }
        else if (p.x > btnsWidth && p.x < 2 * btnsWidth ) {
          resetPressed();
        }
        else if (p.x > 2 * btnsWidth && p.x < 3 * btnsWidth ) {
          plusPressed();
        }
        else if (p.x > 3 * btnsWidth && p.x < 4 * btnsWidth ) {
          minusPressed();
        }
      }
    }


  }
}

void playPressed() {
  //if it is work will be geting work, paused to is the normal and so.
  int mode_hold = mode_current;
  switch (mode_current) {
    case MODE_WORK:
      mode_current = MODE_PAUSE;
      break;

    case MODE_BREAK:
      mode_current = MODE_PAUSE;
      break;

    case MODE_PAUSE:
      mode_current = mode_last;
      break;
    default:
      break;

  }

  mode_last = mode_hold;
  reloadScreen();
}


void resetPressed() {
  resetTimer();

}

void resetTimer() {
  ms = 0;
  segs = 0;
  reloadScreen();
}

void plusPressed() {
  addScore(1);
  reloadScreen();
}
void minusPressed() {
  addScore(-1);
  reloadScreen();
}


void printTime() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  tft.fillRect(0, 0, 240, 240, BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor((240 / 3), 80);
  String message = "time: " + (String)((int) segs / 60) + ":" + ((String)((int)segs % 60));
  tft.println(message);
}

void printTimeSet() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor((240 / 3), 100);
  String message = "Set: " + (String)((int) setSegs / 60) + ":" + ((String)((int)setSegs % 60));
  tft.println(message);
}


void printScore() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  tft.setTextColor(WHITE);
  tft.setTextSize(2); \
  tft.setCursor((240 / 3), 60);
  String message = "score: " + (String)score;
  tft.println(message);
}



void printMode() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor((240 / 3), 40);
  String message = "mode: ";
  switch (mode_current) {
    case MODE_WORK:
      message = message + "WORK";
      break;
    case MODE_BREAK:
      message = message + "BREAK";
      break;
    case MODE_PAUSE:
      message = message + "PAUSE";
      break;
  }

  tft.println(message);
}

void fillScreen() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  tft.fillScreen(BLACK);                  // Black Background
  //tft.fillRect(0, 0, 240, 160, GREEN);    // Upper GREEN Rectange
  tft.fillRect(0, 240, 240, 160, RED);    // Lower RED Rectange

  //draw all the different rects
  int btnsWidth = 240 / 4;
  tft.drawRect( 0, 240, btnsWidth, 80, WHITE);
  tft.drawRect( btnsWidth, 240, btnsWidth, 80, WHITE);
  tft.drawRect( btnsWidth * 2, 240, btnsWidth, 80, WHITE);
  tft.drawRect( btnsWidth * 3, 240, btnsWidth, 80, WHITE);


  tft.setTextColor(WHITE);                // Set Text Proporties
  tft.setTextSize(2);
  tft.setCursor((tft.width() / 2) - 18, 40);
  tft.println("UwU");                 // Write Text on LCD
  showButtons();




}

void showButtons() {

  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  switch (mode_current) {
    case MODE_BREAK:

      tft.fillRect(0, 240, 240, 160, RED);    // Lower RED Rectange
      break;
    case MODE_WORK:

      tft.fillRect(0, 240, 240, 160, GREEN);    // Lower RED Rectange
      break;
  }



  //draw all the different rects
  int btnsWidth = 240 / 4;
  tft.drawRect( 0, 240, btnsWidth, 80, WHITE);
  tft.drawRect( btnsWidth, 240, btnsWidth, 80, WHITE);
  tft.drawRect( btnsWidth * 2, 240, btnsWidth, 80, WHITE);
  tft.drawRect( btnsWidth * 3, 240, btnsWidth, 80, WHITE);

  // play pause, reset, AddScore, Less Score
  tft.setCursor(btnsWidth * 1 / 2, 240 + 80 / 2);
  tft.println("P");

  tft.setCursor(btnsWidth * 3 / 2, 240 + 80 / 2);
  tft.println("R");

  tft.setCursor(btnsWidth * 5 / 2, 240 + 80 / 2);
  tft.println("+");

  tft.setCursor(btnsWidth * 7 / 2, 240 + 80 / 2);
  tft.println("-");
}

void processIfTimeOut() {
  switch (mode_current) {
    case MODE_BREAK:
      if (segs >= (60 * TIME_BREAK )) {
        switchModeUI();
        mode_current = MODE_WORK;
      }
      break;
    case MODE_WORK:
      if (segs >= (60 * TIME_WORK )) {
        switchModeUI();
        addScore(1);
        mode_current = MODE_BREAK;
      }
      break;
    default:
      break;
  }
}


void beep() {
  //beep
}

void switchModeUI() {
  resetTimer();
  beep();
  showButtons();

}


void timerOne(void) {
  if (mode_current == MODE_PAUSE ) {
    return;
  }
  ms = ms + 1;
  if (ms >= 1000) {
    iterateEverySecond();
    segs = segs + 1;
    ms = 0;
    if (setStatus == MODE_WORK) {
      setSegs = setSegs + 1;
    }
  }
}
