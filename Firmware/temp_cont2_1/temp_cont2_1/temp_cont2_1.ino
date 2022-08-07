/*
  temp_cont2_1.ino
  
  This sketch was developed by Synapse(Hiroshi Tanigawa) in 2016.
  This Library is originally distributed at "Synapse's history of making gadgets." 
  <https://synapse.kyoto>

  This sketch is free software: you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation, either version 2.1 of the License, or
  (at your option) any later version.

  This sketch  is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with MGLCD.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SPI.h> // 128×64 LCDシールドとThermocouple Amplifier MAX31855 breakout boardで使用
#include <MGLCD.h> // 128×64 LCDシールドで使用
#include <MGLCD_SPI.h> // 128×64 LCDシールドで使用
#include <ResKeypad.h> // I/Oピン一つで読める4X4キーパッドで使用
#include <Adafruit_MAX31855.h> // Thermocouple Amplifier MAX31855 breakout boardで使用

// 128×64 LCDシールド関係の宣言
#define CS_PIN   10
#define DI_PIN    9
#define MAX_FREQ (7400*1000UL)
MGLCD_GH12864_20_SPI MGLCD(MGLCD_SpiPin2(CS_PIN, DI_PIN), MAX_FREQ);

// I/Oピン一つで読める4X4キーパッド関係の宣言
ResKeypad keypad(A1,16,RESKEYPAD_4X4,RESKEYPAD_4X4_SIDE_B);

// 温度測定用のThermocouple Amplifier MAX31855 breakout board関係の宣言
#define MAXDO   3
#define MAXCS   4
#define MAXCLK  5
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

// ヒーターの制御ピン
#define HEATER_PIN A0 

// 圧電ブザー用のピン
#define BUZZER_PIN 7
#define BUZZER_GND 6

#define TIME_RESOLUTION 100 // 処理時間の最小単位[ms]
#define PWM_RESOLUTION 20 // PWMの分解能

#define TEMP_COOL 60.0 // 冷却終了温度

#define RETRY_LIMIT 5 // 温度計測のリトライ回数の上限

#define GRAPH_WIDTH 122 // グラフの水平方向のピクセル数
#define GRAPH_OFFSET 3  // グラフの水平方向のオフセット
#define GRAPH_HEIGHT 36 // グラフの垂直方向のピクセル数
#define GRAPH_LOW 0.0 // グラフの下限の温度
#define GRAPH_HIGH 260.0 // グラフの上限の温度

#define PROFILE_NAME_LEN 10 // プロファイル名の長さ
#define PROFILE_LIST_LEN 8 // プロファイルの種類

#define FLASH_TICKS 40 // プロファイル名やステート数が表示されている時間のtick数
#define MAX_FLASH_TICK (FLASH_TICKS*2-1) // プロファイル名やステート数の表示用のtickの最大値(表示の更新用に使う)

#define PROFILE_DISP_NUM 4 // 1画面に表示できるプロファイルの数

struct profile { // プロファイル設定の構造体
  const char *name;     // プロファイルの名称
  uint16_t TimeCheck;   // 加熱チェックの時刻[秒]
  int16_t TempCheck;    // 加熱チェック時の温度の下限[℃]
  int16_t TempPreheat;  // 予熱温度の中央値[℃]
  int16_t WidthPreheat; // 予熱温度の幅[℃]
  uint16_t IdlePreheat; // 予熱の初期にヒーターを止める時間[秒]
  uint16_t TimePreheat; // 予熱時間[秒]
  int16_t TempReflow;   // リフロー温度の中央値[℃]
  int16_t WidthReflow;  // リフロー温度の幅[℃]
  uint16_t IdleReflow;  // リフローの初期にヒータを止める時間[秒]
  uint16_t TimeReflow;  // リフロー時間[秒]
  int16_t TempCool;     // 冷却終了温度[℃]
};

// プロファイル設定の各項目の画面上の表示用名称
PROGMEM const char NameLabel        []="ﾒｲｼｮｳ     ";
PROGMEM const char TimeCheckLabel   []="ﾁｪｯｸ ｼﾞｺｸ ";
PROGMEM const char TempCheckLabel   []="ﾁｪｯｸ ｵﾝﾄﾞ ";
PROGMEM const char TempPreheatLabel []="ﾖﾈﾂ ｵﾝﾄﾞ  ";
PROGMEM const char WidthPreheatLabel[]="ﾖﾈﾂ ﾊﾊﾞ   ";
PROGMEM const char IdlePreheatLabel []="ﾖﾈﾂ ｱｲﾄﾞﾙ ";
PROGMEM const char TimePreheatLabel []="ﾖﾈﾂ ｼﾞｶﾝ  ";
PROGMEM const char TempReflowLabel  []="ﾘﾌﾛｰ ｵﾝﾄﾞ ";
PROGMEM const char WidthReflowLabel []="ﾘﾌﾛｰ ﾊﾊﾞ  ";
PROGMEM const char IdleReflowLabel  []="ﾘﾌﾛｰ ｱｲﾄﾞﾙ";
PROGMEM const char TimeReflowLabel  []="ﾘﾌﾛｰ ｼﾞｶﾝ ";
PROGMEM const char TempCoolLabel    []="ﾚｲｷｬｸ ｵﾝﾄﾞ";

// プロファイル名
PROGMEM const char ProfileName0[]="Pbﾌﾘｰ(GN)";
PROGMEM const char ProfileName1[]="Pbﾌﾘｰ(HG)";
PROGMEM const char ProfileName2[]="ﾕｳｴﾝ(GN)";
PROGMEM const char ProfileName3[]="ﾕｳｴﾝ(HG)";
PROGMEM const char ProfileName4[]="ﾍﾞｰｸ 120ﾟC";
PROGMEM const char ProfileName5[]="ﾃｽﾄ";
PROGMEM const char ProfileName6[]="ﾌﾟﾛﾌｧｲﾙ7";
PROGMEM const char ProfileName7[]="ﾌﾟﾛﾌｧｲﾙ8";

// プロファイル
PROGMEM const profile ProfileList[PROFILE_LIST_LEN] = {
  { // 0
    ProfileName0, // name         プロファイルの名称
    120,          // TimeCheck    加熱チェックの時刻[秒]
    100,          // TempCheck    加熱チェック時の温度の下限[℃]
    150,          // TempPreheat  予熱温度の中央値[℃]
    40,           // WidthPreheat 予熱温度の幅[℃]
    16,           // IdlePreheat  予熱の初期にヒーターを止める時間[秒]
    90,           // TimePreheat  予熱時間[秒]
    220,          // TempReflow   リフロー温度の中央値[℃]
    40,           // WidthReflow  リフロー温度の幅[℃]
    9,            // IdleReflow   リフローの初期にヒータを止める時間[秒]
    35,           // TimeReflow   リフロー時間[秒]
    60,           // TempCool     冷却終了温度[℃]
  },
  { // 1
    ProfileName1, // name         プロファイルの名称
    120,          // TimeCheck    加熱チェックの時刻[秒]
    70,           // TempCheck    加熱チェック時の温度の下限[℃]
    152,          // TempPreheat  予熱温度の中央値[℃]
    30,           // WidthPreheat 予熱温度の幅[℃]
    14,           // IdlePreheat  予熱の初期にヒーターを止める時間[秒]
    60,           // TimePreheat  予熱時間[秒]
    222,          // TempReflow   リフロー温度の中央値[℃]
    30,           // WidthReflow  リフロー温度の幅[℃]
    4,            // IdleReflow   リフローの初期にヒータを止める時間[秒]
    32,           // TimeReflow   リフロー時間[秒]
    60,           // TempCool     冷却終了温度[℃]
  },
  { // 2
    ProfileName2, // name         プロファイルの名称
    120,          // TimeCheck    加熱チェックの時刻[秒]
    100,          // TempCheck    加熱チェック時の温度の下限[℃]
    150,          // TempPreheat  予熱温度の中央値[℃]
    40,           // WidthPreheat 予熱温度の幅[℃]
    12,           // IdlePreheat  予熱の初期にヒーターを止める時間[秒]
    90,           // TimePreheat  予熱時間[秒]
    200,          // TempReflow   リフロー温度の中央値[℃]
    40,           // WidthReflow  リフロー温度の幅[℃]
    9,            // IdleReflow   リフローの初期にヒータを止める時間[秒]
    35,           // TimeReflow   リフロー時間[秒]
    60,           // TempCool     冷却終了温度[℃]
  },
  { // 3
    ProfileName3, // name         プロファイルの名称
    120,          // TimeCheck    加熱チェックの時刻[秒]
    70,           // TempCheck    加熱チェック時の温度の下限[℃]
    152,          // TempPreheat  予熱温度の中央値[℃]
    30,           // WidthPreheat 予熱温度の幅[℃]
    14,           // IdlePreheat  予熱の初期にヒーターを止める時間[秒]
    60,           // TimePreheat  予熱時間[秒]
    202,          // TempReflow   リフロー温度の中央値[℃]
    30,           // WidthReflow  リフロー温度の幅[℃]
    4,            // IdleReflow   リフローの初期にヒータを止める時間[秒]
    32,           // TimeReflow   リフロー時間[秒]
    60,           // TempCool     冷却終了温度[℃]
  },
  { // 4
    ProfileName4, // name         プロファイルの名称
    100,          // TimeCheck    加熱チェックの時刻[秒]
    70,           // TempCheck    加熱チェック時の温度の下限[℃]
    103,          // TempPreheat  予熱温度の中央値[℃]
    45,           // WidthPreheat 予熱温度の幅[℃]
    50,           // IdlePreheat  予熱の初期にヒーターを止める時間[秒]
    36000,        // TimePreheat  予熱時間[秒]
    0,            // TempReflow   リフロー温度の中央値[℃]
    40,           // WidthReflow  リフロー温度の幅[℃]
    15,           // IdleReflow   リフローの初期にヒータを止める時間[秒]
    35,           // TimeReflow   リフロー時間[秒]
    60,           // TempCool     冷却終了温度[℃]
  },
  { // 5
    ProfileName5, // name         プロファイルの名称
    120,          // TimeCheck    加熱チェックの時刻[秒]
    49,           // TempCheck    加熱チェック時の温度の下限[℃]
    70,           // TempPreheat  予熱温度の中央値[℃]
    40,           // WidthPreheat 予熱温度の幅[℃]
    40,           // IdlePreheat  予熱の初期にヒーターを止める時間[秒]
    100,          // TimePreheat  予熱時間[秒]
    130,          // TempReflow   リフロー温度の中央値[℃]
    40,           // WidthReflow  リフロー温度の幅[℃]
    15,           // IdleReflow   リフローの初期にヒータを止める時間[秒]
    35,           // TimeReflow   リフロー時間[秒]
    45,           // TempCool     冷却終了温度[℃]
  },
  { // 6
    ProfileName6, // name         プロファイルの名称
    120,          // TimeCheck    加熱チェックの時刻[秒]
    49,           // TempCheck    加熱チェック時の温度の下限[℃]
    70,           // TempPreheat  予熱温度の中央値[℃]
    40,           // WidthPreheat 予熱温度の幅[℃]
    40,           // IdlePreheat  予熱の初期にヒーターを止める時間[秒]
    100,          // TimePreheat  予熱時間[秒]
    130,          // TempReflow   リフロー温度の中央値[℃]
    40,           // WidthReflow  リフロー温度の幅[℃]
    15,           // IdleReflow   リフローの初期にヒータを止める時間[秒]
    35,           // TimeReflow   リフロー時間[秒]
    45,           // TempCool     冷却終了温度[℃]
  },
  { // 7
    ProfileName7, // name         プロファイルの名称
    120,          // TimeCheck    加熱チェックの時刻[秒]
    49,           // TempCheck    加熱チェック時の温度の下限[℃]
    70,           // TempPreheat  予熱温度の中央値[℃]
    40,           // WidthPreheat 予熱温度の幅[℃]
    40,           // IdlePreheat  予熱の初期にヒーターを止める時間[秒]
    100,          // TimePreheat  予熱時間[秒]
    130,          // TempReflow   リフロー温度の中央値[℃]
    40,           // WidthReflow  リフロー温度の幅[℃]
    15,           // IdleReflow   リフローの初期にヒータを止める時間[秒]
    35,           // TimeReflow   リフロー時間[秒]
    45,           // TempCool     冷却終了温度[℃]
  },
}; // ProfileList

uint32_t StartTime1; // 加熱スタート時の時刻[ms]
uint32_t StartTime2; // 現在のステートの始まった時刻[ms]
uint8_t GraphYHist[GRAPH_WIDTH]; // グラフのy座標の過去の値

profile cp; // 現在のプロファイル
uint8_t SelectedProfile=0; // 選択したプロファイルの番号

// プログラムメモリ上の文字列の表示
void PrintPgmStr(const char *p)
{
  while(char c=pgm_read_byte_near(p++)) {
    MGLCD.print(c);
  } // while
} // PrintPgmStr

// 温度を表示する
void PrintTemperature(float temperature)
{
  const int lcd_x=0;
  const int lcd_y=0;
  const int width=8;

  MGLCD.Locate(lcd_x,lcd_y);
  if(!isnan(temperature)) {
    MGLCD.print(temperature);
    MGLCD.print(F("ﾟC"));
  } else {
    MGLCD.print(F("ｾﾝｻｴﾗｰ"));
  } // if
  while(MGLCD.GetX()<lcd_x+width) {
    MGLCD.print(' ');
  } // while
} // PrintTemperature

// 状態を表示する
void PrintStatus(const __FlashStringHelper *status)
{
  const int width=12;
  MGLCD.Locate(0,1);
  MGLCD.print(status);
  while(MGLCD.GetX()<width) {
    MGLCD.print(' ');
  } // while
} // Print Status

void PrintProfileName(const char *ProfileName)
{
  const int width=12;
  MGLCD.Locate(0,1);
  PrintPgmStr(ProfileName);
  while(MGLCD.GetX()<width) {
    MGLCD.print(' ');
  } // while  
} // PrintProfileName

// 経過時間を表示する
void PrintTime()
{
  unsigned long ElapseTime; // 経過時間
  int hh,mm,ss; // 時分秒
  char str[15]; // 時刻の文字列
  const char *format="%02d:%02d:%02d"; // 時刻表示のフォーマット

  ElapseTime=(millis()-StartTime1)/1000; 
  ss=int(ElapseTime%60);
  mm=int((ElapseTime/60)%60);
  hh=int(ElapseTime/3600);
  sprintf(str,format,hh,mm,ss);
  MGLCD.Locate(13,0);
  MGLCD.print(str);

  ElapseTime=(millis()-StartTime2)/1000; 
  ss=int(ElapseTime%60);
  mm=int((ElapseTime/60)%60);
  hh=int(ElapseTime/3600);
  sprintf(str,format,hh,mm,ss);
  MGLCD.Locate(13,1);
  MGLCD.print(str);
} // PrintTime

void PrintMessage(const __FlashStringHelper *msg)
{
  MGLCD.Locate(0,2);
  MGLCD.print(msg);
  if(MGLCD.GetY()==2) {
    MGLCD.ClearRestOfLine();
  } // if  
} // PrintMessage

// PWMのデューティー比を計算する
uint8_t CalculateDuty(float CurrentTemp, float TergetTemp, float TargetWidth)
{
  if(CurrentTemp<TergetTemp-TargetWidth/2.0) {
    return PWM_RESOLUTION;
  } else if(CurrentTemp>TergetTemp+TargetWidth/2.0) {
    return 0;
  } else {
    return uint8_t(((TergetTemp-CurrentTemp)/TargetWidth+0.5)*PWM_RESOLUTION);
  } // if
} // CalculateDuty

uint8_t TempToY(float t)
{
  if(t<GRAPH_LOW) {
    t=GRAPH_LOW;
  } else if(t>GRAPH_HIGH) {
    t=GRAPH_HIGH;
  } // if
  return MGLCD.GetHeight()-GRAPH_OFFSET-1-(t-GRAPH_LOW)/(GRAPH_HIGH-GRAPH_LOW)*(GRAPH_HEIGHT-1); 
} // TempToY

void DrawGraphFrame()
{
  for(int t=0; t<=250; t+=50) {
    int y=TempToY(t);
    MGLCD.Line(0,y,GRAPH_OFFSET-1,y);
    MGLCD.Line(MGLCD.GetWidth()-GRAPH_OFFSET,y,MGLCD.GetWidth()-1,y);
  } // for t
  for(int x=GRAPH_OFFSET+GRAPH_WIDTH-1; x>=GRAPH_OFFSET; x-=12) {
    MGLCD.Line(x,MGLCD.GetHeight()-1,x,MGLCD.GetHeight()-GRAPH_OFFSET);
  } // for x
  MGLCD.Rect(0,MGLCD.GetHeight()-GRAPH_OFFSET-GRAPH_HEIGHT-1,MGLCD.GetWidth()-1,MGLCD.GetHeight()-1);
} // DrawGraphFrame

void LoadProfile(uint8_t number)
{
  cp.name        =pgm_read_word_near(&ProfileList[number].name        );
  cp.TimeCheck   =pgm_read_word_near(&ProfileList[number].TimeCheck   );
  cp.TempCheck   =pgm_read_word_near(&ProfileList[number].TempCheck   );
  cp.TempPreheat =pgm_read_word_near(&ProfileList[number].TempPreheat );
  cp.WidthPreheat=pgm_read_word_near(&ProfileList[number].WidthPreheat);
  cp.IdlePreheat =pgm_read_word_near(&ProfileList[number].IdlePreheat );
  cp.TimePreheat =pgm_read_word_near(&ProfileList[number].TimePreheat );
  cp.TempReflow  =pgm_read_word_near(&ProfileList[number].TempReflow  );
  cp.WidthReflow =pgm_read_word_near(&ProfileList[number].WidthReflow );
  cp.IdleReflow  =pgm_read_word_near(&ProfileList[number].IdleReflow  );
  cp.TimeReflow  =pgm_read_word_near(&ProfileList[number].TimeReflow  );
  cp.TempCool    =pgm_read_word_near(&ProfileList[number].TempCool    );
} // LoadProfile

void DispProfileItems(uint8_t sel, uint8_t top)
{
  for(uint8_t i=top; i<top+4; i++) {
    MGLCD.Locate(0,i-top+2);
    switch(i) {
      case 0:
        PrintPgmStr(NameLabel);
        MGLCD.print(':');
        PrintPgmStr(pgm_read_word_near(&ProfileList[sel].name));
        break;
      case 1:
        PrintPgmStr(TimeCheckLabel);
        MGLCD.print(':');
        MGLCD.print((uint16_t)pgm_read_word_near(&ProfileList[sel].TimeCheck));
        break;
      case 2:
        PrintPgmStr(TempCheckLabel);
        MGLCD.print(':');
        MGLCD.print((int16_t)pgm_read_word_near(&ProfileList[sel].TempCheck));
        break;
      case 3:
        PrintPgmStr(TempPreheatLabel);
        MGLCD.print(':');
        MGLCD.print((int16_t)pgm_read_word_near(&ProfileList[sel].TempPreheat));
        break;
      case 4:
        PrintPgmStr(WidthPreheatLabel);
        MGLCD.print(':');
        MGLCD.print((int16_t)pgm_read_word_near(&ProfileList[sel].WidthPreheat));
        break;
      case 5:
        PrintPgmStr(IdlePreheatLabel);
        MGLCD.print(':');
        MGLCD.print((uint16_t)pgm_read_word_near(&ProfileList[sel].IdlePreheat));
        break;
      case 6:
        PrintPgmStr(TimePreheatLabel);
        MGLCD.print(':');
        MGLCD.print((uint16_t)pgm_read_word_near(&ProfileList[sel].TimePreheat));
        break;
      case 7:
        PrintPgmStr(TempReflowLabel);
        MGLCD.print(':');
        MGLCD.print((int16_t)pgm_read_word_near(&ProfileList[sel].TempReflow));
        break;
      case 8:
        PrintPgmStr(WidthReflowLabel);
        MGLCD.print(':');
        MGLCD.print((int16_t)pgm_read_word_near(&ProfileList[sel].WidthReflow));
        break;
      case 9:
        PrintPgmStr(IdleReflowLabel);
        MGLCD.print(':');
        MGLCD.print((uint16_t)pgm_read_word_near(&ProfileList[sel].IdleReflow));
        break;
      case 10:
        PrintPgmStr(TimeReflowLabel);
        MGLCD.print(':');
        MGLCD.print((uint16_t)pgm_read_word_near(&ProfileList[sel].TimeReflow));
        break;
      case 11:
        PrintPgmStr(TempCoolLabel);
        MGLCD.print(':');
        MGLCD.print((int16_t)pgm_read_word_near(&ProfileList[sel].TempCool));
        break;
    } // switch
    MGLCD.ClearRestOfLine();
  } // for i
} // DispProfileItems

void ViewProfile(uint8_t sel)
{
  const int items=12; // 表示項目数
  const int lines=4;  // 1画面に表示できる項目数
  uint8_t top=0;
  char c;

  MGLCD.ClearScreen();
  MGLCD.print("ﾌﾟﾛﾌｧｲﾙ ｴﾂﾗﾝ");
  MGLCD.Locate(0,7);
  MGLCD.print("#:ｷｬﾝｾﾙ");
  do {
    DispProfileItems(sel,top);
    c=keypad.WaitForChar();
    switch(c) {
      case 'A':
        if(top>0) {
          top--;
          tone(BUZZER_PIN,2000,30);
        } // if
        break;
      case 'D':
        if(top+lines<items) {
          top++;
          tone(BUZZER_PIN,2000,30);
        } // if
        break;
      case '#':
        tone(BUZZER_PIN,1000,30);
        break;
    } // switch
  } while(c!='#');
} // ViewProfile

void DispProfileList(uint8_t top, uint8_t sel)
{
  uint8_t i=top; 
  while(i<top+PROFILE_DISP_NUM) {
    MGLCD.Locate(0,i-top+2);
    if(i<PROFILE_LIST_LEN) {
      if(i==sel) {
        MGLCD.SetInvertMode(MGLCD_INVERT);
      } // if
      if(i<9){
        MGLCD.print(' ');
      } // if
      MGLCD.print(i+1);
      MGLCD.print(':');
      PrintPgmStr(pgm_read_word_near(&ProfileList[i].name));
    } // if
    MGLCD.ClearRestOfLine();
    MGLCD.SetInvertMode(MGLCD_NON_INVERT);
    i++;
  } // while
}  // DispProfileList

void SelectProfileInitScreen()
{
  MGLCD.ClearScreen();
  MGLCD.println(F("ﾌﾟﾛﾌｧｲﾙ ﾉ ｾﾝﾀｸ"));
  MGLCD.SetTextWindow(0,6,20,7);
  MGLCD.SetScrollMode(MGLCD_NON_SCROLL);
  MGLCD.Locate(0,1);
  MGLCD.println(F("*:ｶｸﾃｲ #:ｷｬﾝｾﾙ C:ｴﾂﾗﾝ"));
  MGLCD.SetTextWindow(0,0,20,7);
  MGLCD.SetScrollMode(MGLCD_SCROLL);
} // SelectProfileInitScreen

void SelectProfile()
{
  uint8_t top; // 表示するプロファイルの先頭番号
  uint8_t sel=SelectedProfile; // 選択中のプロファイル番号

  SelectProfileInitScreen();

  top=SelectedProfile;
  if(top+PROFILE_DISP_NUM-1>=PROFILE_LIST_LEN) {
    top=PROFILE_LIST_LEN-PROFILE_DISP_NUM<0 ? 0 : PROFILE_LIST_LEN-PROFILE_DISP_NUM;
  } // if

  char c; // 入力したキー
  do {
    DispProfileList(top,sel);
    c=keypad.WaitForChar();
    switch(c) {
      case 'A': // 上
        if(sel>0) {
          sel--;
          if(top>sel) {
            top=sel;
          } // if
          tone(BUZZER_PIN,2000,30);
        } // if
        break;
      case 'D': // 下
        if(sel<PROFILE_LIST_LEN-1) {
          sel++;
          if(top<sel-PROFILE_DISP_NUM+1) {
            top=sel-PROFILE_DISP_NUM+1;
          } // if
          tone(BUZZER_PIN,2000,30);
        } // if
        break;
      case 'C': // モード
        tone(BUZZER_PIN,2000,30);
        ViewProfile(sel);
        SelectProfileInitScreen();
        break;
      case '*': // OK
        SelectedProfile=sel;
        LoadProfile(sel);
        tone(BUZZER_PIN,2000,100);
        break;
      case '#': // CAN
        tone(BUZZER_PIN,1000,400);
        break;
    } // switch
  } while(c!='*' && c!='#');
} // SelectProfile

// 温度を3回測定して中央値を返す
float ReadTemp()
{
  int cnt=0;
  int RetryCnt=0;
  float TempArray[3];

  while(cnt<3) {
    float f=thermocouple.readCelsius();
    if(isnan(f)) {
       // 規定回数以上センサエラーを検出するとNANを返す
       if(++RetryCnt>RETRY_LIMIT) {
         return NAN;
       } // if
    } else {
      TempArray[cnt++]=f;
      for(int i=cnt-1; i>0; i--) {
        if(TempArray[i-1]>TempArray[i]) {
          float temp;
          temp=TempArray[i-1];
          TempArray[i-1]=TempArray[i];
          TempArray[i]=temp;
        } // if
      } // for i
    } // if
  } // while
  return TempArray[1];   
} // ReadTemp

void setup()
{
  // リレー制御用ピンの初期化
  pinMode(A0,OUTPUT);
  digitalWrite(A0,LOW);

  // 他励式圧電ブザー用ピンの初期化
  pinMode(BUZZER_GND,OUTPUT);
  digitalWrite(BUZZER_GND,LOW);
  pinMode(BUZZER_PIN,OUTPUT);
  digitalWrite(BUZZER_PIN,LOW);

  Serial.begin(9600); // シリアルを9600bpsで初期化

  // LCDの初期化
  MGLCD.Reset();
  if(strlen("ｱ")!=1) MGLCD.SetCodeMode(MGLCD_CODE_UTF8); // 半角カナの表示の有効化

  // グラフの枠の描画
  DrawGraphFrame();

  // GraphYHistの初期化
  for(int i=0; i<GRAPH_WIDTH; i++) {
    GraphYHist[i]=MGLCD.GetHeight()-GRAPH_OFFSET;
  } // for i

  // 制御パラメータの初期化
  LoadProfile(SelectedProfile);

  // 起動音
  tone(BUZZER_PIN,1000,100);
  delay(120);
  tone(BUZZER_PIN,1500,100); 
} // setup

void loop()
{
  enum HeaterState { ST_INITIAL,ST_WAIT,ST_HEAT1,ST_PREHEAT1,ST_PREHEAT2,ST_HEAT2,ST_REFLOW1,ST_REFLOW2,ST_COOL,ST_ERROR,ST_BREAK }; // ステートマシンの状態
  static HeaterState state=ST_WAIT;
  static HeaterState OldState=ST_INITIAL;
  static uint8_t duty=0; // デューティー比
  static float temperature; // 測定点の温度
  static uint8_t PwmCnt=0; // PWM用カウンタ
  static uint8_t GraphCnt=0; // グラフ更新用カウンタ
  static uint32_t NextTick=millis(); // 次のループの時刻 
  static uint8_t FlashTick=0; // プロファイル名とステータスの表示切替用tickカウンタ
  static const __FlashStringHelper *status; // 現在のステータスを表す文字列へのポインタ

  char KeypadChar='\0';
  // キーを読みながら次のTickを待つ
  do {
    if(!KeypadChar) {
      KeypadChar=keypad.GetChar();
    } // if
  } while(int32_t(millis()-NextTick)<0);
  NextTick+=TIME_RESOLUTION;

  // プロファイルの選択
  if(state==ST_WAIT && KeypadChar=='C') {
    KeypadChar='\0';
    tone(BUZZER_PIN,2000,30);
    SelectProfile();
    MGLCD.ClearScreen();
    DrawGraphFrame();
    OldState=ST_INITIAL;
    NextTick=millis();
  } // if
  
  // ヒーター制御
  digitalWrite(HEATER_PIN,PwmCnt<duty ? HIGH : LOW);

  // 温度の測定
  if(PwmCnt==0) {
    temperature=ReadTemp();
    PrintTemperature(temperature);

    // センサエラーを検出すると強制中断
    if((state==ST_HEAT1 || state==ST_PREHEAT1 || state==ST_PREHEAT2 || state==ST_HEAT2 || state==ST_REFLOW1 || state==ST_REFLOW2) && isnan(temperature)) {
      state=ST_BREAK;
    } // if
  } // if
  
  // 時刻の表示
  if(state!=ST_WAIT) PrintTime();

  // グラフの更新  
  if(PwmCnt==1 && ++GraphCnt>=5) {
    GraphCnt=0;
    for(int i=1; i<GRAPH_WIDTH; i++) {
      GraphYHist[i-1]=GraphYHist[i];
    } // for i
    if(isnan(temperature)) {
      GraphYHist[GRAPH_WIDTH-1]=MGLCD.GetHeight()-GRAPH_OFFSET;
    } else {
      GraphYHist[GRAPH_WIDTH-1]=TempToY(temperature);
    } // if
    for(uint8_t i=0; i<GRAPH_WIDTH; i++) {
      uint8_t x=GRAPH_OFFSET+i;
      if(GraphYHist[i]<MGLCD.GetHeight()-GRAPH_OFFSET) {
        MGLCD.Line(x,MGLCD.GetHeight()-GRAPH_OFFSET-1,x,GraphYHist[i],1);
      } // if
      if(GraphYHist[i]>MGLCD.GetHeight()-GRAPH_OFFSET-GRAPH_HEIGHT) {
        MGLCD.Line(x,GraphYHist[i]-1,x,MGLCD.GetHeight()-GRAPH_OFFSET-GRAPH_HEIGHT,0);
      } // if
    } // for i
  } // if
  
  // Serialに時刻と温度を送信
  if(state!=ST_WAIT && PwmCnt==2 && !isnan(temperature)) {
    Serial.print(millis()-StartTime1);
    Serial.print(',');
    Serial.println(temperature);
  } // if

  // 強制中断処理  
  if((state==ST_HEAT1 || state==ST_PREHEAT1 || state==ST_PREHEAT2 || state==ST_HEAT2 || state==ST_REFLOW1 || state==ST_REFLOW2) && (KeypadChar=='D')) {
    state=ST_BREAK;
  } // if

  // ステートの遷移
  switch(state) {
    case ST_WAIT:
      // 初期設定 
      if(OldState!=state) {
        if(OldState!=ST_INITIAL) {
          tone(BUZZER_PIN,1000,2000);
        } // if
        OldState=state;
        status=F("ﾀｲｷ ﾁｭｳ");
        FlashTick=MAX_FLASH_TICK;
        PrintMessage(F("D:ｽﾀｰﾄ C:ﾌﾟﾛﾌｧｲﾙ ｾﾝﾀｸ"));
      } // if

      // ステート遷移
      if(!isnan(temperature) && KeypadChar=='D') { // 温度が測定できていてかつＤキーが押された
        state=ST_HEAT1;
      } // if
      break;
      
    case ST_HEAT1:
      // 初期設定 
      if(OldState!=state) {
        OldState=state;
        tone(BUZZER_PIN,2000,100);
        StartTime1=StartTime2=millis();
        status=F("ｶﾈﾂ ｽﾃｰｼﾞ1");
        FlashTick=MAX_FLASH_TICK;
        PrintMessage(F("Dｷｰ ｦ ｵｽﾄ ｷｮｳｾｲﾁｭｳﾀﾞﾝ"));
        duty=PWM_RESOLUTION;
      } // if

      // ステート遷移
      if(PwmCnt==0 && !isnan(temperature) && temperature<cp.TempCheck && millis()-StartTime2>=cp.TimeCheck*1000L) { // 加熱不良を検出した
        state=ST_ERROR;
      } else if(PwmCnt==0 && !isnan(temperature) && temperature>=cp.TempPreheat-cp.WidthPreheat/2.0) { // 予熱温度の下限に達した
        state=ST_PREHEAT1;
      } // if
      break;  

    case ST_PREHEAT1:
      // 初期設定 
      if(OldState!=state) {
        OldState=state;
        StartTime2=millis();
        status=F("ﾖﾈﾂ ｽﾃｰｼﾞ");
        FlashTick=MAX_FLASH_TICK;
        duty=0;
      } // if

      // ステート遷移
      if(millis()-StartTime2>=cp.IdlePreheat*1000L) {
        state=ST_PREHEAT2;
      } // if
      break;

    case ST_PREHEAT2:
      // 初期設定 
      if(OldState!=state) {
        OldState=state;
      } // if

      // ステート遷移
      if(millis()-StartTime2>=cp.TimePreheat*1000L) {
        if(cp.TempReflow>cp.TempPreheat) {
          state=ST_HEAT2;
        } else{
          state=ST_COOL;
        } // if
      } // if
      break;

    case ST_HEAT2:
      // 初期設定 
      if(OldState!=state) {
        OldState=state;
        StartTime2=millis();
        status=F("ｶﾈﾂ ｽﾃｰｼﾞ2");       
        FlashTick=MAX_FLASH_TICK; 
        duty=PWM_RESOLUTION;
      } // if

      // ステート遷移
      if(PwmCnt==0 && !isnan(temperature) && temperature>=cp.TempReflow-cp.WidthReflow/2.0) { // リフロー温度の下限に達した
        state=ST_REFLOW1;
      } // if
      break;

    case ST_REFLOW1:
      // 初期設定 
      if(OldState!=state) {
        OldState=state;
        StartTime2=millis();
        status=F("ﾘﾌﾛｰ ｽﾃｰｼﾞ");
        FlashTick=MAX_FLASH_TICK;
        duty=0;
      } // if

      // ステート遷移
      if(millis()-StartTime2>=cp.IdleReflow*1000L) {
        state=ST_REFLOW2;
      } // if
      break;
      
    case ST_REFLOW2:
      // 初期設定 
      if(OldState!=state) {
        OldState=state;
      } // if

      // ステート遷移
      if(millis()-StartTime2>=cp.TimeReflow*1000L) {
        state=ST_COOL;
      } // if
      break;
      
    case ST_COOL:
    case ST_ERROR:
    case ST_BREAK:
      // 初期設定 
      if(OldState!=state) {
        OldState=state;
        StartTime2=millis();
        switch(state) {
          case ST_COOL:
            tone(BUZZER_PIN,1000,2000);
            status=F("ﾚｲｷｬｸ ｽﾃｰｼﾞ");
            break;
          case ST_ERROR:
            tone(BUZZER_PIN,1000,2000);
            status=F("ｶﾈﾂﾌﾘｮｳ ｴﾗｰ");
            break;
          case ST_BREAK:
            tone(BUZZER_PIN,1000,100);
            status=F("ｷｮｳｾｲ ﾁｭｳﾀﾞﾝ");
            break;
        } // switch
        FlashTick=MAX_FLASH_TICK;
        PrintMessage(F("ﾚｲｷｬｸﾁｭｳﾊ ｿｳｻﾃﾞｷﾏｾﾝ｡"));
        duty=0;
      } // if

      if(PwmCnt==0 && !isnan(temperature) && temperature<=cp.TempCool) { // 冷却終了温度に達した
        state=ST_WAIT;
      } // if
      break;
  } // switch(state)

  // 予熱時のデューティ比の計算
  if(PwmCnt==0 && state==ST_PREHEAT2) duty=CalculateDuty(temperature,cp.TempPreheat,cp.WidthPreheat);
  
  // リフロー時のデューティ比の計算
  if(PwmCnt==0 && state==ST_REFLOW2) duty=CalculateDuty(temperature,cp.TempReflow,cp.WidthReflow);
  
  // PWM用カウンタを進める
  if(++PwmCnt>=PWM_RESOLUTION) PwmCnt=0;

  // プロファイル名やステータスの表示
  if(FlashTick==MAX_FLASH_TICK) {
    PrintStatus(status);
    FlashTick=0;
  } else {
    if(FlashTick==FLASH_TICKS) {
      PrintProfileName(cp.name);
    } // if
    FlashTick++;
  } // if
}
