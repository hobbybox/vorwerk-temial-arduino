#include <Arduino.h>
#include <TimerOne.h>
#include <math.h>
#include <TouchScreen.h>
#include <LCDWIKI_GUI.h>
#include <LCDWIKI_KBV.h>
#include <SPI.h>
#include <SD.h>

#include "font.h"
#include "switch_font.c"


void GetTemp(void);
void FillHeatingTank(int ml);
void FillTeaTank(int ml);
void HeatingWater(int temp);
void GetSensValues(void);
void PrintSerial(void);
void ParkPosition(void);
void Jauche(int ml);
void TeaOut(int ml);
void WakeUpTea(void);
void MakeTea(String teaFile);
void AddIngredient(void);
void TeaIsReady(void);
void ShowBrewTime(void);
void HotPlateOn(void);
void HotPlateOff(void);
void ReadTeaFile(String fileName);
void DisplayTeaNames(void);
void DisplayRecipeName(String recipeName);
void DisplayRecipeMessage(String message);
void DisplayRecipeMessage(String message, String message2);
void ClearRecipe(void);
void GetTea(void);

void MainScreen(void);


#define TS_MINX 906
#define TS_MAXX 116

#define TS_MINY 92
#define TS_MAXY 952

#define STATUS_X 10
#define STATUS_Y 65

#define MINPRESSURE 10
#define MAXPRESSURE 1000

#define YP A3
#define XM A2
#define YM 9
#define XP 8

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define NAVY 0x000F
#define DARKGREEN 0x03E0
#define DARKCYAN 0x03EF
#define MAROON 0x7800
#define PURPLE 0x780F
#define OLIVE 0x7BE0
#define LIGHTGREY 0xC618
#define DARKGREY 0x7BEF
#define ORANGE 0xFD20
#define GREENYELLOW 0xAFE5
#define PINK 0xF81F

#define TurntableSpeed 44
#define TurntableIn1 23
#define TurntableIn2 22

#define TeaElevatorIn1 24
#define TeaElevatorIn2 25

#define PumpIn1 26
#define PumpIn2 27

#define PumpOut1 28
#define PumpOut2 29

#define TopCoverClosed 20
#define TeaPotIn 40

#define WasteWaterTankAvailableBtn 48
#define WasteWaterTankFullBtn A8

#define WaterHeater 42
#define WarmingPlate 43

#define TurntableSens1 A5
#define TurntableSens2 A6

#define WaterTemperature A7
#define WaterTankCapacity 30

#define Speaker 31

#define LED1 45
#define LED2 46

#define CS 53


File myFile;

const int queryNum = 5;
const int ntcNominal = 10000;
const int tempNominal = 25;
const int bCoefficient = 3977;
const int seriesResistance = 10000;
const long wateringTime = 80000;
const int wateringTimePerMl = 133;
const int elevatorDistanceMax = 22;
const double distancePerMl = 0.044;
const int waterTempMax = 95;
const int mlMax = 600;
const int teaMlOffset = 20;
const int recipeLength = 30;
const int teaListMax = 30;
const int LEDPWM = 2;

String recipe[recipeLength][2];
String teaList[teaListMax];

int query[queryNum];
float average = 0;
float temp;
float cfactor = 5.0;

int ledCounter;
int elevatorWaitCounter;
int teaMl;
int teaMlSum;
int wakeUpML;
int mlCounter;
int waterTemp;
int waterTargetTemp;
int teaElevatorDistance;
int teaElevatorCounter;
int elevatorStartCounter;
double teaBrewingMin;
unsigned long teaBrewingMs;
unsigned long teaBrewingCounter;
unsigned long brewingStartTime;
int teaBrewingStepCount;
bool ledDirection;
bool elevatorIsDown;
bool waterIsHeated;
bool topCoverClosed;
bool teaPotIn;
bool wasteWaterTankAvailable;
bool wasteWaterTankFull;
bool turntableSens1;
bool turntableSens2;
bool jauche;
bool teaIsBrewing;
bool in1;
bool in2;
bool warmingPlate;


