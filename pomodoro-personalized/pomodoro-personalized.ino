#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>   // Touch Support
#include <TimerOne.h>

//SETTINGS
const int TIME_WORK = 1;
const int TIME_BREAK = 5;
const int TIME_SET = 20;

#define TS_MINX 920
#define TS_MINY 120
#define TS_MAXX 150
#define TS_MAXY 940
#define YP A3 // must be an analog pin, use "An" notation!
#define XM A2 // must be an analog pin, use "An" notation!
#define YM 9  // can be a digital pin
#define XP 8  // can be a digital pin

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define HEIGHT 300
#define WIDTH 240

// macros for color (16 bit)
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define LCD_CS A3    // Chip Select goes to Analog 3
#define LCD_CD A2    // Command/Data goes to Analog 2
#define LCD_WR A1    // LCD Write goes to Analog 1
#define LCD_RD A0    // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

unsigned long startMillis; //some global variables available anywhere in the program
unsigned long currentMillis;

#define MODE_WORK 0
#define MODE_BREAK 1
#define MODE_PAUSE 2

#define RED_LED 28
#define BLUE_LED 29
#define GREEN_LED 30

class SetType
{
public:
    double time_minutes;
    String name;
    int timesCompleted = 0;
    SetType(double inTime, String inName)
    {
        time_minutes = inTime;
        name = inName;
    }

    long getTimeInSeconds()
    {
        return time_minutes * 60;
    }

    void completeSetOnce()
    {
        timesCompleted = timesCompleted + 1;
    }

    void reset()
    {
        timesCompleted = 0;
    }
    String getNameComplete()
    {
        return name + "-" + (String)timesCompleted;
    }
};

class ModeType
{
public:
    String name = "Untitled";
    double time_segs = 25 * 60;
    double reward = 1;
    int color = RED;
    double breaktime_segs = 5 * 60;
    int break_color = GREEN;

    ModeType(String namein, double time_segsin, double rewardin, double breaktime_segsin)
    {
        name = namein;
        time_segs = time_segsin;
        reward = rewardin;
        breaktime_segs = breaktime_segsin;
    }
    double getTimeInMinutes()
    {
        return time_segs / 60;
    }
};

class MetaData
{       // The class
public: // Access specifier
    long time_segs, time_ms;
    long set_segs;
    long totalTimeElapsedToday = 0;
    int mode_current;
    int mode_last;
    int set_status;
    int score = 0;
    int setIndex = 0;

    int currentModeTypeIndex;

    // Dynamic
    int set_types_sizes = 4;
    SetType set_types[4]{{.1, "6s"}, {20, "20m"}, {50, "50m"}, {100, "100m"}};
    ModeType mode_types[4]{
        {"Work", 25 * 60, 1, 5 * 60}, {"War", 50 * 60, 3, 10 * 60}, {"Demon", 60 * 60, 4, 0 * 60}, {"Test", 10, 1, 10}};

    Metadata()
    {
        init();
        currentModeTypeIndex = 0;
    }

    void nextModeType()
    {
        currentModeTypeIndex++;
        if (currentModeTypeIndex >= set_types_sizes)
        {
            currentModeTypeIndex = 0;
        }
    }

    int getCurrentModeTimeInSeconds()
    {
        return (int)getCurrentMode().time_segs;
    }

    int getCurrentModeBreakTimeInSeconds()
    {
        return (int)getCurrentMode().breaktime_segs;
    }

    ModeType getCurrentMode()
    {
        return mode_types[currentModeTypeIndex];
    }

    void changeModeType()
    {
        nextModeType();
        // time_segs = 0;
        // time_ms
    }

    void nextSetType()
    {
        setIndex++;
        if (setIndex >= set_types_sizes)
        {
            setIndex = 0;
        }
    }

    SetType getCurrentSet()
    {
        return set_types[setIndex];
    }

    void completeCurrentSet()
    {
        set_types[setIndex].completeSetOnce();
    }

    void resetSets()
    {
        //Iterate every set and reset them
        for (int i = 0; i < set_types_sizes; i++)
        {
            set_types[i].reset();
        }
    }

