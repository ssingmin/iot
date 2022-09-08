#include <SimpleTimer.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

#define untilID 1//1~n
#define Cycle 10000//0~n

unsigned char tmp_humi;
float tmp_temp[3];
int update_flag = 0;

typedef struct _Smodule {
  char stringout[1024];
  unsigned char Smid;
  unsigned char Humi;
  float Temp[3];
} Smodule;

SimpleTimer timer1;
SimpleTimer timer2;

WiFiClient wifiClient;  //추가

SoftwareSerial HC12(D7, D6); // HC-12 TX Pin은 D2(RX용)에, HC-12 RX Pin을 D3(TX용)
int Jsontostring_flag = 0;
int receiveData_flag = 0;



int request_flag = 0;

int sendID = 0;
int checkID = 0;


//unsigned char checksum;
unsigned char parsing_ID;
int post_counter = 0;
int request_counter = 0;
int parsing_counter = 0;

unsigned char parsing_buf[11] = {0,};

const char* ssid = "iotdemo";
const char* password = "iotcuredemo";

int toggle = 0; //for indicator led


void updateDB(void) {//interval 3s

//  if (WiFi.status() == WL_CONNECTED) {    // WiFi가 연결되어 있을 경우 HTTP 접속
//    HTTPClient http;  // HTTPClient 클래스의 오브섹트 선언
//    String server = "http://15.165.203.24:8992/insert_sensor_data";
//
//    http.begin(wifiClient, server);//추가
//    http.addHeader("Content-Type", "application/json");
//    
//    int httpCode = http.POST(stringout);// 여기가 진짜
//
//    if (httpCode > 0) // 정상적으로 요청이 된 경우
//    {
//      String payload = http.getString();   // 응답을 수신
//      Serial.println(payload);            // 수신된 응답 내용을 시리얼 통신을 통해 출력
//    }
//
//    http.end(); // HTTP 접속 종료
//  }
}

void request_data()//interval 1s untilID까지 보냈으면 더이상 보내지 말기 
{
  if (request_flag == 1) {
    sendID--; // if gateway don't receive message, it will repeat request
  }
  request_flag = 1;
  request_counter = 0;
  checkID = sendID;
  requestID(sendID++);
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
  timer1.setInterval(3000, updateDB); // 주기적으로 DB업데이트
  timer2.setInterval(1000, request_data); // 주기적으로 DB업데이트
}

  String test = "";

  for(int i=1;i<=untilID;i++){
    Smodule SM[i] = {"",i, 0,{0.0, 0.0, 0.0}}; //untilID 까지 자동 선언 
  }
void loop() {

  //timer1.run(); //updateDB
  timer2.run(); //request_data
  receiveData_flag = receiveData();

  if(receiveData_flag>0){
    SM[receiveData_flag].Smid = receiveData_flag;
    SM[receiveData_flag].Humi = tmp_humi;
    for(int i=0;i<3;i++){SM[receiveData_flag].Temp[i] = tmp_temp[i];}
    //if(receiveData_flag){update_flag = 1;}
  }
  for(int i=0;i<3;i++){tmp_temp[i] = 0;} //init
  tmp_humi = 0;//init

    if (Jsontostring(SM)) {
      Jsontostring_flag++;
      test += "ID";
      test += String(checkID);
      test += ": top(";
      test += tmp_temp[0];
      test += ") mid(";
      test += tmp_temp[1];
      test += ") bot(";
      test += tmp_temp[2];
      test += ") humi(";
      test += tmp_humi;
      test += ")";

      Serial.println(test);      // 읽어서 HC-12 모듈로 전달합니다
    }

  if (post_counter++ > Cycle) {
    post_counter = 0;
    toggle ^= 1;
    digitalWrite(14, toggle);
  }
}

///////////////////////////////////loop end////////////////////////////////
int receiveData()
{
  ////parsing////
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
      if (checkID > untilID) {
        checkID = 0;
      }
      if (sendID > untilID) {
        sendID = 0;
      }
      return buf[2];//return id
    }
    return 0;
  }
  return 0;
}


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

//void Jsontostring(char *s1, float *Temp, unsigned char Humi)
int Jsontostring(Smodule module)
{
  DynamicJsonDocument doc(1024);
  //char output[1024];
  doc["create_time"] = 220905121000;
  doc["frequency"] = 10;
  JsonArray ports = doc["data_list"].to<JsonArray>();
  JsonObject root = ports.createNestedObject();
  root["sensor_id"] = module.Smid;
  root["top"] = module.Temp[0];
  root["middle"] = module.Temp[1];
  root["bottom"] = module.Temp[2];
  root["humidity"] = module.Humi;

  serializeJson(doc, module.stringout, 1024);
  //  serializeJson(doc, Serial);
  Serial.println("");
  return 1;
}
