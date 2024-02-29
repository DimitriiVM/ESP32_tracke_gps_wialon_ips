//Разработано:
//Дмитрий Витальевич Мельник
//2024 год
#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h> //Для работы со встроенной памятью

/*Пины для получения  данных от GPS NEO 6
rx_pin: 16
tx_pin: 17
*/
#define BAUD 9600 //Скорость работы  порта с GPS
#define RXPIN 16 //Пин для получения данных от GPS модуля

//Для работы со временем
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     0
#define UTC_OFFSET_DST 0
struct tm timeinfo;
//Нужны для времени

//Для  работы  спамятью
//bool bleFlag = true;
char command[64];
char ssid[32];
char pass[32];
char port[6];
char host[32];
char imei[16];
char errmsg[] = "Invalid command format";
uint8_t flagConfig;
//int q = 0; //Эта переменная  для  флага наличия ячейки работы с массивом
//конец переменных для патями

//const char* ssid = "SG543644"; //Для вайфай
//const char* password = "1234qwerty"; //Для вайфай

uint16_t intport = 20332; // Порт сервера
//const char * host = "193.193.165.165"; //Айпи сервера

//char SendPack [256];
//char IMEI [] = "999999999999000";//ИМЕЙ устройства 15 цифр
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

char truegps [] = "$GPGGA";

bool ConnectServer = false; //Проверка наличия соеденения с сервером
bool AuthServer = false; //Проверяем есть  ли у нас авторизация на сервере

WiFiClient client;

const int timerInterval = 2000;
uint32_t Timer = 0; //Проверка наличи файфай и авторизации на сервере
uint32_t Timer2 = 0; //Пинг до сервера для поддержания соеденения
uint32_t Timer3 = 0; //Проверка соеденений 
uint32_t Timer4 = 0; //Время с сервера
uint32_t Timer5 = 0; //Оправка пакетов на сервер
uint32_t Timer6 = 0; //Для проверки  получения данных от GPS
uint32_t Timer7 = 0; //Для проверки получения  команд с сервера
uint32_t Timer8 = 0; //Для получения  команд по USB

//Переменные для работы со временем
char timeSec[3];
char timeMin[3];
char timeHour[3];
char timeDay[3];
char timeMon[3];
char timeYear[5];
char kav[2] = ";";
//char pack[16];
//End

//Для  сокращенного пакета
char lat1 [13] = "00.01";
char lat2 [2] = "N";
char lon1 [13] = "00.01";
char lon2 [2] = "E";
char speed [7] = "0";
char course [7] = "0";
char height [7] = "0";
char sats [7] = "0";

void setup() {
  Serial.begin(115200);
  Serial1.begin(BAUD, SERIAL_8N1, RXPIN);
  EEPROM.begin(512);
  Serial.println("Start ESP32!");
  startConfig();
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass); 
  int WFcount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    WFcount ++;
    if (WFcount > 30) 
    { 
      Serial.println();
      Serial.println("Ошибка подключения к Wi-Fi");
      //ESP.restart();
      break;
    }
  }
  if(WiFi.status() == WL_CONNECTED){
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
  }
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
}

