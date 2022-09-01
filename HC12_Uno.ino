#include <HttpClient.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

#include <SoftwareSerial.h>

#include <ESP8266wifi.h>
#include <ESP8266HttpClient.h>
#include <SimpleTimer.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

SoftwareSerial HC12(0, 2);

WiFiServer server(80); // 80번 포트를 이용
WiFiClient client;
HTTPClient http;
SimpleTimer timer;


const char* ssid = "MIFI_ADE2"; // wifi name
const char* password = "sscz090619"; // wifi password
// const char* ssid = "Printer"; // wifi name
// const char* password = ""; // wifi password
const char* host = "175.118.126.52"; // domain or IPAddress
const char* host2 = "15.164.138.60";

String id = "";
String temp1 = "0";
String temp2 = "0";
String temp3 = "0";
String humid = "0";
String date = "";
String dataLength = "";
String PostData; // DB에 보낼 데이터를 저장

String jsondata = "";
String sensing = "";
String jsondate = "";

void request(int mcode) {
  Serial.println("request");
  HC12.print("request" + String(mcode) + '/');
  delay(500);
  getMsg();
  dataToDB(mcode); // DB에 보낼 데이터 저장
  connectServer(); // 서버에 연결
 
  
  // This will send the request to the server
  client.println("POST /CallData.php?select=show HTTP/1.1"); // 접근할 php파일 경로
  client.println("Host: 175.118.126.52"); // 접속할 서버 도메인
  client.println("Connection: close"); // 없을시 timeout오류남
  client.println("Content-Type: application/x-www-form-urlencoded"); // 정보 보내는 방식
  client.print("Content-Length:");
  client.println(dataLength); // 보낼 데이터의 크기
  client.println();
  client.println(PostData); // 데이터 전송
  
  /*
  int timeout = millis() + 500; // 5초동안 응닶 없을시 연결 해제
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  

  // 서버로 부터 받은 데이터 출력
  
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
  */
  request2(mcode);
}

void request2(int mcode) {
  Serial.println("request2");
  connectServer2(); // 서버에 연결
  jsonEncoding(mcode); // DB에 보낼 데이터 저장

  
  // This will send the request to the server
  client.println("POST /v2/admin/roadiot/sensor/signal HTTP/1.1"); // 접근할 php파일 경로
  client.println("Host: 15.164.138.60:8401"); // 접속할 서버 도메인
  client.println("Connection: close"); // 없을시 timeout오류남
  client.println("Content-Type: application/json"); // 정보 보내는 방식
  client.println("UUID: 706E7430-F5F8-466E-AFF9-25556B57FE6D");
  client.println("token: 2d6f6cfd96519c7b3e63212b5d654b17");
  client.print("Content-Length:");
  client.println(jsondata.length()); // 보낼 데이터의 크기
  client.println();
  client.println(jsondata); // 데이터 전송
  
  /*
  int timeout = millis() + 500; // 5초동안 응닶 없을시 연결 해제
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  

  // 서버로 부터 받은 데이터 출력
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.println("respone");
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");
  */
}
void dataToDB(int mcode)
{ 
  Serial.println("dataToDB");
  PostData = "";
  PostData += "submit";
  PostData += "&&";
  PostData += "tem1=";
  PostData += String(temp1);
  PostData += "&&";
  PostData += "tem2=";
  PostData += String(temp2);
  PostData += "&&";
  PostData += "tem3=";
  PostData += String(temp3);
  PostData += "&&";
  PostData += "humid=";
  PostData += String(humid);
  PostData += "&&";
  PostData += "date=";
  PostData += String(date);
  PostData += "&&";
  PostData += "mcode=";
  PostData += String(mcode);
  dataLength = String(PostData.length());
  
  Serial.println("temp1 : "+  String(temp1));
  Serial.println("temp2 : "+ String(temp2));
  Serial.println("temp3 : "+ String(temp3));
  Serial.println("humid : "+ String(humid));
  Serial.println("time : "+  String(date));
  Serial.println("mcode : " + String(mcode));
  
}

void jsonEncoding(int mcode)
{
  Serial.println("jsonEncoding");
  jsondata = "[ {";
  jsondata += "\"id\": \"";
  jsondata += String(id);
  jsondata += "\", \"datetime\": \"";
  jsondata += String(jsondate);
  jsondata += "\", \"sensing\": { \"temperature1\": ";
  jsondata += String(temp1);
  jsondata += ", \"temperature2\": ";
  jsondata += String(temp2);
  jsondata += ", \"temperature3\": ";
  jsondata += String(temp3);
  jsondata += ", \"humidity\": ";
  jsondata += String(humid);
  jsondata += ", \"number\": ";
  jsondata += String(mcode);
  jsondata += "}}]";
  
  /*
  DynamicJsonDocument jsonBuffer(1024);
  DynamicJsonDocument sensingBuffer(1024);

  sensingBuffer["temperature"] = String(temp1);
  sensingBuffer["humidity"] = String(humid);

  serializeJson(sensingBuffer, sensing);
  
  jsonBuffer["id"] = String(id);
  jsonBuffer["datetime"] = String(jsondate);
  jsonBuffer["sensing"] = String(sensing);
  
  
  serializeJson(jsonBuffer, jsondata);
  jsondata = "[ " + jsondata + "]";
  */
  Serial.println(jsondata);
}

