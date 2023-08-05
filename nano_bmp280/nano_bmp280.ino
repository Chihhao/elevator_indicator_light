#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <SFE_BMP180.h>

//#define DEBUG

#define OLED_RESET 4

// 當電梯在任一樓層超過30秒，執行重新校正
#define WAIT_TIME_TO_CAL 30000  //30s

#define BASE_HIGH   3.4
#define FLOOR_HIGH  4.7

Adafruit_SH1106 display(OLED_RESET);
SFE_BMP180 bmp180;

int floorNo = -99;
unsigned long timerStartTime=0;

int stdFloorNo = 1;
double stdPressure = 1000;

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

  display.begin(SH1106_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();

  // 初始設定
  if (bmp180.begin())
    Serial.println(F("BMP180 init success"));
  else
  {
    // 初始化錯誤，一般是連接問題
    Serial.println(F("BMP180 init fail\n\n"));
    display.println(F("BMP180 init fail"));
    display.display();    
    while(1);    // 永久停在這裡
  }

  // 取得初始壓力 (設備必須在1F重新啟動)
  stdPressure = GetPressure();
  stdFloorNo = 1;
  Serial.print(F("Reset stdPressure = "));
  Serial.println(stdPressure, 2);
  Serial.println(F("Reset stdFloorNo = 1"));
 
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
  const int TIMES = 6;
  double _P = 0;
  double _total = 0;
  for(int i=0; i<TIMES; i++){
    _total += GetPressureSingle();
  }
  _P = _total / (double)TIMES ;
  Serial.print(F("Get Avg pressure: "));
  Serial.println(_P);   
  return _P;  
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

int GetFloor(double _distance){
	// input: 目標高度 與 標準樓層 的距離
	// output: 目標樓層
	
	// 因為地下室樓高只有3.4m(估)，與樓高4.7m不同，所以這裡有一點複雜
	
	int _floor = -2;
	double distance_1F = 0;
	double distance_B1 = 0;
	double distance_B2 = 0;		

  {  // 計算 distance_1F, distance_B1, distance_B2
  	if(stdFloorNo >= 1){ // 1F (含)以上		
  		distance_1F = 0 - FLOOR_HIGH * (stdFloorNo - 1);
  		distance_B1 = distance_1F - BASE_HIGH;	
  		distance_B2 = distance_1F - BASE_HIGH * 2;	
  	}
  	else if(stdFloorNo == 0){  // B1
  		distance_1F = BASE_HIGH;
  		distance_B1 = 0;
  		distance_B2 = -BASE_HIGH;		
  	}
  	else if(stdFloorNo == -1){ // B2 		
  		distance_1F = BASE_HIGH * 2;	
  		distance_B1 = BASE_HIGH;
  		distance_B2 = 0;			
  	}
  	else{ // Error		
  		Serial.println(F("Get Floor Error"));
  		return -2;
  	}	
  }

	if(_distance >= distance_1F ){  // 目標樓高 >= 1F	
    if(stdFloorNo >= 1){ // 標準 樓高 >= 1F
      _floor = stdFloorNo + round(_distance/FLOOR_HIGH);    
    }
    else if(stdFloorNo == 0) {  // 標準 樓高 = B1
      double _d = _distance - BASE_HIGH;      
      _floor = 1 + round(_d/FLOOR_HIGH);
    } 
    else if(stdFloorNo == -1) {  // 標準 樓高 = B2
      double _d = _distance - BASE_HIGH*2;      
      _floor = 1 + round(_d/FLOOR_HIGH);
    }
	}
	else { // 目標 是地下室			
		if(stdFloorNo >= 1){ // 標準 樓高 >= 1F	
			// 扣掉目標到 1F 的距離
			double _d = _distance + FLOOR_HIGH * (stdFloorNo-1);			
			_floor = 1 + round(_d/BASE_HIGH);		
		}   
		else {  // 標準 是地下室			
			_floor = stdFloorNo + round(_distance/BASE_HIGH);				
		}
	}
	
	Serial.print(F("Get Floor: "));
  Serial.println(_floor);   
	return _floor;
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
    //ShowOLED(_floor, _pressure, _distance);
    ShowOLED(_floor, 0, _distance);
      
    delay(200);  
 

#endif
  
  
  
  
}

void ShowOLED(int _floor, double _pressure, double _distance){

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
    display.print("STD Presr: ");  
    display.print(stdPressure);  
    display.println();    

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
    
    display.print("Now Presr: ");  
    display.print(_pressure);  
    display.println();    
    
    display.print("Distance:  ");  
    display.print(_distance);  
    display.print(" m"); 
    display.println();    
      
    display.display();  
}

double GetDistance(double _pressure){
  double _distance = bmp180.altitude(_pressure, stdPressure);
  Serial.print(F("distance: "));
  Serial.print(_distance, 2);
  Serial.println(F(" meters"));  
  return _distance;
}

void SetFloor(int _floor){
  if(_floor != floorNo){
    floorNo = _floor;
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