    void init()
    {
        mode_current = MODE_PAUSE;
        mode_last = MODE_WORK;
        set_status = MODE_PAUSE;
        time_segs = 0;
        time_ms = 1490;
        set_segs = 0;
        score = 0;
        resetSets();
    }

    void toggleSetType()
    {
        if (set_status == MODE_WORK)
        {
            set_status = MODE_BREAK;
            set_segs = 0;
        }
        else
        {
            set_segs = getCurrentSet().getTimeInSeconds();
            set_status = MODE_WORK;
        }
    }
};

MetaData metadata;

void setup()
{

    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    metadata = MetaData();
    resetDay();
    //  metadata.time_segs = 1480;

    //Timer One     
    Timer1.initialize(1000);          // siendo el tiempo 1000 = 1 milisegundo
    Timer1.attachInterrupt(timerOne); //siendo timer el nombre de la funcion que se va a crear

    Serial.begin(9600);

    tft.reset();       // Reset LCD
    tft.begin(0x9341); // Initialise LCD
    fillScreen();

    startMillis = millis(); //initial start time
}

void loop()
{

    currentMillis = millis();               //get the current "time" (actually the number of milliseconds since the program started)
    if (currentMillis - startMillis >= 100) //test whether the period has elapsed
    {
        iterateEvery100Milis();
        startMillis = currentMillis; //IMPORTANT to save the start time of the current LED state.
    }
}

void turnLedsOff()
{
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
}

void turnRed()
{
    turnLedsOff();
    digitalWrite(RED_LED, HIGH);
}

void turnBlue()
{
    turnLedsOff();
    digitalWrite(BLUE_LED, HIGH);
}

void turnGreen()
{
    turnLedsOff();
    digitalWrite(GREEN_LED, HIGH);
}

void turnLedsDependingState()
{
    turnLedsOff();
    switch (metadata.mode_current)
    {
    case MODE_WORK:
        turnRed();
        break;
    case MODE_BREAK:
        turnGreen();
        break;
    case MODE_PAUSE:

        break;
    }
    if (metadata.set_status == MODE_WORK)
    {
        turnBlue();
    }
}

void machineGeneral()
{
    switch (metadata.mode_current)
    {
    case MODE_WORK:
        break;
    case MODE_BREAK:
        break;
    case MODE_PAUSE:
        break;
    }
}
void iterateEverySecond()
{
    processIfTimeOut();
    reloadTimeOnly();

}

void reloadScreen(){
    reloadDataScreen();
    showButtons();
}

void reloadDataScreen()
{
    turnLedsDependingState();
    fillTextOnlyBlack();
    printTime();
    printTimeSet();
    printscore();
    printMode();
}

void addscore(int amount)
{
    metadata.score += amount;
    reloadDataScreen();
}

void iterateEvery100Milis()
{

    machineGeneral();

    TSPoint p = ts.getPoint();

    boolean led_state = false;

    //  tft.drawRect( 0, 240, btnsWidth, 80, WHITE);
    //  tft.drawRect( btnsWidth, 240, btnsWidth, 80, WHITE);
    //  tft.drawRect( btnsWidth * 2, 240, btnsWidth, 80, WHITE);
    //  tft.drawRect( btnsWidth * 3, 240, btnsWidth, 80, WHITE);

    //   ### BUTTON PRESS
    int btnsWidth = 240 / 4;

    if (p.z > 10 && p.z < 1000)
    {
        p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
        p.y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
        if (p.x > 0 && p.x < 240)
        {
            if (p.y > 240 && p.y < 320)
            {
                if (p.x > 0 && p.x < btnsWidth)
                {
                    playPressed();
                }
                else if (p.x > btnsWidth && p.x < 2 * btnsWidth)
                {
                    resetPressed();
                }
                else if (p.x > 2 * btnsWidth && p.x < 3 * btnsWidth)
                {
                    plusPressed();
                }
                else if (p.x > 3 * btnsWidth && p.x < 4 * btnsWidth)
                {
                    minusPressed();
                }
            }

            if (p.y > 160 && p.y < 240)
            {
                if (p.x > 0 && p.x < btnsWidth)
                {
                    playSetPressed();
                }
                else if (p.x > btnsWidth && p.x < 2 * btnsWidth)
                {
                    resetDay();
                }
                else if (p.x > 2 * btnsWidth && p.x < 3 * btnsWidth)
                {
                    changeSetPressed();
                }
                else if (p.x > 3 * btnsWidth && p.x < 4 * btnsWidth)
                {
                    modeTypePressed();
                }
            }
        }
    }
}