void connectWiFi()
{

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password); // 와이파이 연결
  while (WiFi.status() != WL_CONNECTED) // 연결이 될때까지 실행
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
  Serial.print("MAC ADDRESS : ");
  Serial.println(WiFi.macAddress());
  id = WiFi.macAddress();

  
  server.begin();
  Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());
}

void connectServer()
{
  const int httpPort = 80; // 사용할 포트번호
  client.connect(host, httpPort); // 서버에 연결
  if (!client.connected()) {
    Serial.println("not connected");
    return;
  }
}

void connectServer2()
{
  const int httpPort = 8401; // 사용할 포트번호
  if(client.connect(host2, httpPort)) {
    Serial.println("connected to server"); // 서버에 연결
  }
  if (!client.connected()) {
    Serial.println("not connected");
    return;
  }
}

void getMsg() {
 
  temp1 = "0";
  temp2 = "0";
  temp3 = "0";
  humid = "0";

  int timeout = millis() + 1000;
  while(!HC12.available()){
    int cutime = millis();
    if (timeout - cutime < 0) {
      Serial.println("Timeout !");
      break;
    }
  }
  if(HC12.available()){
    Serial.println("Test");
    String msg = HC12.readStringUntil('/');
    int data_loc[3];
    data_loc[0] = msg.indexOf(",");
    data_loc[1] = msg.indexOf(",",data_loc[0]+1);
    data_loc[2] = msg.indexOf(",",data_loc[1]+1);
    temp1 = msg.substring(0, data_loc[0]);
    temp2 = msg.substring(data_loc[0]+1, data_loc[1]);
    temp3 = msg.substring(data_loc[1]+1, data_loc[2]);
    humid = msg.substring(data_loc[2]+1);
  }
}

String getTime() {
  WiFiClient client;
  while (!client.connect("google.com", 80)) {
    Serial.println("connection failed, retrying...");
  }
  client.print("HEAD / HTTP/1.1\r\n\r\n");
  while (!client.available()) {
    yield();
  }
  while (client.available()) {
    if (client.read() == '\n') {
      if (client.read() == 'D') {
        if (client.read() == 'a') {
          if (client.read() == 't') {
            if (client.read() == 'e') {
              if (client.read() == ':') {
                client.read();
                String timeData = client.readStringUntil('\r');
                client.stop();
                return timeData;
              }
            }
          }
        }
      }
    }
  }
}

void setTime() {
  String ct = getTime();

  int mDate = ct.substring(5, 7).toInt();
  String tempMonth = ct.substring(8, 11);
  int mMonth;
  
  if (tempMonth == "DEC") {
    mMonth = 12;
  } else if (tempMonth == "JAN") {
    mMonth = 1;
  } else if (tempMonth == "FEB") {
    mMonth = 2;
  } else if (tempMonth == "MAR") {
    mMonth = 3;
  } else if (tempMonth == "APR") {
    mMonth = 4;
  } else if (tempMonth == "MAY") {
    mMonth = 5;
  } else if (tempMonth == "JUN") {
    mMonth = 6;
  } else if (tempMonth == "JUL") {
    mMonth = 7;
  } else if (tempMonth == "AUG") {
    mMonth = 8;
  } else if (tempMonth == "SEP") {
    mMonth = 9;
  } else if (tempMonth == "OCT") {
    mMonth = 10;
  } else if (tempMonth == "NOV") {
    mMonth = 11;
  }

  int mYear = ct.substring(12, 16).toInt();
  int mHour = ct.substring(17, 19).toInt() + 9; //구글 타임존과 달라서 9시간을 더해줌
  if (mHour >= 24)
  {
    mHour -= 24;
    mDate += 1;
  }
  int mMinute = ct.substring(20, 22).toInt();
  int mSecond = ct.substring(23, 25).toInt();

  String sMonth = String(mMonth);
  String sDate = String(mDate);
  String sHour = String(mHour);
  String sMinute = String(mMinute);
  String sSecond = String(mSecond);
  String sMill = "000";
  
  if(mMonth < 10) {
    sMonth = String(0) + String(mMonth);
  }
  if(mDate < 10) {
    sDate = String(0) + String(mDate);
  }
  if(mHour < 10) {
    sHour = String(0) + String(mHour);
  }
  if(mMinute < 10) {
    sMinute = String(0) + String(mMinute);
  }
  if(mSecond < 10) {
    sSecond = String(0) + String(mSecond);
  }
  
  date = "";
  date += String(mYear);
  date += "-";
  date += sMonth;
  date += "-";
  date += sDate;
  date += " ";
  date += sHour;
  date += ":";
  date += sMinute;
  date += ":";
  date += sSecond;

  jsondate = "";
  jsondate += String(mYear);
  jsondate += sMonth;
  jsondate += sDate;
  jsondate += sHour;
  jsondate += sMinute;
  jsondate += sSecond;
  jsondate += sMill;
  // setTime(mHour, mMinute, mSecond, mDate, mMonth, mYear); // 재귀로 계속 시간 업데이트
}

void updateDB() {
  setTime();
  Serial.println(second());
  if ( second()%3 == 0) {
    request(1);
    request(2);
    request(3);
    request(4);
    request(5);
    request(6);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  HC12.begin(9600);
  
  connectWiFi();
  timer.setInterval(1000, updateDB); // 주기적으로 DB업데이트
}

void loop() {
  timer.run(); // 타이머 시작
}