LCDWIKI_KBV tft(ILI9486, A3, A2, A1, A0, A4);  //320x480
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);


boolean is_pressed(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t px, int16_t py) {
  if ((px > x1 && px < x2) && (py > y1 && py < y2)) {
    return true;
  } else {
    return false;
  }
}


void setup() {
  Serial.begin(9600);

  pinMode(WaterHeater, OUTPUT);
  pinMode(WarmingPlate, OUTPUT);

  digitalWrite(WaterHeater, HIGH);
  digitalWrite(WarmingPlate, LOW);

  pinMode(TurntableSpeed, OUTPUT);
  pinMode(TurntableIn1, OUTPUT);
  pinMode(TurntableIn2, OUTPUT);

  pinMode(TeaElevatorIn1, OUTPUT);
  pinMode(TeaElevatorIn2, OUTPUT);

  pinMode(TeaPotIn, INPUT);
  digitalWrite(TeaPotIn, HIGH);

  pinMode(TopCoverClosed, INPUT);
  digitalWrite(TopCoverClosed, HIGH);

  pinMode(WasteWaterTankAvailableBtn, INPUT);
  digitalWrite(WasteWaterTankAvailableBtn, HIGH);

  pinMode(TurntableSens1, INPUT);
  pinMode(TurntableSens2, INPUT);

  pinMode(WaterTemperature, INPUT);
  pinMode(WaterTankCapacity, INPUT);
  pinMode(WasteWaterTankFullBtn, INPUT);

  pinMode(Speaker, OUTPUT);

  pinMode(LED1, OUTPUT);
  analogWrite(LED1, LEDPWM);

  pinMode(LED2, OUTPUT);
  analogWrite(LED2, LEDPWM);

  analogWrite(TurntableSpeed, 200);

  digitalWrite(PumpIn1, LOW);
  digitalWrite(PumpIn2, LOW);

  digitalWrite(PumpOut1, LOW);
  digitalWrite(PumpOut2, LOW);

  tft.Init_LCD();


  if (!SD.begin(CS)) {
    tft.Fill_Screen(BLUE);
    tft.Set_Text_Back_colour(BLUE);
    tft.Set_Text_colour(WHITE);
    tft.Set_Text_Size(3);
    Serial.println("SD-Card fail");
    tft.Print_String("SD Card Init fail!", 0, 200);
    while(true)
    {
      delay(1000); 
    }    
  } else {
    Serial.println("SD-Card init OK!");
  }

  tft.Fill_Screen(WHITE);
  tft.Set_Text_colour(WHITE);
  tft.Set_Text_Back_colour(DARKGREY);
  tft.Set_Text_Size(3);

  wasteWaterTankAvailable = false;
  wasteWaterTankFull = false;
  waterIsHeated = false;
  warmingPlate = false;

  turntableSens1 = false;
  turntableSens2 = false;
  elevatorIsDown = false;
  ledCounter = 50;

  jauche = true;
  teaIsBrewing = false;

  in1 = true;
  in2 = false;

  GetSensValues();
  ParkPosition();
  for (int i = 0; i <= teaListMax; i++) {
    ReadTeaFile("/Rezepte/" + (String)i + ".txt");
    teaList[i] = recipe[0][1];
    if (teaList[i] == "") {
      break;
    }
  }
  MainScreen();
  DisplayTeaNames();

  teaMl = 360;
  wakeUpML = 150;
  waterTargetTemp = 50;
  teaBrewingMin = 3;
  //elevatorIsDown = true;
  //warmingPlate = true;

  //FillHeatingTank(teaMl);
  //HeatingWater(waterTargetTemp);
  //teaIsBrewing = true;

  //teaElevatorDistance = teaMl * distancePerMl - 3;
  //FillTeaTank(teaMl);
  //TeaIsReady();
  //TeaOut(teaMl);
  //WakeUpTea();
  //Jauche(wakeUpML);
  //MakeTea("/Rezepte/3.txt");
  //digitalWrite(TeaElevatorIn1, HIGH);
  //digitalWrite(TeaElevatorIn2, LOW);
  //delay(elevatorDistanceMax * 1000);
  //ReadTeaFile("Rezepte/GrInTee.txt");
  //HotPlateOff();
}



