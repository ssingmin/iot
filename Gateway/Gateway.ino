#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

#define untilID 2//0~n
#define Cycle 10//0~n

//#define VAR_ID "6304e64050e064938a161e8f"
//#define TOKEN "BBFF-Ul97dOUWh6L1m7sqhpiEtyIcCSkQeM"

WiFiClient wifiClient;  //추가

SoftwareSerial HC12(D7,D6); // HC-12 TX Pin은 D2(RX용)에, HC-12 RX Pin을 D3(TX용)

int request_flag = 0;

int sendID = 0;
int checkID = 0;
float Temp[3] = {0,};
unsigned char Humi = 0;

//unsigned char checksum;
unsigned char parsing_ID;
int post_counter = 0;
int request_counter = 0;
int parsing_counter = 0;

char output[1024];

unsigned char parsing_buf[11] = {0,};

const char* ssid = "iotdemo";
const char* password = "iotcuredemo";

int toggle=0;//for indicator led

void setup () {
  pinMode(14, OUTPUT);
  HC12.begin(9600);               // HC-12 시리얼 통신 속도 설정
  Serial.begin(115200);       // PC에서 모니터링하기 위한 시리얼 통신 시작
  WiFi.begin(ssid, password); // WiFi 접속

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting..");
  }
}

void loop() {
  ////https get////

  DynamicJsonDocument doc(1024);
  
  doc["create_time"]=220905121000;
  doc["frequency"]=10;
  JsonArray ports = doc["data_list"].to<JsonArray>();
    JsonObject root = ports.createNestedObject();
    root["sensor_id"]="sensor_a";
    root["top"]=Temp[0];
    root["middle"]=Temp[1];
    root["bottom"]=Temp[2];
    root["humidity"]=(int)Humi;
  
  
  //ports.printTo(output);
  serializeJson(doc, output);
  //serializeJson(doc, Serial);
  Serial.println("");

  
  if(post_counter++>Cycle){
    post_counter = 0;
    if (WiFi.status() == WL_CONNECTED) {    // WiFi가 연결되어 있을 경우 HTTP 접속
      HTTPClient http;  // HTTPClient 클래스의 오브섹트 선언
      String server = "http://15.165.203.24:8992/insert_sensor_data";

      http.begin(wifiClient, server);//추가 
      http.addHeader("Content-Type", "application/json");
      //int httpCode = http.POST("{\"value\":\"20\"}");  // POST 실행

        int httpCode = http.POST(output);// 여기가 진짜
          if (httpCode > 0) // 정상적으로 요청이 된 경우
          { 
            String payload = http.getString();   // 응답을 수신
            Serial.println(payload);            // 수신된 응답 내용을 시리얼 통신을 통해 출력
        }
      http.end(); // HTTP 접속 종료
    }
  }
  
  ///////////////

  
  ////request////
  if(request_counter++>Cycle){
    if(request_flag == 1){sendID--;}// if gateway don't receive message, it will repeat request
    request_flag = 1;
    request_counter = 0;
    checkID=sendID;
    requestID(sendID++);
    //if(sendID > untilID){sendID=0;}
  }
  ///////////////

 
  ////parsing////
  if (HC12.available() > 0) 
  {
    parsing_buf[parsing_counter++] = HC12.read();
    if(parsing_counter>=11)
    {
      parsing_counter=0;
      request_flag=parsingMSG(parsing_buf);
    }
  }

  ///////////////
      
  
  delay(100);
  toggle^=1;
  digitalWrite(14, toggle);
}

int parsingMSG(unsigned char buf[])
{
  unsigned char checksum;
  String test = "";
  
  if((buf[0]==0xff) && (buf[1]==0xfe))
  {
    checksum = buf[2];
    for(int i=4;i<11;i++){checksum += buf[i];}
    if(buf[3] == (checksum^0xff))
    {
      for(int i=0;i<3;i++) {Temp[i] = ((float)(buf[(i*2)+4]*256+buf[(i*2)+5])/100);}
      Humi = buf[10];
      //should divide
      test += "ID";
      test += String(checkID);
      test += ": top(";
      test += Temp[0];
      test += ") mid(";
      test += Temp[1];
      test += ") bot(";
      test += Temp[2];
      test += ") humi(";
      test += Humi;
      test +=")";

      Serial.println(test);      // 읽어서 HC-12 모듈로 전달합니다
///////////////////////////////////
      if(checkID > untilID){checkID=0;}
      if(sendID > untilID){sendID=0;}
      return 0;
    }
    return 1;
  }
  return 1;
}


void requestID(unsigned char i)
{
  const int buf_size = 5;

  unsigned char request_buf[buf_size] = {0xfe,0xfe,0x00,0xff,0x00}; 

  request_buf[2] = i;
  request_buf[3] = (request_buf[2]+request_buf[4])^0xff;
  
  for(int i=0;i<buf_size;i++)
    {
      HC12.write(request_buf[i]);
      Serial.print(String(request_buf[i]));
      Serial.print(" ");
    }  
    Serial.println(" ");
    
}
