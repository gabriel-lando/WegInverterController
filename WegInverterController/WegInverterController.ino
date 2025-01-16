#include <Wire.h>               // Biblioteca utilizada para fazer a comunicação com o I2C
#include <LiquidCrystal_I2C.h>  // Biblioteca utilizada para fazer a comunicação com o display 20x4

#include "Settings.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

const unsigned long TOTAL_TIME = RAMP_1_UP_TIME + DIRECTION_1_TIME + RAMP_1_DOWN_TIME + RAMP_2_UP_TIME + DIRECTION_2_TIME + RAMP_2_DOWN_TIME;

#define DIR_1 LOW
#define DIR_2 HIGH

enum class States {
  Idle = 0,
  Ramp1Up,
  Direction1,
  Ramp1Down,
  Ramp2Up,
  Direction2,
  Ramp2Down,
  Ended,
  Emergency
};

unsigned long StatesTime[9] = {
  0,
  RAMP_1_UP_TIME,
  DIRECTION_1_TIME,
  RAMP_1_DOWN_TIME,
  RAMP_2_UP_TIME,
  DIRECTION_2_TIME,
  RAMP_2_DOWN_TIME,
  0,
  0
};

String StatesStr[9] = {
  "",
  "R1 Sub",
  "Dir. 1",
  "R1 Desc",
  "R2 Sub",
  "Dir. 2",
  "R2 Desc",
  "",
  ""
};

bool inProcess = false;
unsigned long startTime = 0;
unsigned long startStep = 0;
unsigned long currentTime = 0;
States currentState = States::Idle;

void setup() {
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(START_PIN, OUTPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  pinMode(RAMP_PIN, OUTPUT);

  digitalWrite(ENABLE_PIN, LOW);
  digitalWrite(START_PIN, LOW);
  digitalWrite(DIRECTION_PIN, LOW);
  digitalWrite(RAMP_PIN, LOW);

  pinMode(START_BTN, INPUT_PULLUP);
  pinMode(STOP_BTN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  PrintInfo();
  delay(2500);
  ResetState();
}

void loop() {
  auto stopButtonState = GetButtonState(STOP_BTN);
  auto startButtonState = GetButtonState(START_BTN);

  if (inProcess && stopButtonState) {
    currentState = States::Emergency;
    SetInverterState();
    PrintForcedStop();
    delay(EMERGENCY_STOP_TIME * 1000);
    ResetState();
  } else if (!inProcess && startButtonState) {
    inProcess = true;
  }

  CheckState();
  delay(50);
}

void NextState() {
  currentState = static_cast<States>(((int)currentState) + 1);
}

void ResetState() {
  PrintHome();
  inProcess = false;
  startTime = 0;
  startStep = 0;
  currentTime = 0;
  currentState = States::Idle;
}

void CheckState() {
  if (!inProcess) return;

  if (currentState == States::Idle) {
    InitProcess();
    NextState();
    UpdateState();
    return;
  }

  if (currentState == States::Ended) {
    PrintEnd();
    delay(5000);
    ResetState();
    return;
  }

  if (CheckTime()) {
    NextState();
    UpdateState();
    return;
  }

  auto currTime = (millis() - startStep) / 1000;
  auto elapsedTime = (millis() - startTime) / 1000;
  UpdateDisplayStep(currTime);
  UpdateCurrentTime(elapsedTime);
}

void UpdateState() {
  startStep = millis();
  SetInverterState();

  auto elapsedTime = (millis() - startTime) / 1000;
  PrintDisplayStep(StatesStr[(int)currentState], elapsedTime);
}

void InitProcess() {
  startTime = millis();
}

bool CheckTime() {
  auto currTime = (millis() - startStep) / 1000;
  return currTime >= StatesTime[(int)currentState];
}

void PrintDisplayStep(String newStep, unsigned long currentTime) {
  if (!newStep.length()) return;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(newStep);
  lcd.print(": ");
  UpdateDisplayStep(0);

  PrintCurrentTime(currentTime);
}

void UpdateDisplayStep(unsigned long currentTime) {
  lcd.setCursor(9, 0);
  lcd.print(IntToString(currentTime) + "/" + IntToString(StatesTime[(int)currentState]));
}

void PrintCurrentTime(unsigned long currentTime) {
  lcd.setCursor(0, 1);
  lcd.print("Total: ");

  UpdateCurrentTime(currentTime);
}

void UpdateCurrentTime(unsigned long currentTime) {
  lcd.setCursor(9, 1);
  lcd.print(IntToString(currentTime) + "/" + IntToString(TOTAL_TIME));
}

void PrintInfo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Controle CFW100");
  lcd.setCursor(12, 1);
  lcd.print("v1.0");
}

void PrintHome() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Aperte VERDE");
  lcd.setCursor(2, 1);
  lcd.print("para comecar.");
}

void PrintEnd() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("FIM!");
}

void PrintForcedStop() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("PARADA DE");
  lcd.setCursor(3, 1);
  lcd.print("EMERGENCIA");
}

void SetInverterState() {
  switch (currentState) {
    case States::Ramp1Up: Start(DIR_1); break;
    case States::Direction1: break;
    case States::Ramp1Down: EndDirection(); break;
    case States::Ramp2Up: StartDirection(DIR_2); break;
    case States::Direction2: break;
    case States::Ramp2Down: EndDirection(); break;
    default: Stop(); break;
  }
}

bool GetButtonState(unsigned char button) {
  return !digitalRead(button);
}

void Start(bool direction) {
  StartDirection(direction);
  digitalWrite(ENABLE_PIN, HIGH);
}

void EndDirection() {
  digitalWrite(START_PIN, LOW);
}

void StartDirection(bool direction) {
  digitalWrite(DIRECTION_PIN, direction);
  digitalWrite(RAMP_PIN, direction);
  digitalWrite(START_PIN, HIGH);
}

void Stop() {
  digitalWrite(ENABLE_PIN, LOW);
  digitalWrite(START_PIN, LOW);
  digitalWrite(DIRECTION_PIN, LOW);
  digitalWrite(RAMP_PIN, LOW);
}

char buffer[4];
String IntToString(unsigned long value) {
  sprintf(buffer, "%3lu", value);
  return String(buffer);
}