void loop() {
  GetTea();
}


void GetTea(void) {
  GetSensValues();

  while (true) {
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      p.x = map(p.x, TS_MINX, TS_MAXX, tft.Get_Display_Width(), 0);
      p.y = map(p.y, TS_MINY, TS_MAXY, tft.Get_Display_Height(), 0);

      if (p.y > 0 && p.y < 100 && p.x > 165 && p.x < 320) {
        if (warmingPlate) {
          HotPlateOff();
        } else {
          HotPlateOn();
        }
      }

      if (p.y > 110 && p.y < 135) {
        MakeTea("/Rezepte/0.txt");
      }

      if (p.y > 135 && p.y < 160) {
        MakeTea("/Rezepte/1.txt");
      }

      if (p.y > 160 && p.y < 185) {
        MakeTea("/Rezepte/2.txt");
      }

      if (p.y > 185 && p.y < 210) {
        MakeTea("/Rezepte/3.txt");
      }

      if (p.y > 210 && p.y < 235) {
        MakeTea("/Rezepte/4.txt");
      }

      if (p.y > 235 && p.y < 260) {
        MakeTea("/Rezepte/5.txt");
      }

      if (p.y > 260 && p.y < 285) {
        MakeTea("/Rezepte/6.txt");
      }

      if (p.y > 285 && p.y < 310) {
        MakeTea("/Rezepte/7.txt");
      }

      if (p.y > 310 && p.y < 335) {
        MakeTea("/Rezepte/8.txt");
      }

      if (p.y > 335 && p.y < 360) {
        MakeTea("/Rezepte/9.txt");
      }
    }

    if (ledDirection) {
      ledCounter++;
    } else {
      ledCounter--;
    }

    if (ledCounter >= 50) {
      ledDirection = false;
    }

    if (ledCounter <= 10) {
      ledDirection = true;
    }

    analogWrite(LED1, LEDPWM);
    analogWrite(LED2, LEDPWM);
    //delay(60);
  }
}


void DisplayTeaNames() {
  tft.Set_Text_colour(WHITE);
  tft.Set_Text_Back_colour(DARKGREY);
  tft.Set_Text_Size(3);

  int pos = 110;
  for (int i = 0; i <= teaListMax; i++) {
    if (teaList[i] == "") {
      break;
    }
    tft.Fill_Rect(0, pos, 480, 24, DARKGREY);
    tft.Print_String(teaList[i], 10, pos);
    pos += 25;
  }
}


void ClearRecipe() {
  for (int i = 0; i <= recipeLength; i++) {
    recipe[i][0] = "";
    recipe[i][1] = "";
  }
}


void MainScreen(void) {
  tft.Fill_Screen(WHITE);
  tft.Set_Text_colour(WHITE);
  tft.Set_Text_Back_colour(DARKGREY);
  tft.Set_Text_Size(7);
  tft.Set_Draw_color(DARKGREY);

  tft.Fill_Rect(0, 0, 155, 100, DARKGREY);
  tft.Fill_Rect(165, 0, 155, 100, DARKGREY);

  tft.Fill_Rect(0, 380, 155, 100, DARKGREY);
  tft.Fill_Rect(165, 380, 155, 100, DARKGREY);

  tft.Print_String("<", 60, 400);
  tft.Print_String(">", 210, 400);

  if (warmingPlate) {
    HotPlateOn();
  } else {
    HotPlateOff();
  }
}


void DisplayRecipeName(String recipeName) {
  tft.Fill_Rect(0, 101, 320, 279, WHITE);
  tft.Set_Text_colour(WHITE);
  tft.Set_Text_Back_colour(DARKGREY);
  tft.Set_Text_Size(3);
  tft.Fill_Rect(0, 109, 320, 30, DARKGREY);
  tft.Print_String(recipeName, 5, 112);
}