void playPressed()
{
    //if it is work will be geting work, paused to is the normal and so.
    int mode_hold = metadata.mode_current;
    switch (metadata.mode_current)
    {
    case MODE_WORK:

        metadata.mode_current = MODE_PAUSE;
        break;

    case MODE_BREAK:

        metadata.mode_current = MODE_PAUSE;
        break;

    case MODE_PAUSE:
        metadata.mode_current = metadata.mode_last;
        break;
    default:
        break;
    }

    metadata.mode_last = mode_hold;
    reloadScreen();
}

void playSetPressed()
{

    metadata.toggleSetType();

    reloadDataScreen();
}

void changeSetPressed()
{
    metadata.nextSetType();
    fillScreen();
    reloadDataScreen();
}

void resetPressed()
{
    if (metadata.mode_current == MODE_BREAK)
    {
        metadata.time_segs = metadata.getCurrentModeBreakTimeInSeconds();
        return;
    }
    resetTimer();
}

void resetTimer()
{
    metadata.time_segs = 0;
    reloadDataScreen();
}

void plusPressed()
{
    addscore(1);
    reloadDataScreen();
}
void minusPressed()
{
    addscore(-1);
    reloadDataScreen();
}

void modeTypePressed()
{
    metadata.changeModeType();
    reloadDataScreen();
}

void printTime()
{
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    

    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor((240 / 3), 80);
    String message = "time: " ;
    tft.println(message);
    printTimeText();
}

void fillTextOnlyBlack(){

    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    tft.fillRect(0, 0, 240, 240 - 80, BLACK);
}

void reloadTimeOnly()
{

    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    tft.fillRect(WIDTH*2/3-4, 240*1/3, WIDTH/4+10, 50, BLACK);
    printTimeText();
    printTimeSet();


    
}

void printTimeText(){
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor((240 / 3)+40, 80);
    String message = "   " + (String)((int)metadata.time_segs / 60) + ":" + ((String)((int)metadata.time_segs % 60));
    tft.println(message);
}

void printTimeSet()
{
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor((240 / 3), 100);
    if (metadata.set_status == MODE_WORK)
    {
        String message = metadata.getCurrentSet().getNameComplete() + (String)(int)metadata.getCurrentSet().timesCompleted + " " + (String)((int)metadata.set_segs / 60) + ":" + ((String)((int)metadata.set_segs % 60));
        tft.println(message);
    }
}

void printscore()
{
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor((240 / 3), 60);
    String message = "score: " + (String)metadata.score;
    tft.println(message);
}

void resetDay()
{
    metadata.init();
    reloadScreen();
    
}

void printMode()
{
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor((240 / 3), 40);
    String message = "mode: ";
    String currentModeName = metadata.getCurrentMode().name;
    switch (metadata.mode_current)
    {
    case MODE_WORK:
        message = message + currentModeName;
        break;
    case MODE_BREAK:
        message = message + currentModeName + "-B";
        break;
    case MODE_PAUSE:
        message = message + currentModeName + "-P";
        break;
    }

    tft.println(message);
}

void fillScreen()
{
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    tft.fillScreen(BLACK); // Black Background
    //tft.fillRect(0, 0, 240, 160, GREEN);    // Upper GREEN Rectange
    tft.fillRect(0, 240, 240, 160, RED); // Lower RED Rectange

    //draw all the different rects
    int btnsWidth = 240 / 4;
    tft.drawRect(0, 240, btnsWidth, 80, WHITE);
    tft.drawRect(btnsWidth, 240, btnsWidth, 80, WHITE);
    tft.drawRect(btnsWidth * 2, 240, btnsWidth, 80, WHITE);
    tft.drawRect(btnsWidth * 3, 240, btnsWidth, 80, WHITE);

    tft.setTextColor(WHITE); // Set Text Proporties
    tft.setTextSize(2);
    tft.setCursor((tft.width() / 2) - 18, 40);
    tft.println("UwU"); // Write Text on LCD
    showButtons();

}