void loop() {
  if(millis() - Timer >= 1000){//Проверяем наличие соеделения WiFi и наличие соедделения сервера
    Timer = millis();
    //Serial.println("Timer");
    if(WiFi.status() == WL_CONNECTED){//Проверка наличия соденения WiFi
      Serial.println("WiFi Connected ON");
      if(!client.connected()){
        if(!client.connect(host, intport)){
          ConnectServer = false;
          Serial.println("Error conected server");
          WiFi.begin(ssid, pass);
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
      WiFi.begin(ssid, pass);
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
      ConnectServer = false;
      AuthServer = false;
    }
    else{
      if(!client.connected()){
        AuthServer = false;
        ConnectServer = false;
      }
    }
  }

  if(millis()-Timer4 >= 1000){//Запрос времени с NTP Сервера
    Timer4 = millis();
    //Serial.println("Timer4");
    GetTimeNTP();
  }

  if(millis()-Timer5 >= 10000){//Отправка пакетов сокрашенных SD
    Timer5 = millis();
    //Serial.println("Timer5");
    if(AuthServer){
      SendSDPack();
    }
  }

  if(millis()-Timer6 >= 1000){//За прием GPS
    Timer6 = millis();
    UpDateGps();
  }

  if(millis() - Timer7 >= 1000){//Проверяем есть ли команды от сервера
    Timer7 = millis();
    if(AuthServer){//Проверяем есть ли авторизация
      AnswerPackServer();//Обрабатываем данные если они есть
    }
  }//END

  if(millis() - Timer8 >= 500){//Мы ищем команды и текст по Serial
    Timer8 = millis();
      if (Serial.available() > 0) {  
         Serial.readBytesUntil('\n', command, 32);
          delay(20);
            if(strncmp(command, "setimei#", 8)==0){
              Serial.println("Получина команда setimei");
              setimei();
            }
            else if(strncmp(command, "setssid#", 8)==0){
              Serial.println("Получина команда setssid");
              setssid();
            }
            else if(strncmp(command, "setpass#", 8)==0){
              Serial.println("Получина команда setpass");
              setpass();
            }
            else if(strncmp(command, "setport#", 8)==0){
              Serial.println("Получина команда setport");
              setport();            
            }  
            else if(strncmp(command, "sethost#", 8)==0){
              Serial.println("Получина команда sethost");
              sethost();
            } 
            else if(strncmp(command, "getssid#", 8)==0){
              Serial.println("Получина команда getssid");
              Serial.println(ssid);
            }  
            else if(strncmp(command, "getpass#", 8)==0){
              Serial.println("Получина команда getpass");
              Serial.println(pass);
            }   
            else if(strncmp(command, "getport#", 8)==0){
              Serial.println("Получина команда getport");
              Serial.println(port);
            } 
            else if(strncmp(command, "gethost#", 8)==0){
              Serial.println("Получина команда gethost");
              Serial.println(host);
            } 
            else if(strncmp(command, "getimei#", 8)==0){
              Serial.println("Получина команда getimei");
              Serial.println(imei);
            }   
            else if(strncmp(command, "getall#", 7)==0){
              Serial.println(ssid);
              Serial.println(pass);
              Serial.println(port);
              Serial.println(host);
              Serial.println(imei);
            }
            else if(strncmp(command, "settest#", 8)==0){
            Serial.println("Принято");             
            EEPROM.write(0,command[8]);
            EEPROM.commit();
            }
            else if(strncmp(command, "reset#", 6)==0) {
            Serial.println("RESET OK!");
            ESP.restart();           
            }                                                                                                   
           else Serial.println(errmsg);
      }  
  }//END

}//-----------------------------------------------------------------------

//$GPGGA,045104.000,3014.1985,N,09749.2873,W,1,09,1.2,211.6,M,-22.5,M,,0000*62\r\n
void UpDateGps(){//Функция разбора пакетов от GPS
    //Serial.println("GPS START");
    uint32_t timer = millis();
    const int SizwGPS = 100;
    char gps[SizwGPS]; 
    char simbol;
    while(millis()-timer <= 500){//Только 100 мс ловим  пакеты от GPS
      int i = 0;
      while (Serial1.available() > 0){
        simbol = Serial1.read();
        if(simbol == '\n'){
          simbol = 0x00;
          gps[i+1] = simbol;
          break;
        }
        gps[i] = simbol;
        i++;
      }
      if(i > 1){
        //Serial.println(gps);
  //START
  if(strncmp(gps, "$GPGGA", 6) == 0){
    byte q = 0;//Используем переменную для подсчета количества запитых.
    byte i = 1;
    byte LenGpsPack = 0;//Тут будем хранить длину строки
    //Serial.println("Есть совпадение");
    //strlen(str) возрашает длину строки без учета конца строки
    LenGpsPack = strlen(gps);
    //Serial.println(LenGpsPack);
    while(q < LenGpsPack)
    {
      if(gps[q] == ',' && i == 1){
        i++;
        q++;
      }
      if(gps[q] == ',' && i == 2){
        i++;
        q++;
        byte w = 0;
        while(gps[q] != ','){
          lat1[w] = gps[q];
          w++;
          q++;
        }
      }
      if(gps[q] == ',' && i == 3){
        i++;
        q++;
        byte w = 0;
        while(gps[q] != ','){
          lat2[w] = gps[q];
          w++;
          q++;
        }
      }
      if(gps[q] == ',' && i == 4){
        i++;
        q++;
        byte w = 0;
        while(gps[q] != ','){
          lon1[w] = gps[q];
          w++;
          q++;
        }
      }
      if(gps[q] == ',' && i == 5){
        i++;
        q++;
        byte w = 0;
        while(gps[q] != ','){
          lon2[w] = gps[q];
          w++;
          q++;
        }
      }
      if(gps[q] == ',' && i == 6){
        i++;
        q++;
      }
      if(gps[q] == ',' && i == 7){
        i++;
        q++;
        byte w = 0;
        while(gps[q] != ','){
          sats[w] = gps[q];
          w++;
          q++;
        }
      }
      if(gps[q] == ',' && i == 8){
        i++;
        q++;
      }
      if(gps[q] == ',' && i == 9){
        i++;
        q++;
        byte w = 0;
        while(gps[q] != ','){
          height[w] = gps[q];
          w++;
          q++;
        }
      }
      q++;
    }
  }
  //END
        for(int i = 0; i < SizwGPS; i++){//Почистим массив от  беды по дальше
          gps[i] = 0x00;
        }
      }
    }

}//END

void AnswerPackServer(){
  byte i = 0;
  uint32_t timer = millis();
  char simbol;
  char AnswerPackServer[64] = "";//Данные от сервера
  while(millis()-timer <= 100){//Ждем ответ с авторизацией с таймаутом  100
    if(client.available()){// если что есть от сервера
      simbol = client.read();
      if(simbol == '\n'){
        simbol = 0x00;
        AnswerPackServer[i+1] = simbol;
        break;
      }
      AnswerPackServer[i] = simbol;
      i++;
    }//Считываем данные в виде ответа и складываем все в массив
  }
   if(i > 1){
    client.flush();
    //Serial.print("Принятая команда:");
    //Serial.println(AnswerPackServer);
    //Чтобы было  видно на сервере, что мы получили команду, полученные данные отправляем назад
    //Как сообщения для  водителя #M#msg \r\n
    char SendPackM[64] = "#M#";
    strcat(SendPackM, AnswerPackServer);
    strcat(SendPackM, EndPack);
    Serial.println(SendPackM);
    client.print(SendPackM);
    //Вот тут  он может послать на сервер  пустой ответ так как время еще есть, но там не команда, а  ответ на команду.
  timer = millis();//Обнуляем пакет ожидания
  i = 0; //Обнуляем адрес байта на начало
  while(millis()-timer <= 2000){//Cлучаем ответ получили или нет пакет ответа
    if(client.available() > 0){
      simbol = client.read();
      if(simbol == '\n'){
        simbol = 0x00;
        AnswerPackServer[i+1] = simbol;
        break;
      }
      AnswerPackServer[i] = simbol;
      i++;
    }
  }
  if( i > 1){
    client.flush();
  }  
  Serial.println(AnswerPackServer);//Посылаем ответ в монитор порта
  }
}//END

void SendSDPack(){//Отправка сокращенного пакета
//#SD#date;time;lat1;lat2;lon1;lon2;speed;course;height;sats\r\n
  char SendSDPack[128] = "";//Сюда мы собираем данные в массив для отправки
  strcat(SendSDPack, HeadSD);
  strcat(SendSDPack, timeDay);
  strcat(SendSDPack, timeMon);
  char vrtimeYear [3] = "";
  vrtimeYear [0] = timeYear[2];
  vrtimeYear [1] = timeYear[3];
  vrtimeYear [2] = 0x00;
  strcat(SendSDPack, vrtimeYear);
  strcat(SendSDPack, kav);
  strcat(SendSDPack, timeHour);
  strcat(SendSDPack, timeMin);
  strcat(SendSDPack, timeSec);
  strcat(SendSDPack, kav);
  strcat(SendSDPack, lat1);
  strcat(SendSDPack, kav);
  strcat(SendSDPack, lat2);
  strcat(SendSDPack, kav);
  strcat(SendSDPack, lon1);
  strcat(SendSDPack, kav);
  strcat(SendSDPack, lon2);
  strcat(SendSDPack, kav);
  strcat(SendSDPack, speed);
  strcat(SendSDPack, kav);
  strcat(SendSDPack, course);
  strcat(SendSDPack, kav);
  strcat(SendSDPack, height);
  strcat(SendSDPack, kav);
  strcat(SendSDPack, sats);
  strcat(SendSDPack, EndPack);
  Serial.println(SendSDPack);//Что собрали отправляем в порт мониторинга
  client.print(SendSDPack);//Отправляем на сервер
  uint32_t timer = millis();//Далее мы получаем ответ от сервера
  char simbol;
  char AnswerPack[16] = "";
  byte i = 0;
  while(millis()-timer <= 2000){//Ждем ответ с авторизацией с таймаутом  2000
    if(client.available()){
      simbol = client.read();
      if(simbol == '\n'){
        simbol = 0x00;
        AnswerPack[i+1] = simbol;
        break;
      }
      AnswerPack[i] = simbol;
      i++;
    }
  }
  if( i > 1){
    client.flush();
    Serial.print("AnswerSDPack");
    Serial.println(AnswerPack);
  }
  if(strncmp(AnswerPack, TrueSD, 6) == 0){
    AuthServer = true;
  }
  else{
    AuthServer = false;
    client.stop();
  }
}

void GetTimeNTP(){//Полечение времени
    if (!getLocalTime(&timeinfo)){
      Serial.println("ERROR NTP");
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
    //Serial.println(&timeinfo);
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
      if(simbol == '\n'){
        simbol = 0x00;
        AnswerPack[i+1] = simbol;
        break;
      }
      AnswerPack[i] = simbol;
      i++;
     }
  } 
  if ( i > 1){
    Serial.print("AnswerPack:");
    Serial.println(AnswerPack);
    client.flush();
  }
  if(strncmp(AnswerPack, AnswerPingPack, 4) == 0){
    AuthServer = true;
  }
  else{
    //Serial.println("AUTH NO");
    AuthServer = false;
    client.stop();
  } 

}

void SendLoginPack()
{
  char SendPack[128] = "";
  char AnswerPack[16] = "";
  byte i = 0;
  strcat(SendPack, StartLoginPack);
  strcat(SendPack, imei);
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
    client.stop();
  }
}