void DisplayRecipeMessage(String message) {
  tft.Fill_Rect(0, 180, 320, 30, DARKGREY);
  tft.Set_Text_colour(WHITE);
  tft.Set_Text_Back_colour(DARKGREY);
  tft.Set_Text_Size(3);
  tft.Print_String(message, 5, 185);
}

void DisplayRecipeMessage(String message, String message2) {
  tft.Fill_Rect(0, 180, 320, 30, DARKGREY);
  tft.Fill_Rect(0, 250, 320, 30, DARKGREY);
  tft.Set_Text_colour(WHITE);
  tft.Set_Text_Back_colour(DARKGREY);
  tft.Set_Text_Size(3);
  tft.Print_String(message, 5, 185);
  tft.Print_String(message2, 5, 255);
}



void ReadTeaFile(String fileName) {
  myFile = SD.open(fileName, O_RDONLY);
  String fullString;

  if (myFile) {
    while (myFile.available()) {
      fullString = myFile.readString();
    }
    myFile.close();
  } else {
    Serial.print("error opening: ");
    Serial.println(fileName);
  }

  String name = fullString;
  String array[recipeLength];
  int r = 0, t = 0;

  for (int i = 0; i < fullString.length(); i++) {
    if (fullString[i] == ' ' || fullString[i] == ';') {
      if (i - r > 1) {
        array[t] = fullString.substring(r, i);
        t++;
      }
      r = (i + 1);
    }
  }

  for (int k = 0; k <= t; k++) {
    int count = 0;
    for (int y = 0; y <= array[k].length(); y++) {
      if (array[k][y] == ':') {
        count = y;
      }
    }
    if (k == 0) {
      recipe[k][0] = array[k].substring(0, count);
    } else {
      recipe[k][0] = array[k].substring(1, count);
    }
    recipe[k][1] = array[k].substring(count + 1, array[k].length());
  }
}


void MakeTea(String teaFile) {
  ClearRecipe();
  ReadTeaFile(teaFile);
  teaMlSum = 0;
  bool loop = true;

  for (int i = 0; i < recipeLength; i++) {
    if (recipe[i][0] == "name") {
      DisplayRecipeName(recipe[i][1]);
      DisplayRecipeMessage("Wassertank", "fuellen...");

      tft.Fill_Rect(0, 320, 155, 50, DARKGREY);
      tft.Fill_Rect(165, 320, 155, 50, DARKGREY);

      tft.Set_Text_colour(WHITE);
      tft.Set_Text_Back_colour(DARKGREY);
      tft.Set_Text_Size(3);

      tft.Print_String("Abbruch", 10, 335);
      tft.Print_String("Start", 200, 335);

      while (loop) {
        digitalWrite(13, HIGH);
        TSPoint p = ts.getPoint();
        digitalWrite(13, LOW);
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);

        if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
          p.x = map(p.x, TS_MINX, TS_MAXX, tft.Get_Display_Width(), 0);
          p.y = map(p.y, TS_MINY, TS_MAXY, tft.Get_Display_Height(), 0);

          if (p.y > 320 && p.y < 360 && p.x > 0 && p.x < 150) {
            delay(1000);
            MainScreen();
            DisplayTeaNames();
            GetTea();
          }

          if (p.y > 320 && p.y < 360 && p.x > 170 && p.x < 320) {
            tft.Fill_Screen(WHITE);
            tft.Set_Text_colour(WHITE);
            tft.Set_Text_Back_colour(DARKGREY);
            tft.Set_Text_Size(7);
            tft.Set_Draw_color(DARKGREY);

            tft.Fill_Rect(0, 0, 155, 100, DARKGREY);
            tft.Fill_Rect(165, 0, 155, 100, DARKGREY);

            tft.Fill_Rect(0, 180, 320, 30, DARKGREY);
            tft.Fill_Rect(0, 280, 320, 30, DARKGREY);

            tft.Fill_Rect(0, 380, 155, 100, DARKGREY);
            tft.Fill_Rect(165, 380, 155, 100, DARKGREY);

            DisplayRecipeName(recipe[0][1]);
            loop = false;
            analogWrite(LED1, LEDPWM);
            analogWrite(LED2, LEDPWM);
          }
        }
      }
    }

    if (recipe[i][0] == "text") {
      DisplayRecipeMessage(recipe[i][1]);
    }

    if (recipe[i][0] == "zutat" && recipe[i][1] == "dazugeben") {
      AddIngredient();
    }

    if (recipe[i][0] == "fuellen") {
      teaMl = recipe[i][1].toInt();
      teaMlSum = teaMlSum + (teaMl - teaMlOffset);
      DisplayRecipeMessage("Wasser einlassen!");
      FillHeatingTank(recipe[i][1].toInt() - teaMlOffset);
    }

    if (recipe[i][0] == "erhitzen") {
      HeatingWater(recipe[i][1].toInt());
    }

    if (recipe[i][0] == "ziehen") {
      teaElevatorDistance = teaMlSum * distancePerMl - 3;
      teaBrewingMin = recipe[i][1].toInt();
      DisplayRecipeMessage("Zubereitung...", "");
      FillTeaTank(teaMl - teaMlOffset);
      TeaIsReady();
    }

    if (recipe[i][0] == "tee" && recipe[i][1] == "ausgeben") {
      TeaOut(teaMlSum - teaMlOffset);
    }

    if (recipe[i][0] == "tee" && recipe[i][1] == "warmhalten") {
      HotPlateOn();
    }

    if (recipe[i][0] == "tee" && recipe[i][1] == "fertig") {
      break;
    }
  }
  Serial.println("ENDE");
  MainScreen();
  DisplayTeaNames();
  GetTea();
}


