#include <SimpleTimer.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

#define untilID 6//1~n
#define Cycle 10000//0~n
#define UPDATECYCLE 30000//1당 1ms초
#define REQUESTCYCLE 300//1당 1ms초
#define MISSINGCYCLE 100//1당 1ms초

#define MISSINGDELAY 30//1당 1ms초

unsigned int pre_chg_updatecycle = 30;
unsigned int chg_updatecycle = 30;

char SMstr[untilID][1024];

unsigned char tmp_humi;
float tmp_temp[3];
int update_flag = 0;

typedef struct _Smodule {
  char stringout[1024];
  unsigned char Smid;
  unsigned char Humi;
  float Temp[3];
} Smodule;

Smodule SM[6] = {
{"",1, 0,{0.0, 0.0, 0.0}},
{"",2, 0,{0.0, 0.0, 0.0}}, 
{"",3, 0,{0.0, 0.0, 0.0}}, 
{"",4, 0,{0.0, 0.0, 0.0}}, 
{"",5, 0,{0.0, 0.0, 0.0}}, 
{"",6, 0,{0.0, 0.0, 0.0}},  
};
SimpleTimer timer1;
SimpleTimer timer2;
SimpleTimer timer3;

WiFiClient wifiClient;  //추가

SoftwareSerial HC12(D7, D6); // HC-12 TX Pin은 D2(RX용)에, HC-12 RX Pin을 D3(TX용)

int updatedone_flag = 0;
int Jsontostring_flag = 0;

int numcounter = 0;
int timerID = 0;
int missingdata = 0;
int nextid = 0;
int Pre_receiveData_flag = 255;
int receiveData_flag = 0;
int receiveData_buf[untilID+1] ={0,};
//int request_flag = 257;

int sendID = 1;
int checkID = 0;

unsigned char parsing_ID;
int post_counter = 0;
int request_counter = 0;
int parsing_counter = 0;

unsigned char parsing_buf[11] = {0,};

// const char* ssid = "iotdemo";
// const char* password = "iotcuredemo";

const char* ssid = "U+Net5055";
const char* password = "95456565A#";

int toggle = 0; //for indicator led


void updateDB(void) {//interval 3s
  for(int i=0;i<untilID;i++){
   if (WiFi.status() == WL_CONNECTED) {    // WiFi가 연결되어 있을 경우 HTTP 접속
     HTTPClient http;  // HTTPClient 클래스의 오브섹트 선언
     String server = "http://3.39.237.15:8992/insert_sensor_data";
  
     http.begin(wifiClient, server);//추가
     http.addHeader("Content-Type", "application/json");
  
     
      String str1(SMstr[i]);
      Serial.println(str1);
  
      int httpCode = http.POST(SMstr[i]);
  
      // String str1(SM[i].stringout);
      // Serial.println(str1);
      // int httpCode = http.POST(SM[i].stringout);

      if (httpCode > 0) // 정상적으로 요청이 된 경우
      {
        String payload = http.getString();   // 응답을 수신
        Serial.println(payload);            // 수신된 응답 내용을 시리얼 통신을 통해 출력

        for(int i=0;i<4;i++)//scan digit
        {
          if(payload[i+24]>=0x30 && payload[i+24]<=0x39){
            numcounter = i;
          }
        }
        if(payload[8] == '2'){
        switch (numcounter)
          {
          case 0://1
            chg_updatecycle = (payload[24]-0x30);
            break;

          case 1://10
            chg_updatecycle = (payload[25]-0x30);
            chg_updatecycle += (payload[24]-0x30)*10;
            break;

          case 2://100
            chg_updatecycle = (payload[26]-0x30);
            chg_updatecycle = (payload[25]-0x30)*10;
            chg_updatecycle = (payload[24]-0x30)*100;
            break;

          case 3:///1000
            chg_updatecycle = (payload[27]-0x30);
            chg_updatecycle = (payload[26]-0x30)*10;
            chg_updatecycle = (payload[25]-0x30)*100;
            chg_updatecycle = (payload[24]-0x30)*1000;
            break;
          }
        Serial.println(chg_updatecycle);
        }
        

        
      }
     delay(100);     
     for(int i=0;i<untilID+1;i++){receiveData_buf[i]=0;}//receive data buf clear
     
     http.end(); // HTTP 접속 종료
   }
 }
}

void request_data()//interval 1s untilID까지 보냈으면 더이상 보내지 말기 
{
  for(int i=1;i<=untilID;i++){
    if(receiveData_buf[i] == 0){
      requestID(i);
      nextid = i;
      break;
      }
  }
}

void missingcheck()
{
  missingdata++;
}

void setup () {
  pinMode(14, OUTPUT);
  HC12.begin(9600);               // HC-12 시리얼 통신 속도 설정
  Serial.begin(115200);        // PC에서 모니터링하기 위한 시리얼 통신 시작
  WiFi.begin(ssid, password); // WiFi 접속
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting..");
  }
  timerID = timer1.setInterval(UPDATECYCLE, updateDB); 
  timer2.setInterval(REQUESTCYCLE, request_data);
  timer2.setInterval(MISSINGCYCLE, missingcheck);
}