void startConfig(){//Данная функция отвечает за стартовые значения при запуске камеры
  byte q = 0;
  Serial.println("Start config");
  flagConfig = EEPROM.read(0);  
  Serial.print(flagConfig, HEX);
  Serial.println();
  //Заполнение SSID если он пустой
  //Заполнение PASS если он пустой
  //Заполнение PORT если он пустой
  //Заполнение HOST если он пустой
  //Заполнение IMEI если он пустой
if (flagConfig!=0x30){//Данная функция по умолчанию заполняет память данным
//host 193.193.165.165
//port 20332
//name wifi geosmt.ru
//pass wifi Amrb3UT9bB4v
char f_host[] ="193.193.165.165";
char f_port[] ="20332";
char f_ssid[] ="SG543644";
char f_pass[] ="1234qwerty";

    if((imei[0]<30)||(imei[0]>39)){//Генерируем IMEI если он пустой
      for(byte i = 0; i<15; i++){
          imei[i] = random(48,57);
      }    
      Serial.println("Create IMEI");
      Serial.println(imei);
    } 
      strcpy(host, f_host);//32
      strcpy(port, f_port);//6
      intport = atoi(port);
      strcpy(ssid, f_ssid);//32
      strcpy(pass, f_pass);//32
      //EEPROM.put(1, ssid);//32
      //EEPROM.put(33, pass);//32
      //EEPROM.put(65, port);//6
      //EEPROM.put(71, host);//32
      //EEPROM.put(103, imei);//16
//===БЛОК записипамяти, работает только для ESP32===//
      //EEPROM.put(1, ssid);//32
      q = 0;
      for(int i = 1; i < 33; i++) {
        EEPROM.write(i,ssid[q]); 
        EEPROM.commit();
        if(ssid[q] == 0x00){
          q = 0;         
          break;          
        }
        q++;
      }            
      //EEPROM.put(33, pass);//32
      q = 0;
      for(int i = 33; i < 65; i++) {
        EEPROM.write(i,pass[q]); 
        EEPROM.commit();
        if(pass[q] == 0x00){
          q = 0;         
          break;          
        }
        q++;
      }       
      //EEPROM.put(65, port);//6
      q = 0;
      for(int i = 65; i < 71; i++) {
        EEPROM.write(i,port[q]); 
        EEPROM.commit();
        if(port[q] == 0x00){
          q = 0;         
          break;          
        }
        q++;
      } 
      //EEPROM.put(71, host);//32
      q = 0;
      for(int i = 71; i < 103; i++) {
        EEPROM.write(i,host[q]); 
        EEPROM.commit();
        if(host[q] == 0x00){
          q = 0;         
          break;          
        }
        q++;
      } 
      //EEPROM.put(103, imei);//16
      q = 0;
      for(int i = 103; i < 119; i++) {
        EEPROM.write(i,imei[q]); 
        EEPROM.commit();
        if(imei[q] == 0x00){
          Serial.println("Конец строки IMEI");
          q = 0;         
          break;          
        }
        q++;
      }      
//Конец блока  записи//

//flagConfig = 0;    
EEPROM.write(0, 0x30); //Данная страка должна сигнализировать что данные по умолчанию записаны и их не требуется больше перезаписывать
EEPROM.commit();
}//конец функции, которая создает данные по умолчанию.
else{
      //EEPROM.get(1, ssid);//32
      q = 0;  
      for (int i = 1; i < 33; i++) {
        ssid [q] = EEPROM.read(i);
        if(ssid [q] == 0x00){
          q= 0;          
          break;          
        } 
        q++;               
      }
      //EEPROM.get(33, pass);//32
      q = 0;  
      for (int i = 33; i < 65; i++) {
        pass [q] = EEPROM.read(i);
        if(pass [q] == 0x00){
          q= 0;          
          break;          
        } 
        q++;               
      }
      //EEPROM.get(65, port);//6
      q = 0;  
      for (int i = 65; i < 71; i++) {
        port [q] = EEPROM.read(i);
        if(port [q] == 0x00){
          q= 0;          
          break;          
        } 
        q++;               
      }
      //EEPROM.get(71, host);//32
      q = 0;  
      for (int i = 71; i < 103; i++) {
        host [q] = EEPROM.read(i);
        if(host [q] == 0x00){
          q= 0;          
          break;          
        } 
        q++;               
      }
      //EEPROM.get(103, imei);//16
      q = 0;  
      for (int i = 103; i < 119; i++) {
        imei [q] = EEPROM.read(i);
        if(imei [q] == 0x00){
          q= 0;          
          break;          
        } 
        q++;               
      }     
}
Serial.println("Настройки по умолчанию:");
Serial.print("ssid:"); Serial.println(ssid);
Serial.print("pass:");Serial.println(pass);
Serial.print("port:");Serial.println(port);
Serial.print("host:");Serial.println(host);
Serial.print("imei:");Serial.println(imei);
}