void AddIngredient(void) {
  GetSensValues();
  while (topCoverClosed) {
    GetSensValues();
    analogWrite(LED1, 50);
    analogWrite(LED2, 50);
    delay(1000);
    analogWrite(LED1, 0);
    analogWrite(LED2, 0);
    delay(1000);
  }

  analogWrite(LED1, 3);
  analogWrite(LED2, 3);

  GetSensValues();
  while (!topCoverClosed) {
    GetSensValues();
    DisplayRecipeMessage("Deckel schliessen!");
    Serial.println("Deckel schließen...");
    delay(2000);
  }
}



void ShowBrewTime() {
  if (teaIsBrewing) {
    unsigned long allSeconds = teaBrewingCounter / 1000;
    int runHours = allSeconds / 3600;
    int secsRemaining = allSeconds % 3600;
    int runMinutes = secsRemaining / 60;
    int runSeconds = secsRemaining % 60;
    char buf[21];

    if (teaBrewingCounter > 0) {
      sprintf(buf, "%02d:%02d", runMinutes, runSeconds);
      Serial.println(buf);
      tft.Print_String(buf, 110, 255);

      teaBrewingCounter = teaBrewingCounter - 1000;
      teaElevatorCounter++;
      elevatorStartCounter++;

      if (elevatorStartCounter > 130) {
        elevatorStartCounter = 30;
        elevatorWaitCounter = 0;
        in1 = true;
        in2 = false;
      }

      if (elevatorWaitCounter > teaElevatorDistance) {
        digitalWrite(TeaElevatorIn1, LOW);
        digitalWrite(TeaElevatorIn2, LOW);
      } else if (elevatorStartCounter > 80) {
        digitalWrite(TeaElevatorIn1, LOW);
        digitalWrite(TeaElevatorIn2, HIGH);
        elevatorWaitCounter++;
      } else if (teaElevatorCounter >= teaElevatorDistance && (teaMl >= 150 || teaMlSum >= 250) && elevatorStartCounter > 30) {
        teaElevatorCounter = 0;
        digitalWrite(TeaElevatorIn1, in1);
        digitalWrite(TeaElevatorIn2, in2);
        in1 = !in1;
        in2 = !in2;
        elevatorWaitCounter = 0;
      }
    }

    if (runMinutes == 0 && runSeconds <= 1) {
      Timer1.stop();
      Timer1.detachInterrupt();
      digitalWrite(TeaElevatorIn1, LOW);
      digitalWrite(TeaElevatorIn2, LOW);
      teaIsBrewing = false;
    }
  }
}



