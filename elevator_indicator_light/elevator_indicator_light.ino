#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"  // https://github.com/adafruit/Adafruit_BMP3XX

// 當電梯在任一樓層超過30秒，執行重新校正
#define WAIT_TIME_TO_CAL 30000  //30s

Adafruit_BMP3XX bmp;

int currentFloorNo = -99;
unsigned long timerStartTime=0;

int stdFloorNo = 6;
double stdPressure;

// 樓層資訊
struct Floor {
  int number;
  float distance;
};

const int numFloors = 11;  // 總樓層數
const float B2_B1_DISTANCE = 2.6;
const float B1_1F_DISTANCE = 4.5;
const float _1F_2F_DISTANCE = 3.5;
const float _2F_3F_DISTANCE = 4.6;
const float _3F_4F_DISTANCE = 4.6;
const float _4F_5F_DISTANCE = 4.7;
const float _5F_6F_DISTANCE = 4.7;
const float _6F_7F_DISTANCE = 4.7;
const float _7F_8F_DISTANCE = 4.7;
const float _8F_9F_DISTANCE = 4.6;

Floor floors[numFloors] = {
  {-1, 0.0},
  {0, B2_B1_DISTANCE},
  {1, B2_B1_DISTANCE + B1_1F_DISTANCE},
  {2, B2_B1_DISTANCE + B1_1F_DISTANCE + _1F_2F_DISTANCE},
  {3, B2_B1_DISTANCE + B1_1F_DISTANCE + _1F_2F_DISTANCE + _2F_3F_DISTANCE},
  {4, B2_B1_DISTANCE + B1_1F_DISTANCE + _1F_2F_DISTANCE + _2F_3F_DISTANCE + _3F_4F_DISTANCE},
  {5, B2_B1_DISTANCE + B1_1F_DISTANCE + _1F_2F_DISTANCE + _2F_3F_DISTANCE + _3F_4F_DISTANCE + _4F_5F_DISTANCE},
  {6, B2_B1_DISTANCE + B1_1F_DISTANCE + _1F_2F_DISTANCE + _2F_3F_DISTANCE + _3F_4F_DISTANCE + _4F_5F_DISTANCE + _5F_6F_DISTANCE},
  {7, B2_B1_DISTANCE + B1_1F_DISTANCE + _1F_2F_DISTANCE + _2F_3F_DISTANCE + _3F_4F_DISTANCE + _4F_5F_DISTANCE + _5F_6F_DISTANCE + _6F_7F_DISTANCE},
  {8, B2_B1_DISTANCE + B1_1F_DISTANCE + _1F_2F_DISTANCE + _2F_3F_DISTANCE + _3F_4F_DISTANCE + _4F_5F_DISTANCE + _5F_6F_DISTANCE + _6F_7F_DISTANCE + _7F_8F_DISTANCE},
  {9, B2_B1_DISTANCE + B1_1F_DISTANCE + _1F_2F_DISTANCE + _2F_3F_DISTANCE + _3F_4F_DISTANCE + _4F_5F_DISTANCE + _5F_6F_DISTANCE + _6F_7F_DISTANCE + _7F_8F_DISTANCE + _8F_9F_DISTANCE}
};

void setup()   {     
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  
  Serial.begin(115200);

  // 初始設定
  if (bmp.begin_I2C()){    
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_127);
    bmp.setOutputDataRate(BMP3_ODR_50_HZ);  
    bmp.performReading();
    delay(30);
    Serial.println(F("BMP390 init success"));
  }
  else{
    // 初始化錯誤，一般是連接問題
    Serial.println(F("BMP390 init fail\n\n"));
    while(1);    // 永久停在這裡
  }

  // 取得初始壓力 (設備必須在1F重新啟動)
  stdPressure = GetPressure();
  Serial.print(F("Reset stdPressure = "));
  Serial.println(stdPressure, 2);
  Serial.print(F("Reset stdFloorNo = "));
  Serial.println(stdFloorNo);
}

