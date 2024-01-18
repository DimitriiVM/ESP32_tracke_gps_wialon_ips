#include <Arduino.h>
#include <WiFi.h>

//Разработано:
//Дмитрий Витальевич Мельник
//2024 год

/*Пины для получения  данных от GPS NEO 6
rx_pin: 16
tx_pin: 17
*/
#define BAUD 9600
#define RXPIN 16

//Для работы со временем
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     0
#define UTC_OFFSET_DST 0
struct tm timeinfo;
//Нужны для времени

const char* ssid = "SG543644";
const char* password = "1234qwerty";

const uint16_t port = 20332; // Порт сервера
const char * host = "193.193.165.165"; //Айпи сервера

//char SendPack [256];
char IMEI [] = "999999999999000";//ИМЕЙ устройства 15 цифр
char StartLoginPack [] = "#L#"; //Заголовок пакета логина
char EndLoginPack [] = ";NA\r\n"; //Конец строки  для логина
char EndPack [] = "\r\n"; // Конец пакета  для всех
char TrueLogin [] = "#AL#1";//Потверждение авторизации
char HeadD [] = "#D#"; //Заголовок пакета данных
//#D#date;time;lat1;lat2;lon1;lon2;speed;course;height;sats;hdop;inputs;outputs;adc;ibutton;params\r\n
char TrueD [] = "#AD#1"; //Проверка потверждения получения пакета с данными
char NA [] = "NA";//Признак отсуствия данных или пароль!!!
char PingPack [] = "#P#\r\n";
char AnswerPingPack [] = "#AP#";
char HeadSD [] = "#SD#";
char TrueSD [] = "#ASD#1";

bool ConnectServer = false; //Проверка наличия соеденения с сервером
bool AuthServer = false;

WiFiClient client;

const int timerInterval = 2000;
uint32_t Timer = 0; //Проверка наличи файфай и авторизации на сервере
uint32_t Timer2 = 0; //Пинг до сервера для поддержания соеденения
uint32_t Timer3 = 0; //Проверка соеденений 
uint32_t Timer4 = 0; //Время с сервера
uint32_t Timer5 = 0; //Оправка пакетов на сервер
uint32_t Timer6 = 0; //Для проверки  получения данных

//Переменные для работы со временем
char timeSec[3];
char timeMin[3];
char timeHour[3];
char timeDay[3];
char timeMon[3];
char timeYear[5];
char q[2] =";";
//char pack[16];
//End

//Для  сокращенного пакета
char lat1 [10] = "00.01";
char lat2 [] = "N";
char lon1 [10] = "00.01";
char lon2 [] = "E";
char speed [3] = "NA";
char course [3] = "NA";
char height [3] = "NA";
char sats [3] = "NA";

void setup() {
  Serial.begin(115200);
  Serial1.begin(BAUD, SERIAL_8N1, RXPIN);
  Serial.println("Start ESP32!");
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); 
  int WFcount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    WFcount ++;
    if (WFcount > 30) 
    { 
      Serial.println();
      Serial.println("Ошибка подключения к Wi-Fi");
      ESP.restart();
      }
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
}

void loop() {
  if(millis() - Timer >= 1000){//Проверяем наличие соеделения WiFi и наличие соедделения сервера
    Timer = millis();
    //Serial.println("Timer");
    if(WiFi.status() == WL_CONNECTED){//Проверка наличия соденения WiFi
      Serial.println("WiFi Connected ON");
      if(!ConnectServer){
        if(!client.connect(host, port)){
          ConnectServer = false;
          Serial.println("Error conected server");
        }
        else{
          Serial.println("Sever connected OK");
          ConnectServer = true;
          SendLoginPack();
        }
      }
    }
    else{
      Serial.println("WiFi Connected OFF");
      ConnectServer = false;
      WiFi.begin(ssid, password);
    }
  }

  if(millis() - Timer2 >= 2000){//Делаем пинг каждые 2 секунды
    Timer2 = millis();
    //Serial.println("Timer2");
    if(AuthServer){
      SendPingPack();
    }
    
  }

  if(millis() - Timer3 >= 100){
    Timer3 = millis();
    //Serial.println("Timer3");
    if(WiFi.status() != WL_CONNECTED){//Если пропал файфай скидываем все флаги
      bool ConnectServer = false;
      bool AuthServer = false;
    }
    else{
      if(!client.connected()){
        bool AuthServer = false;
      }
    }
  }

  if(millis()-Timer4 >= 1000){//Запрос времени с NTP Сервера
    Timer4 = millis();
    Serial.println("Timer4");
    GetTimeNTP();
  }

  if(millis()-Timer5 >= 10000){//Отправка пакетов сокрашенных SD
    Timer5 = millis();
    Serial.println("Timer5");
    if(AuthServer){
      SendSDPack();
    }
  }

  if(millis()-Timer6 >= 100){
    Timer6 = millis();
    char simbol;
    if(Serial1.available()>0){
      simbol = Serial1.read();
      Serial.print(simbol);
    }
  }

}//-----------------------------------------------------------------------

