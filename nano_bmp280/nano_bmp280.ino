#include <SPI.h>
#include <Wire.h>
#include <SFE_BMP180.h>

//#define DEBUG
//#define ENABLE_OLED

#ifdef ENABLE_OLED
  #include <Adafruit_GFX.h>
  #include <Adafruit_SH1106.h>
  #define OLED_RESET 4
  Adafruit_SH1106 display(OLED_RESET);
#endif

// 當電梯在任一樓層超過30秒，執行重新校正
#define WAIT_TIME_TO_CAL 30000  //30s

SFE_BMP180 bmp180;

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
  
  Serial.begin(9600);

#ifdef DEBUG
  Serial.println(F("==========DEBUG MODE=========="));
#endif

#ifdef ENABLE_OLED
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();
#endif

  // 初始設定
  if (bmp180.begin())
    Serial.println(F("BMP180 init success"));
  else
  {
    // 初始化錯誤，一般是連接問題
    Serial.println(F("BMP180 init fail\n\n"));
#ifdef ENABLE_OLED
    display.println(F("BMP180 init fail"));
    display.display();    
#endif
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
    const int TIMES = 20;  
    double _total = 0;
    int cnt = 0;
    while(cnt < TIMES){
        double _p = GetPressureSingle();   
        if(_p>900 && _p<1100){
            _total += _p;
            cnt++;
        }
        delay(3);
    }
//    for(int i=0; i<TIMES; i++){   
//        _total += GetPressureSingle();        
//        delay(5);
//    }
    double outputP = _total / (double)TIMES ;
    Serial.print(F("Get Avg pressure: "));
    Serial.println(outputP);   
    return outputP;  
}

double GetPressureSingle(){
  char _bmpStatus;
  double _P=0, _T=0;
  _bmpStatus = bmp180.startTemperature();
  if (_bmpStatus != 0){
    delay(_bmpStatus);
    _bmpStatus = bmp180.getTemperature(_T);
    if (_bmpStatus != 0){      
      // 參數設定從 0 到 3 (最高的解析度，等待較久)
      _bmpStatus = bmp180.startPressure(3);
      if (_bmpStatus != 0){
        delay(_bmpStatus);
        _bmpStatus = bmp180.getPressure(_P, _T);
        if (_bmpStatus != 0){
          //Serial.print(F("absolute pressure: "));
          //Serial.println(_P);     
        }        
        else Serial.println(F("error retrieving pressure measurement\n"));
      }
      else Serial.println(F("error starting pressure measurement\n"));
    }
    else Serial.println(F("error retrieving temperature measurement\n"));
  }
  else Serial.println(F("error starting temperature measurement\n"));

  return _P;
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
  //Serial.println();
  //double _distance = -999;
  
#ifdef DEBUG
    if (Serial.available() > 0) {
        double _distance = Serial.parseFloat(); 
        if(_distance != 0){         
            // 換算成 現在樓層
            int _floor = GetFloor(_distance);
              
            // 設定現在樓層，開始計時，檢查是否需要重新校正
            SetFloor(_floor);
          
            // 顯示                     
            ShowOLED(_floor, 0, _distance);           
            delay(200);  
        }
    }
      
#else 
    // 取得 現在壓力
    double _pressure = GetPressure();
    if(_pressure<900 || _pressure>1100){
      delay(200);
      return;
    }
    
    // 取得 現在高度 與 標準樓層 的距離
    double _distance = GetDistance(_pressure); 
    if(_distance<-50 || _distance>50){
      delay(200);
      return;
    }
    
    // 換算成 現在樓層
    int _floor = GetFloor(_distance);
    if(_floor<-1 || _floor>9){
      delay(200);
      return;
    }
      
    // 設定現在樓層，開始計時，檢查是否需要重新校正
    SetFloor(_floor);
  
    // 顯示
    Show7Led(_floor);
    ShowOLED(_floor, _pressure, _distance);
    //ShowOLED(_floor, 0, _distance);
      
    delay(200); 
#endif  
}

void ShowOLED(int _floor, double _pressure, double _distance){
#ifdef ENABLE_OLED 
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);

    display.print("STD Floor: ");  
    if(stdFloorNo > 0){    
      display.print(stdFloorNo); 
      display.print(" F");  
    }
    else{
      if(stdFloorNo == 0) { display.print("B1"); }
      if(stdFloorNo == -1) { display.print("B2"); }
    }
    
    display.println();    
//    display.print("STD Presr: ");  
//    display.print(stdPressure);  
//    display.println();    

    display.println();    
    
    display.print("Now Floor: ");  
    if(_floor > 0){    
      display.print(_floor); 
      display.print(" F");  
    }
    else{
      if(_floor == 0) { display.print("B1"); }
      if(_floor == -1) { display.print("B2"); }
    }  
    display.println(); 
    
//    display.print("Now Presr: ");  
//    display.print(_pressure);  
//    display.println();    
    
    display.print("Distance:  ");  
    display.print(_distance);  
    display.print(" m"); 
    display.println();    
      
    display.display();  
#endif
}

double GetDistance(double _pressure){
  double _distance = bmp180.altitude(_pressure, stdPressure);
  Serial.print(F("distance: "));
  Serial.print(_distance, 2);
  Serial.println(F(" meters"));  
  return _distance;
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