void loop() {
  delay(1);
  
  if(pre_chg_updatecycle != chg_updatecycle)
  {
    Serial.print("chg_updatecycle pre_chg_updatecycle: ");
    Serial.print(chg_updatecycle);
    Serial.print("  ");
    Serial.println(chg_updatecycle);
    pre_chg_updatecycle = chg_updatecycle;
    timer1.deleteTimer(timerID);
    timerID = timer1.setInterval(chg_updatecycle*1000, updateDB); 
  }

  for(int i=0;i<3;i++){tmp_temp[i] = 0;} //init
  tmp_humi = 0;//init
  
  timer1.run(); //updateDB
  timer2.run(); //request_data
  timer3.run(); //request_data
  
  if(Pre_receiveData_flag == receiveData_flag)
  {
    //missingdata++;
    if(missingdata >MISSINGDELAY){
      //if(nextid == 7){nextid = 0;}
      receiveData_buf[nextid] = 1;

      Serial.print("nextid: "); 
      Serial.println(nextid); 

      missingdata = 0;
    }
  }
  else {missingdata = 0; }

  // Pre_receiveData_flag = receiveData_flag;

  receiveData_flag = receiveData();
  receiveData_buf[receiveData_flag] = 1;

  Pre_receiveData_flag = receiveData_flag;
  
  if(receiveData_flag>0){
  
    SM[receiveData_flag].Smid = receiveData_flag;
    SM[receiveData_flag].Humi = tmp_humi;
    for(int i=0;i<3;i++){SM[receiveData_flag].Temp[i] = tmp_temp[i];}

     if (Jsontostring(SM[receiveData_flag])) {
      String test = "";
     //Jsontostring_flag++;
     test += "ID";
     test += String(SM[receiveData_flag].Smid);
     test += ": top(";
     test += SM[receiveData_flag].Temp[0];
     test += ") mid(";
     test += SM[receiveData_flag].Temp[1];
     test += ") bot("; 
     test += SM[receiveData_flag].Temp[2];
     test += ") humi(";
     test += SM[receiveData_flag].Humi;
     test += ")";

     Serial.println(test);      // 읽어서 HC-12 모듈로 전달합니다
     //Serial.println(SM[receiveData_flag].stringout);      // 읽어서 HC-12 모듈로 전달합니다
   }
  }


  if (post_counter++ > Cycle) {
    post_counter = 0;
    toggle ^= 1;
    digitalWrite(14, toggle);
    delay(10);
  }
}

///////////////////////////////////loop end////////////////////////////////


void requestID(unsigned char i)
{
  const int buf_size = 5;

  unsigned char request_buf[buf_size] = {0xfe, 0xfe, 0x00, 0xff, 0x00};

  request_buf[2] = i;
  request_buf[3] = (request_buf[2] + request_buf[4]) ^ 0xff;

  for (int i = 0; i < buf_size; i++)
  {
    HC12.write(request_buf[i]);
    Serial.print(String(request_buf[i]));
    Serial.print(" ");
  }
  Serial.println(" ");
}


int receiveData()
{
  ////parsing////
int request_flag;

  if (HC12.available() > 0)
  {
    parsing_buf[parsing_counter++] = HC12.read();
    if (parsing_counter >= 11)
    {
      parsing_counter = 0;
      request_flag = parsingMSG(parsing_buf);
      for (int i = 0; i < 11; i++) {
        parsing_buf[i] = 0; //clear buf
      }
    }
    return request_flag;
  }
  return 0;
  
  ///////////////
}

int parsingMSG(unsigned char buf[])
{
  unsigned char checksum;

  if ((buf[0] == 0xff) && (buf[1] == 0xfe))
  {
    checksum = buf[2];
    for (int i = 4; i < 11; i++) {
      checksum += buf[i];
    }
    if (buf[3] == (checksum ^ 0xff))
    {
      for (int i = 0; i < 3; i++) {
        tmp_temp[i] = ((float)(buf[(i * 2) + 4] * 256 + buf[(i * 2) + 5]) / 100);
      }
      tmp_humi = buf[10];
      ///////////////////////////////////
      return buf[2];//return id
    }

    return 0;
  }
  return 0;
}

int Jsontostring(Smodule module)
{
  DynamicJsonDocument doc(1024);
  //char output[1024];
  doc["create_time"] = 220905121000;
  doc["frequency"] = 10;
  JsonArray ports = doc["data_list"].to<JsonArray>();
  JsonObject root = ports.createNestedObject();
  //root["sensor_id"] = module.Smid;
  switch(module.Smid){
  case 1 :
    root["sensor_id"] = "sensor_a";break;
  case 2 :
    root["sensor_id"] = "sensor_b";break;
  case 3 :
    root["sensor_id"] = "sensor_c";break;
  case 4 :
    root["sensor_id"] = "sensor_d";break;
  case 5 :
    root["sensor_id"] = "sensor_e";break;
  case 6 :
    root["sensor_id"] = "sensor_f";break;
 }
  root["top"] = module.Temp[0];
  root["middle"] = module.Temp[1];
  root["bottom"] = module.Temp[2];
  root["humidity"] = module.Humi;
  serializeJson(doc, SMstr[module.Smid-1], 1024);
  return 1;
}