void Show7Led(int _no){
  if(_no == -1){  //B2
    digitalWrite(3, LOW);
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
    digitalWrite(6, HIGH);
    digitalWrite(7, LOW);
    digitalWrite(8, LOW);
    digitalWrite(9, HIGH);
  }
  else if(_no == 0){  //B1
    digitalWrite(3, LOW);
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);
    digitalWrite(8, LOW);
    digitalWrite(9, HIGH);
  }
  else if(_no == 1){
    digitalWrite(3, LOW);
    digitalWrite(4, HIGH);
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);
    digitalWrite(8, LOW);
    digitalWrite(9, LOW);
  }
  else if(_no == 2){
    digitalWrite(3, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(5, LOW);
    digitalWrite(6, HIGH);
    digitalWrite(7, HIGH);
    digitalWrite(8, LOW);
    digitalWrite(9, HIGH);
  }
  else if(_no == 3){
    digitalWrite(3, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(5, HIGH);
    digitalWrite(6, HIGH);
    digitalWrite(7, LOW);
    digitalWrite(8, LOW);
    digitalWrite(9, HIGH);
  }
  else if(_no == 4){
    digitalWrite(3, LOW);
    digitalWrite(4, HIGH);
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
  }
  else if(_no == 5){
    digitalWrite(3, HIGH);
    digitalWrite(4, LOW);
    digitalWrite(5, HIGH);
    digitalWrite(6, HIGH);
    digitalWrite(7, LOW);
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
  }
  else if(_no == 6){
    digitalWrite(3, HIGH);
    digitalWrite(4, LOW);
    digitalWrite(5, HIGH);
    digitalWrite(6, HIGH);
    digitalWrite(7, HIGH);
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
  }
  else if(_no == 7){
    digitalWrite(3, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);
    digitalWrite(8, LOW);
    digitalWrite(9, LOW);
  }
  else if(_no == 8){
    digitalWrite(3, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(5, HIGH);
    digitalWrite(6, HIGH);
    digitalWrite(7, HIGH);
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
  }
  else if(_no == 9){
    digitalWrite(3, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(5, HIGH);
    digitalWrite(6, HIGH);
    digitalWrite(7, LOW);
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
  }
  
}

double GetPressure(){
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return 0.0;
  }
  return bmp.pressure / 100.0F;
}

int GetFloor(float distance) {
  int currentIdx = -1;

  // 尋找標準樓層的索引
  for (int i = 0; i < numFloors; i++) {
    if (floors[i].number == stdFloorNo) {
      currentIdx = i;
      break;
    }
  }

  // 計算目標樓層的索引
  int targetIdx = currentIdx;
  float remainingDistance = distance;
  // Serial.print("targetIdx: ");    Serial.println(targetIdx);
  // Serial.print("remainingDistance: ");    Serial.println(remainingDistance);
  
  if (distance > 0) {
    for (int i = currentIdx + 1; i < numFloors; i++) {
      float floorHeight = floors[i].distance - floors[i - 1].distance;
      if (remainingDistance >= floorHeight / 2) {
        remainingDistance -= floorHeight;
        targetIdx++;
        // Serial.print("targetIdx: ");    Serial.println(targetIdx);
        // Serial.print("remainingDistance: ");    Serial.println(remainingDistance);
      } else {
        break;
      }
    }
  } else {
    for (int i = currentIdx - 1; i >= 0; i--) {
      float floorHeight = floors[i + 1].distance - floors[i].distance;
      if (-remainingDistance >= floorHeight / 2) {
        remainingDistance += floorHeight;
        targetIdx--;
        // Serial.print("targetIdx: ");    Serial.println(targetIdx);
        // Serial.print("remainingDistance: ");    Serial.println(remainingDistance);
      } else {
        break;
      }
    }
  }

  if (targetIdx < 0) {
    targetIdx = 0;
  } else if (targetIdx >= numFloors) {
    targetIdx = numFloors - 1;
  }
  
  // Serial.print("adj targetIdx: ");    Serial.println(targetIdx);
  return floors[targetIdx].number;
}

void loop(){  
    // 取得 現在壓力
    double _pressure = GetPressure();
    if(_pressure<900.0 || _pressure>1100.0){
      delay(200);
      return;
    }
    
    // 取得 現在高度 與 標準樓層 的距離
    double _distance = bmp.readAltitude(stdPressure);
    if(_distance<-50.0 || _distance>50.0){
      delay(200);
      return;
    }
    
    // 換算成 現在樓層
    int _floor = GetFloor(_distance);
    if(_floor<-1 || _floor>9){
      delay(200);
      return;
    }
    
    SetFloor(_floor);  // 設定現在樓層，開始計時，檢查是否需要重新校正 
    Show7Led(_floor);  // 顯示

    Serial.print("_pressure = ");   Serial.print(_pressure);  
    Serial.print(", _distance = "); Serial.print(_distance); 
    Serial.print(", _floor = "); Serial.print(_floor); 
    Serial.println();

    delay(1);
}

void SetFloor(int _floor){
  if(_floor != currentFloorNo){
    currentFloorNo = _floor;
    TimerReStart();    
  }
  else {
    if( TimerGetEscapeTime() > WAIT_TIME_TO_CAL){
      // 執行校正
      stdFloorNo = _floor;
      stdPressure = GetPressure();
      Serial.print(F("Set stdFloorNo: ")); Serial.println(stdFloorNo);
      Serial.print(F("Set stdPressure: ")); Serial.println(stdPressure, 2);

      // 校正後重新計時
      TimerReStart();    
    }
  }  
}

void TimerReStart(){
  timerStartTime = millis();
}

unsigned long TimerGetEscapeTime(){
  unsigned long current_time = millis();
  return current_time - timerStartTime;
}