void setport(){ //данная функция изменяет значение порта
  byte sizeport = 0;
  byte q = 0;
  bool errport = true;
  for(byte i = 8; i<64; i++){
  sizeport++;  
  if(command[i]==0x23) break;
  if((command[i]<0x30)||(command[i]>0x39)){
    Serial.println(errmsg);
    errport = false;
    break;
  }
  if(sizeport>5){
    Serial.println(errmsg);
    errport = false;
    break;
  }
  }
  if(errport){
    byte countport = 0;
    for(byte i = 8; i<64; i++){
      if(command[i]==0x23) {
        port[countport] = 0x00;
        break;
      }
      else port[countport] = command[i];
      countport++;
    }
    //EEPROM.put(65, port);//6
      q = 0;
      for(int i = 65; i < 71; i++) {
        EEPROM.write(i,port[q]); 
        EEPROM.commit();
        if(port[q] == 0x00){
          q = 0;         
          break;          
        }
        q++;
      }     
  }
}

void setssid(){
  byte q = 0;
  bool errssid = false;
  for (byte i = 8; i <64; i ++){
    if (i >= 39 ) {
      errssid = false;
      Serial.println(errmsg);
      break;
    }
    if(command[i]==0x23){
      errssid = true;
      break;
    }
  }
  if(errssid){
    byte countssid = 0;
    for(byte i = 8; i < 64; i++){
      if(command[i]==0x23) {
        ssid[countssid] = 0x00;
        break;
      }
      else ssid[countssid] = command[i];
      countssid++; 
    }
    //EEPROM.put(1, ssid);//32
      q = 0;
      for(int i = 1; i < 33; i++) {
        EEPROM.write(i,ssid[q]); 
        EEPROM.commit();
        if(ssid[q] == 0x00){
          q = 0;         
          break;          
        }
        q++;
      }   
  }
}