void WakeUpTea() {
  FillHeatingTank(wakeUpML);
  FillTeaTank(wakeUpML);
  delay(3000);
  Jauche(wakeUpML);
}



void FillHeatingTank(int ml) {
  GetSensValues();
  mlCounter = 0;
  ml = ml + 20;
  while (true) {
    while (!topCoverClosed) {
      Serial.println("Top-Cover schließen...");
      DisplayRecipeMessage("Deckel schliessen!");
      delay(2000);
      GetSensValues();
    }
    digitalWrite(PumpIn1, HIGH);

    if (mlCounter == ml) {
      digitalWrite(PumpIn1, LOW);
      break;
    }

    delay(wateringTimePerMl);
    mlCounter++;
  }
}



void HeatingWater(int temperature) {
  DisplayRecipeMessage("Wasser heizt auf!");
  tft.Fill_Rect(0, 250, 320, 30, DARKGREY);

  tft.Set_Text_colour(WHITE);
  tft.Set_Text_Back_colour(DARKGREY);
  tft.Set_Text_Size(3);

  if (temperature > waterTempMax) {
    temperature = waterTempMax;
  }

  while (true) {
    GetSensValues();
    tft.Print_String((String)waterTemp + " / " + (String)temperature, 100, 255);
    delay(1000);
    digitalWrite(WaterHeater, LOW);
    if (waterTemp >= temperature) {
      digitalWrite(WaterHeater, HIGH);
      waterIsHeated = true;
      break;
    }
  }
  delay(5000);
}



void FillTeaTank(int ml) {
  int counter = 0;
  teaBrewingCounter = teaBrewingMin * 60000;
  elevatorStartCounter = 0;
  if (topCoverClosed && teaPotIn) {
    if (!elevatorIsDown) {
      digitalWrite(TeaElevatorIn1, LOW);
      digitalWrite(TeaElevatorIn2, HIGH);
      delay(elevatorDistanceMax * 1000);
      digitalWrite(TeaElevatorIn2, LOW);
      elevatorIsDown = true;
    } else {
      delay(10000);
    }
    Timer1.initialize(1000000);
    Timer1.attachInterrupt(ShowBrewTime);
    Timer1.start();

    teaIsBrewing = true;
    while (counter != ml) {
      digitalWrite(PumpOut2, HIGH);
      delay(wateringTimePerMl - 105);
      counter++;
    }

    digitalWrite(PumpOut2, LOW);
    delay(300);
    digitalWrite(PumpOut2, HIGH);
    delay(800);
    digitalWrite(PumpOut2, LOW);
    delay(300);
    digitalWrite(PumpOut2, HIGH);
    delay(800);
    digitalWrite(PumpOut2, LOW);
    delay(300);
    digitalWrite(PumpOut2, HIGH);
    delay(2000);
    digitalWrite(PumpOut2, LOW);

    while (teaBrewingCounter > 0) {
      delay(1000);
    }
  } else if (!topCoverClosed) {
    Serial.println("Deckel schließen....");
    DisplayRecipeMessage("Deckel schliessen!");
  } else if (!teaPotIn) {
    Serial.println("Brüheinheit einsetzen...");
    DisplayRecipeMessage("Brüheinheit", "einsetzen...");
  }
}



void TeaIsReady() {
  Serial.println("TeaIsReady");
  digitalWrite(TeaElevatorIn1, HIGH);
  digitalWrite(TeaElevatorIn2, LOW);
  delay(elevatorDistanceMax * 1000);
  digitalWrite(TeaElevatorIn1, LOW);
  elevatorIsDown = false;
  delay(5000);
}


