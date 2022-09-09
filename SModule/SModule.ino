#include <OneWire.h>
#include <SoftwareSerial.h>
#define BUFFER_SIZE 11
#define PARSING_SIZE 5
#define SMODULE_ID 2
#define LED_PORT 13

SoftwareSerial HC12(2, 3); // HC-12 TX Pin, HC-12 RX Pin
int resetflag = 0;         //임시

int polling_flag = 0;

///////////////////////////////humi/////////////////////////////////////////
const int AirValue = 600;   // you need to replace this value with Value_1
const int WaterValue = 350; // you need to replace this value with Value_2

int soilMoistureValue = 0;
int soilmoisturepercent = 0;

//////////////////////////////////////////////////////////////////////////////
//데이터 배열을 일률적으로 해야할듯 그래서 다른데에서 오는 데이터도 받고 버릴 수 있게 지금 버퍼가 차이가 나면서 꼬이는 문제가 발생하네 ㅅㅄㅄㅄㅂ

/////////////////////////////////////////protocol/////////////////////////////////////////
// head(0):0xff, head(1):0xfe, id(2):0x00~0a, checksum(3):0xff, checksum=!(id(2)+datasum(4~10))
// A0temp_H(4):0xff, A0temp_L(5):0xff, A1temp_H(6):0xff, A1temp_L(7):0xff, A2temp_H(8):0xff, A2temp_L(9):0xff, humi(10):0xff,
unsigned char parsing_ID;
int parsing_counter = 0;
unsigned char checksum = 0;
unsigned char rev_checksum = 0;
unsigned char RF_buf[BUFFER_SIZE] = {0xff, 0xfe, SMODULE_ID, checksum, 4, 5, 6, 7, 8, 9, 0};
unsigned char parsing_buf[PARSING_SIZE] = {
    0,
};
//////////////////////////////////////////////////////////////////////////////////////////

// int DS18S20_Pin = 2;                             //온도센서를 2번 핀으로 연결
int DS18S20_topPin = 14; // a0=14
int DS18S20_midPin = 15; // a1=15
int DS18S20_botPin = 16; // a2=16

int toggle = 0; // for indicator led

float temperature[3] = {
    0,
};

int tmp = 0;

OneWire ds[3] = {14, 15, 16}; // 14=a0, 15=a1, 16=a2

void setup()
{
    HC12.begin(9600); // Serial port to HC12
    pinMode(13, OUTPUT);
    Serial.begin(115200);
}

void loop()
{

    for (int i = 0; i < 3; i++)
    {
        temperature[i] = getTemp(ds[i]); //온도 측정 후 변수에 저장
        RF_buf[(i * 2) + 4] = (((int)(temperature[i] * 100)) >> 8);
        RF_buf[(i * 2) + 5] = ((int)(temperature[i] * 100));
        // tmp = (int)(temperature[i]*100);
        // Serial.println(tmp);
    }

    soilMoistureValue = analogRead(A3); // put Sensor insert into soil
    soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
    RF_buf[10] = soilmoisturepercent;

    /// checksum//
    checksum = SMODULE_ID;
    for (int i = 4; i < 11; i++)
    {
        checksum += RF_buf[i];
    }
    RF_buf[3] = (~checksum);
    //////////////

    ////paring////

    if (HC12.available() > 0)
    {
        parsing_buf[parsing_counter++] = HC12.read();

        if (parsing_counter >= PARSING_SIZE)
        {
            for (int i = 0; i < 5; i++)
            {
                Serial.print(String(parsing_buf[i]) + " ");
            }
            Serial.println(" ");
            parsing_counter = 0;

            if ((parsing_buf[0] == 0xfe) && (parsing_buf[1] == 0xfe))
            {
                parsing_ID = parsing_buf[2];
                rev_checksum = parsing_buf[2] + parsing_buf[4];
                if ((parsing_ID == SMODULE_ID) && (parsing_buf[3] == (rev_checksum ^ 0xff)))
                {
                    for (int i = 0; i < 11; i++)
                    {
                        Serial.print(String(RF_buf[i]) + " ");
                    }
                    Serial.println(" ");
                    for (int i = 0; i < BUFFER_SIZE; i++)
                    {
                        HC12.write(RF_buf[i]);
                    }

                    for (int i = 3; i <= BUFFER_SIZE; i++)
                    {
                        RF_buf[i] = 0;
                    } // clear RF_buf
                    for (int i = 0; i <= PARSING_SIZE; i++)
                    {
                        parsing_buf[i] = 0;
                    } // clear parsing_buf
                }
                else {
                    parsing_counter = 0;
                    for (int i=0; i<BUFFER_SIZE; i++) {
                        parsing_buf[i] = 0;
                    }
                    HC12.flush();
                }
            }
        }
    }
    /////////////
        if ((resetflag++) > 100)
        {
            asm volatile(" jmp 0");
        } //임시방법
    #if 0
    polling_flag++;
    if (polling_flag > 10)
    {
        Serial.println("waiting for your command...");
        polling_flag = 0;
        // toggle^=1;
        digitalWrite(LED_PORT, toggle); // 상태확인
        if ((resetflag++) > 10)
        {
            asm volatile(" jmp 0");
        } //임시방법
    }
    #endif
}

////////////////////////////////////////////////////////////////

float getTemp(OneWire ds)
{ //온도 측정 후 반환하는 함수
    byte data[12];
    byte addr[8];
    if (!ds.search(addr))
    {
        ds.reset_search();
        return -1000;
    }
    if (OneWire::crc8(addr, 7) != addr[7])
    {
        Serial.println("CRC is not valid!");
        return -1000;
    }
    if (addr[0] != 0x10 && addr[0] != 0x28)
    {
        Serial.print("Device is not recognized");
        return -1000;
    }
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);
    byte present = ds.reset();
    ds.select(addr);
    ds.write(0xBE);

    for (int i = 0; i < 9; i++)
    {
        data[i] = ds.read();
    }

    ds.reset_search();
    byte MSB = data[1];
    byte LSB = data[0];
    float tempRead = ((MSB << 8) | LSB);
    float TemperatureSum = tempRead / 16;

    return TemperatureSum;
}
