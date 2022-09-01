#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>
#define untilID 0//0~n
#define Cycle 30//0~n

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

unsigned char parsing_buf[11] = {0,};

const char* ssid = "U+Net5055";
const char* password = "95456565A#";

int toggle=0;//for indicator led
int temp = 19;

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
  #if 0
  if(post_counter++>Cycle){
    post_counter = 0;
    if (WiFi.status() == WL_CONNECTED) {    // WiFi가 연결되어 있을 경우 HTTP 접속
      HTTPClient http;  // HTTPClient 클래스의 오브섹트 선언
      //String ubidots = "http://things.ubidots.com/api/v1.6/variables/";
      String ubidots = "http://175.208.204.151:8080/sangmin";
      //ubidots += VAR_ID;              // Variable ID 추가
      //ubidots += "/values/?token=";
      //ubidots += TOKEN;               // Token 추가
      http.begin(wifiClient, ubidots);//추가 
      http.addHeader("Content-Type", "application/json");
      //int httpCode = http.POST("{\"value\":\"20\"}");  // POST 실행
      int httpCode = http.POST("{\"value\":\"thank you for your services\"}");  // POST 실행
      Serial.println("{\"value\":\"smlee1\"}");//check
      //Serial.println("{\"value\":\""+String(temp)+"\"}");//check
      
      if (httpCode > 0) { // 정상적으로 요청이 된 경우
        String payload = http.getString();   // 응답을 수신
        Serial.println(payload);            // 수신된 응답 내용을 시리얼 통신을 통해 출력
      }
      http.end(); // HTTP 접속 종료
    }
  }
  #endif
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
      // if((parsing_buf[0]==0xff) && (parsing_buf[1]==0xfe))
      // {
      //   checksum = parsing_buf[2];
      //   for(int i=4;i<11;i++){checksum += parsing_buf[i];}
      //   if(parsing_buf[3] == (checksum^0xff))
      //   {
      //     for(int i=0;i<3;i++) {Temp[i] = ((float)(parsing_buf[(i*2)+4]*256+parsing_buf[(i*2)+5])/100);}
      //     Humi = parsing_buf[10];
          
      //     test += "ID";
      //     test += String(checkID);
      //     test += ": top(";
      //     test += Temp[0];
      //     test += ") mid(";
      //     test += Temp[1];
      //     test += ") bot(";
      //     test += Temp[2];
      //     test += ") humi(";
      //     test += Humi;
      //     test +=")";

      //     Serial.println(test);      // 읽어서 HC-12 모듈로 전달합니다

      //     if(checkID > untilID){checkID=0;}
      //     request_flag = 0;
      //     if(sendID > untilID){sendID=0;}
      //   }
      // }
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
  unsigned char request_buf[4] = {0xfe,0xfe,0x00,0xff}; 

  request_buf[2] = i;
  request_buf[3] = request_buf[2]^0xff;
  
  for(int i=0;i<4;i++)
    {
      HC12.write(request_buf[i]);
      Serial.print(String(request_buf[i]));
      Serial.print(" ");
    }  
    Serial.println(" ");
    
}