void TeaOut(int ml) {
  int counter = 0;
  GetSensValues();
  digitalWrite(TurntableIn2, HIGH);

  while (true) {
    GetSensValues();
    if (turntableSens1 == 0 && turntableSens2 == 1) {
      delay(350);
      digitalWrite(TurntableIn2, LOW);
      break;
    }
  }

  while (counter != ml) {
    delay(wateringTimePerMl - 34);
    counter++;
  }
  ParkPosition();
}


void HotPlateOn() {
  tft.Fill_Rect(165, 0, 155, 100, RED);
  tft.Set_Text_colour(WHITE);
  tft.Set_Text_Back_colour(RED);
  tft.Set_Text_Size(2);
  tft.Print_String("Heizplatte", 170, 60);
  tft.Print_String("an", 170, 80);

  warmingPlate = true;
  digitalWrite(WarmingPlate, HIGH);
}


void HotPlateOff() {
  tft.Fill_Rect(165, 0, 155, 100, DARKGREY);
  tft.Set_Text_colour(WHITE);
  tft.Set_Text_Back_colour(DARKGREY);
  tft.Set_Text_Size(2);
  tft.Print_String("Heizplatte", 170, 60);
  tft.Print_String("aus", 170, 80);
  warmingPlate = false;
  digitalWrite(WarmingPlate, LOW);
}



void ParkPosition() {
  Serial.println("ParkPosition");
  if (jauche) {
    digitalWrite(TurntableIn2, HIGH);
    delay(500);
    digitalWrite(TurntableIn2, LOW);
  }
  digitalWrite(TurntableIn1, HIGH);
  while (true) {
    GetSensValues();
    if (turntableSens1 == 1 && turntableSens2 == 1) {
      digitalWrite(TurntableIn1, LOW);
      break;
    }
  }
  jauche = false;
}



void Jauche(int ml) {
  Serial.println("Jauche");
  jauche = true;
  GetSensValues();
  int counter = 0;
  if (wasteWaterTankAvailable && !wasteWaterTankFull) {
    digitalWrite(TurntableIn1, HIGH);
    while (true) {
      GetSensValues();
      if (turntableSens1 == 1 && turntableSens2 == 0) {
        digitalWrite(TurntableIn1, LOW);
        break;
      }
    }

    while (counter != (ml * 2)) {
      delay((wateringTimePerMl - 24) / 2);
      counter++;
    }
  } else if (!wasteWaterTankAvailable) {
    Serial.println("Abwassertank einsetzen...");
    delay(1000);
    Jauche(wakeUpML);
  } else if (wasteWaterTankFull) {
    Serial.println("Abwassertank leeren...");
    delay(1000);
    Jauche(wakeUpML);
  }
  ParkPosition();
}


void GetSensValues() {
  GetTemp();
  teaPotIn = !digitalRead(TeaPotIn);
  topCoverClosed = !digitalRead(TopCoverClosed);
  wasteWaterTankAvailable = !digitalRead(WasteWaterTankAvailableBtn);
  wasteWaterTankFull = (analogRead(WasteWaterTankFullBtn) < 200);
  turntableSens1 = (analogRead(TurntableSens2) > 500);
  turntableSens2 = (analogRead(TurntableSens1) > 500);
}


void GetTemp() {
  for (int i = 0; i < queryNum; i++) {
    query[i] = analogRead(WaterTemperature);
    delay(10);
  }

  average = 0;
  for (int i = 0; i < queryNum; i++) {
    average += query[i];
  }
  average /= queryNum;

  average = 1023 / average - 1;
  average = seriesResistance / average;
  temp = average / ntcNominal;
  temp = log(temp);
  temp /= bCoefficient;
  temp += 1.0 / (tempNominal + 273.15);
  temp = 1.0 / temp;
  temp -= 273.15;
  waterTemp = temp;
}