void SendSDPack(){//Отправка сокращенного пакета
//#SD#date;time;lat1;lat2;lon1;lon2;speed;course;height;sats\r\n
  char SendSDPack[128] = "";
  strcat(SendSDPack, HeadSD);
  strcat(SendSDPack, timeDay);
  strcat(SendSDPack, timeMon);
  char vrtimeYear [3] = "";
  vrtimeYear [0] = timeYear[2];
  vrtimeYear [1] = timeYear[3];
  vrtimeYear [2] = 0x00;
  strcat(SendSDPack, vrtimeYear);
  strcat(SendSDPack, q);
  strcat(SendSDPack, timeHour);
  strcat(SendSDPack, timeMin);
  strcat(SendSDPack, timeSec);
  strcat(SendSDPack, q);
  strcat(SendSDPack, lat1);
  strcat(SendSDPack, q);
  strcat(SendSDPack, lat2);
  strcat(SendSDPack, q);
  strcat(SendSDPack, lon1);
  strcat(SendSDPack, q);
  strcat(SendSDPack, lon2);
  strcat(SendSDPack, q);
  strcat(SendSDPack, speed);
  strcat(SendSDPack, q);
  strcat(SendSDPack, course);
  strcat(SendSDPack, q);
  strcat(SendSDPack, height);
  strcat(SendSDPack, q);
  strcat(SendSDPack, sats);
  strcat(SendSDPack, EndPack);
  Serial.println(SendSDPack);
  client.print(SendSDPack);
  uint32_t timer = millis();
  char simbol;
  char AnswerPack[16] = "";
  byte i = 0;
  while(millis()-timer <= 2000){//Ждем ответ с авторизацией с таймаутом  2000
    if(client.available()){
      simbol = client.read();
      AnswerPack[i] = simbol;
      i++;
      Serial.print(simbol);
      if(simbol == '\n'){
        simbol = 0x00;
        AnswerPack[i+1] = simbol;
        break;
      }
    }
  }
  if(strncmp(AnswerPack, TrueSD, 6)==0){
    AuthServer = true;
  }
  else{
    AuthServer = false;
  }
}

void GetTimeNTP(){//Полечение времени
    if (!getLocalTime(&timeinfo)){
      Serial.println("ERROR");
      return;
    }/*
    for(byte i = 0; i < sizeof(pack); i++)
    {
      if(pack[i] == 0x00) break;
      pack[i] = 0x00;
    }*/
    strftime(timeSec, 3, "%S", &timeinfo);
    strftime(timeMin, 3, "%M", &timeinfo);
    strftime(timeHour, 3, "%H", &timeinfo);
    strftime(timeDay, 3, "%d", &timeinfo);
    strftime(timeMon, 3, "%m", &timeinfo);
    strftime(timeYear, 5, "%Y", &timeinfo);
    Serial.println(&timeinfo);
    //pack[14] = 0x00;
    //Serial.println(pack);
}

void SendPingPack(){// Отправка пингового пакета
  //#P# \r\n - Запрос
  //#AP#\r\n - Ответ
  char AnswerPack[16] = "";
  uint32_t timer = millis();
  byte i = 0;
  char simbol;
  client.print(PingPack);
  while(millis()-timer <= 2000){//Ждем ответ с авторизацией с таймаутом  2000
    if(client.available()){
      simbol = client.read();
      AnswerPack[i] = simbol;
      i++;
      Serial.print(simbol);
      if(simbol == '\n'){
        simbol = 0x00;
        AnswerPack[i+1] = simbol;
        break;
      }
    }
  } 
  if(strncmp(AnswerPack, AnswerPingPack, 4)==0){
    AuthServer = true;
  }
  else{
    AuthServer = false;
  } 

}

void SendLoginPack()
{
  char SendPack[128] = "";
  char AnswerPack[16] = "";
  byte i = 0;
  strcat(SendPack, StartLoginPack);
  strcat(SendPack, IMEI);
  strcat(SendPack, EndLoginPack);
  Serial.print(SendPack);
  client.print(SendPack);
  uint32_t timer = millis();
  char simbol;
  while(millis()-timer <= 2000){//Ждем ответ с авторизацией с таймаутом  2000
    if(client.available()){
      simbol = client.read();
      AnswerPack[i] = simbol;
      i++;
      Serial.print(simbol);
      if(simbol == '\n'){
        simbol = 0x00;
        AnswerPack[i+1] = simbol;
        break;
      }
    }
  }
  if(strncmp(AnswerPack, TrueLogin, 5)==0){
    AuthServer = true;
  }
  else{
    AuthServer = false;
  }
}