void setimei(){
  byte q = 0;
  bool errimei = false;
  byte countimei = 0;
    for(byte i = 8; i <64; i++){
    if(i >= 39){
      Serial.println(errmsg);
      break;
    }
    if(command[i]==0x23){
      errimei = true;
      break;
    }
    countimei++;
  }
  Serial.println(countimei);
  if(countimei!=15){
      Serial.println(errmsg);
      errimei = false;
  }
  if(errimei){
      for(byte i = 0; i< 15; i++){
        imei[i]=command[i+8];
      }
      //imei[16] = 0x00;
      //EEPROM.put(103, imei);//16
      q = 0;
      for(int i = 103; i < 119; i++) {
        EEPROM.write(i,imei[q]); 
        EEPROM.commit();
        if(imei[q] == 0x00){
          Serial.println("Конец строки IMEI");
          q = 0;         
          break;          
        }
        q++;
      }      
  }

}

void setpass(){
  byte q = 0;
  bool errpass = false;
  for(byte i = 8; i <64; i++){
    if(i >= 39){
      Serial.println(errmsg);
      break;
    }
    if(command[i]==0x23){
      errpass = true;
      break;
    }
  }
  if(errpass){
    byte countpass = 0;
    for(byte i = 8; i < 64; i++){
      if(command[i]==0x23) {
        pass[countpass] = 0x00;
        break;
      }
      else pass[countpass] = command[i];
      countpass++; 
    }
    //EEPROM.put(33, pass);//32
      q = 0;
      for(int i = 33; i < 65; i++) {
        EEPROM.write(i,pass[q]); 
        EEPROM.commit();
        if(pass[q] == 0x00){
          q = 0;         
          break;          
        }
        q++;
      }    
  }
}

void sethost(){
  byte q = 0;
  bool errhost = false;
  for(byte i = 8; i <64; i++){
    if(i >= 39){
      Serial.println(errmsg);
      break;
    }
    if(command[i]==0x23){
      errhost = true;
      break;
    }
  }
  if(errhost){
    byte counthost = 0;
    for(byte i = 8; i < 64; i++){
      if(command[i]==0x23) {
        host[counthost] = 0x00;
        break;
      }
      else host[counthost] = command[i];
      counthost++;       
    }
    //EEPROM.put(71, host);//32
    //Serial.println("Настройки сохранены");
      q = 0;
      for(int i = 71; i < 103; i++) {
        EEPROM.write(i,host[q]); 
        EEPROM.commit();
        if(host[q] == 0x00){
          q = 0;         
          break;          
        }
        q++;
      }    
  }
}