void showButtons()
{

    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    int btnsWidth = 240 / 4;

    int height = 240 - 80;

    if (metadata.getCurrentSet().timesCompleted > 0)
    {

        tft.fillRect(btnsWidth * 2, height, btnsWidth, 80, YELLOW);
    }

    switch (metadata.mode_current)
    {
    case MODE_BREAK:

        tft.fillRect(0, 240, 240, 160, GREEN); 

        break;
    case MODE_WORK:

        tft.fillRect(0, 240, 240, 160, RED); 

        break;
    }

    //draw all the different rects
    tft.drawRect(0, 240, btnsWidth, 80, WHITE);
    tft.drawRect(btnsWidth, 240, btnsWidth, 80, WHITE);
    tft.drawRect(btnsWidth * 2, 240, btnsWidth, 80, WHITE);
    tft.drawRect(btnsWidth * 3, 240, btnsWidth, 80, WHITE);

    tft.setCursor(btnsWidth * 1 / 2, 240 + 80 / 2);
    tft.println("P");

    tft.setCursor(btnsWidth * 3 / 2 - 10, 240 + 80 / 2);

    tft.println(metadata.mode_current == MODE_BREAK ? "S" : "R");

    tft.setCursor(btnsWidth * 5 / 2, 240 + 80 / 2);
    tft.println("+");

    tft.setCursor(btnsWidth * 7 / 2, 240 + 80 / 2);
    tft.println("-");

    tft.drawRect(0, height, btnsWidth, 80, WHITE);
    tft.drawRect(btnsWidth, height, btnsWidth, 80, WHITE);
    tft.drawRect(btnsWidth * 2, height, btnsWidth, 80, WHITE);
    tft.drawRect(btnsWidth * 3, height, btnsWidth, 80, WHITE);

    tft.setCursor(btnsWidth * 1 / 2 - 20, 240 - 40);
    tft.println("Set");

    tft.setCursor(btnsWidth * 3 / 2 - 10, 240 - 40);
    tft.println("RD");

    tft.setCursor(btnsWidth * 7 / 2 - 10, 240 - 40);
    tft.println("CM");

    tft.setTextSize(1);
    tft.setCursor(btnsWidth * 5 / 2 - 20, 240 - 40);
    tft.println(metadata.getCurrentSet().getNameComplete());

    //
    //  tft.setCursor(btnsWidth * 5 / 2, 240 );
    //  tft.println("+");
    //
    //  tft.setCursor(btnsWidth * 7 / 2, 240 -40);
    //  tft.println("-");
}

void processIfTimeOut()
{
    switch (metadata.mode_current)
    {
    case MODE_BREAK:
        if (metadata.time_segs >= (metadata.getCurrentModeBreakTimeInSeconds()))
        {
            
            metadata.mode_current = MODE_WORK;
            switchModeUI();
        }
        break;
    case MODE_WORK:
        if (metadata.time_segs >= (metadata.getCurrentModeTimeInSeconds()))
        {
            
            addscore(1);
            metadata.mode_current = MODE_BREAK;
            switchModeUI();
        }
        break;
    default:
        break;
    }

    if (metadata.set_segs <= 0 && metadata.set_status == MODE_WORK)
    {
        metadata.toggleSetType();
        metadata.completeCurrentSet();

        fillScreen();
        reloadDataScreen();
    }
}

void beep()
{
    //beep
}

void switchModeUI()
{
    resetTimer();
    beep();
    showButtons();
}

void timerOne(void)
{
    if (metadata.mode_current == MODE_PAUSE)
    {
        return;
    }
    metadata.time_ms = metadata.time_ms + 1;
    if (metadata.time_ms >= 1000)
    {
        iterateEverySecond();
        metadata.time_segs = metadata.time_segs + 1;
        metadata.time_ms = 0;
        if (metadata.set_status == MODE_WORK)
        {
            metadata.set_segs = metadata.set_segs - 1;
        }
    }
}
