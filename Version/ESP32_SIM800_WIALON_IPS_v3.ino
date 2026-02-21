#define VERSION "0.03"
#define RELEASE "20.02.2026"
#define DEVNAME "IRON_ROGER_TRACKER"
#define RX2 16
#define TX2 17
#define SERIALTIMEOUT 20

#include <HardwareSerial.h>
#include <Preferences.h>

HardwareSerial SIM800(2);
Preferences prefs;

uint32_t Timer0 = 0;
uint32_t Timer1 = 0;
uint32_t Timer2 = 0;

//app.wialonlocal.com:20332
String HOST;
int  PORT;
String APN;
String APN_LOGIN;
String APN_PASSWORD;
String IMEI;
String IPLOCAL;

//Статусы работы  для модема, и сети
bool StatusModem = false;     //Статус отвечает ли GSM модуль
bool RegModem = false;        //Зарегистрирован ли модем в сети
bool CIICRModem = false;      //активация PDP‑контекста
bool CIFSRModem = false;      //AT+CIFSR — получение локального IP
bool RoumingModem = false;    //Находится ли модем в роуменге
bool NetModem = false;        //Статус разрешен ли интернет
bool CIPSTARTModem = false;   //Статус соеденения с сервером 
bool StatusSendConn = false;  // Статус отправляли ли мы  запрос на соеденение
bool StatusAUTH = false;      //Статус имеется ли авторизация на сервере
bool StatusCLTS = false;      //активирует автоматическую синхронизацию времени от сети (Network Identity and Time Zone).
int StatusCommand = 0;        //Статус какая команда была запущена
//0 Нет ни каких команд
//1 Была  отправлена  команда проверки отвечает ли сим модуль
//2 запрос можности сигнала
//3 Считывания имея для  установки по умолчанию
//4 Тип регистрации в сети
//5 Включаем разрешение на интернет
//6 AT+CIICR — активация PDP‑контекста
//7 AT+CIFSR — получение локального IP
//8 Контроль соеденения с сервером по TPC //waitForResponse("AT+CIPSTART=\"UDP\",\"app.wialonlocal.com\",\"20332\"", "CONNECT OK", 5000))
//9 признак отправки  данных
//10 Статус отправки  пакета авторизации
//11 Павет с данными
//12 Устанавливаем автоматический  забор времени от сотовой вышки

bool StatusIMEI = false;
int RSSIModem = 0;
char Message[64];
int ZiseMessage = 0;

//Далее это данные для  формирования пакета
String dataPack; //DDMMYY
String timePack; //HHMMSS 
String lat1 = "NA";
String lat2 = "NA";
String lon1 = "NA";
String lon2 = "NA";
String speed = "NA"; 
String course = "NA"; 
String height = "NA"; 
String sats = "NA"; 
String hdop = "NA";
String inputs = "NA"; 
String outputs = "NA";
String adc = "NA";
String ibutton = "NA";
String params;
//dataPack.c_str(), timePack.c_str(), lat1.c_str(), lat2.c_str(), lon1.c_str(), lon2.c_str(), speed.c_str(), course.c_str(), height.c_str(), sats.c_str(), hdop.c_str(), inputs.c_str(), outputs.c_str(), adc.c_str(), ibutton.c_str(), params.c_str()
void SendDataTCP() //#L#imei;password\r\n
{
  if(StatusCommand == 0)
  {
    StatusCommand = 11;
    SIM800.printf("AT+CIPSEND\r\n");
    uint32_t TempTimer = millis();
    //#D#date;time;lat1;lat2;lon1;lon2;speed;course;height;sats;hdop;inputs;outputs;adc;ibutton;params\r\n
    while(millis() - TempTimer < 50)
    {
      if(SIM800.available() > 0)
      {
        String MSGSIM800 = SIM800.readString(); // Считываем всё до таймаута //Ждем ответа  от модуля может стопануть прогу но  не должно
        if(MSGSIM800.indexOf(">") >= 0)//Если мы  получили ответ, что готовы принять сообщение посылаем его
        {
          Serial.printf("#D#%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s\r\n", dataPack.c_str(), timePack.c_str(), lat1.c_str(), lat2.c_str(), lon1.c_str(), lon2.c_str(), speed.c_str(), course.c_str(), height.c_str(), sats.c_str(), hdop.c_str(), inputs.c_str(), outputs.c_str(), adc.c_str(), ibutton.c_str(), params.c_str());
          SIM800.printf("#D#%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s\r\n", dataPack.c_str(), timePack.c_str(), lat1.c_str(), lat2.c_str(), lon1.c_str(), lon2.c_str(), speed.c_str(), course.c_str(), height.c_str(), sats.c_str(), hdop.c_str(), inputs.c_str(), outputs.c_str(), adc.c_str(), ibutton.c_str(), params.c_str());
          SIM800.write(0x1A);
          break;
        }
      }
    }
  }
}

void SendLoginTCP() //#L#imei;password\r\n
{
  if(StatusCommand == 0)
  {
    StatusAUTH = false;
    StatusCommand = 10;
    SIM800.printf("AT+CIPSEND\r\n");
    uint32_t TempTimer = millis();
    while(millis() - TempTimer < 50)
    {
      if(SIM800.available() > 0)
      {
        String MSGSIM800 = SIM800.readString(); // Считываем всё до таймаута //Ждем ответа  от модуля может стопануть прогу но  не должно
        if(MSGSIM800.indexOf(">") >= 0)//Если мы  получили ответ, что готовы принять сообщение посылаем его
        {
          Serial.printf("#L#%s;NA\r\n", IMEI.c_str());
          SIM800.printf("#L#%s;NA\r\n", IMEI.c_str());
          SIM800.write(0x1A);
          break;
        }
      }
    }
  }
}

void SendMSG(String MSG)
{
  if(StatusCommand == 0)
  {
    StatusCommand = 9;
    SIM800.printf("AT+CIPSEND\r\n");
    uint32_t TempTimer = millis();
    while(millis() - TempTimer < 50)
    {
      if(SIM800.available() > 0)
      {
        String MSGSIM800 = SIM800.readString(); // Считываем всё до таймаута //Ждем ответа  от модуля может стопануть прогу но  не должно
        if(MSGSIM800.indexOf(">") >= 0)//Если мы  получили ответ, что готовы принять сообщение посылаем его
        {
          Serial.printf("#M#%s\r\n", MSG.c_str());
          SIM800.printf("#M#%s\r\n", MSG.c_str());
          SIM800.write(0x1A);
          break;
        }
      }
    }
  }
}

void ConnectServer(String Type = "TCP") //TCP or UDP //Поумолчанию всегда TCP
{
  if(StatusCommand == 0)
  {
    StatusSendConn = true;
    StatusCommand = 8;
    CIPSTARTModem = false;
    Serial.printf("AT+CIPSTART=\"%s\",\"%s\",\"%d\"\r\n", Type.c_str(), HOST.c_str(), PORT); 
    SIM800.printf("AT+CIPSTART=\"%s\",\"%s\",\"%d\"\r\n", Type.c_str(), HOST.c_str(), PORT); 
  }
}

void ModemCIFSR() //AT+CIICR — активация PDP‑контекста.
{
  if(StatusCommand == 0)
  {
    StatusCommand = 7;
    CIFSRModem = false;
    SIM800.printf("AT+CIFSR\r\n"); 
  }
}

void ModemCIICR() //AT+CIICR — активация PDP‑контекста.
{
  if(StatusCommand == 0)
  {
    StatusCommand = 6;
    CIICRModem = false;
    SIM800.printf("AT+CIICR\r\n"); 
  }
}

void ModemCLTS() //AT+CIICR — активация PDP‑контекста.
{
  if(StatusCommand == 0)
  {
    StatusCommand = 12;
    StatusCLTS = false;
    SIM800.printf("AT+CLTS=1\r\n"); 
  }
}

void ModemNetOn()
{
  if(StatusCommand == 0)
  {
    StatusCommand = 5;
    NetModem = false;
    SIM800.printf("AT+CSTT=\"CMNET\"\r\n");  
  }
}

void ModemREG() //Тип регистрации в сети //AT+CREG?
{
  if(StatusCommand == 0)
  {
    StatusCommand = 4;
    RegModem = false;
    SIM800.printf("AT+CREG?\r\n");
    {/*
      +CREG: <n>,<stat>
      OK
      <n> — параметр ответа
      0 — незапрашиваемый код регистрации в сети отключен
      1 — незапрашиваемый код регистрации в сети включен
      2 — незапрашиваемый код регистрации в сети включен с информацией о местоположении
      <stat> — статус
      0 — незарегистрирован, не ищет нового оператора для регистрации
      1 — зарегистрирован в домашней сети
      2 — незарегистрирован, но в поиске нового оператора для регистрации
      3 — регистрация запрещена
      4 — неизвестно
      5 — зарегистрирован, в роуминге 
    */}
  }
}

void SetIMEI()
{
  if(StatusCommand == 0)
  {
    StatusCommand = 3;
    SIM800.printf("AT+GSN\r\n");
  }
}

void ModemSignal()
{
  if(StatusCommand == 0)
  {
    StatusCommand = 2;
    SIM800.printf("AT+CSQ\r\n");
  }
}

void ModemOk()
{
  if(StatusCommand == 0)
  {
    StatusCommand = 1;
    StatusModem = false;
    SIM800.printf("AT\r\n");
  }
}

void SIMRead(String S)
{
  //Serial.printf("READ:%s", S.c_str());
  switch(StatusCommand) 
  {
    case 0:
      Serial.printf("READ:%s", S.c_str());
      StatusCommand = 0; //Переводим статус комант в исполненое, нет команд
      break;
    case 1:
    {
      if(S.indexOf("OK") >= 0)
      {
        StatusModem = true; //Модем отвечает и работает
        StatusCommand = 0; //Переводим статус комант в исполненое, нет команд
        Serial.printf("STATUS MODEM ON\r\n");
      }
      else
      {
        StatusModem = false; //Модем не отвечает и не работает
        RegModem = false;
        StatusCommand = 0; //Переводим статус комант в исполненое, нет команд
        Serial.printf("STATUS MODEM OFF\r\n");
      }
      break;
    }
    case 2:
    {
      if(S.indexOf("+CSQ: ") >= 0)
      {
        int pos = S.indexOf(" ") + 1;
        // читаем цифры, пока не встретим не-цифру
        String num = "";
        while (pos < S.length() && isDigit(S[pos])) {
          num += S[pos];
          pos++;
        }
        pos = num.toInt();  // преобразуем в число
        if (pos != RSSIModem)
        {
          Serial.printf("SIGNAL POWER:%d\r\n", RSSIModem);
          RSSIModem = pos;
        }    
        StatusCommand = 0; //Переводим статус комант в исполненое, нет команд
      }      
      break;
    }
    case 3:
    {
      String TEMPIMEI = "";
      char C_IMEI[16];
      for(int i = 2; i < 17; i++)
      {
        TEMPIMEI += S[i];
        C_IMEI[i-2] = char(S[i]);
      }
      IMEI = TEMPIMEI;
      C_IMEI[15] = 0x00;
      Serial.printf("SET IMEI:%s\r\n", IMEI.c_str());
      StatusCommand = 0;
      StatusIMEI = true;
      prefs.putString("IMEI", C_IMEI);//C_IMEI
      IMEI = prefs.getString("IMEI");
      Serial.printf("SET IMEI:%s\r\n", IMEI.c_str());
      break;
    }
    case 4:
    {
      if(S.indexOf("+CREG: ") >= 0)
      {
        int i = S.indexOf(" "); //+CREG: 0,1 // +CREG: <n>,<stat>
        int n = S[i+1] - '0';
        int stat = S[i+3] - '0';
        switch(stat)
        {
          case 0:
          {
            Serial.printf("Незарегистрирован, не ищет нового оператора для регистрации\r\n");
            RegModem = false;
            break;
          }
          case 1:
          {
            Serial.printf("Зарегистрирован в домашней сети\r\n");
            RegModem = true;
            RoumingModem = false;
            break;
          }
          case 2:
          {
            Serial.printf("Незарегистрирован, но в поиске нового оператора для регистрации\r\n");
            RegModem = false;
            break;
          }
          case 3:
          {
            Serial.printf("Регистрация запрещена\r\n");
            RegModem = false;
            break;
          }
          case 4:
          {
            Serial.printf("Неизвестно\r\n");
            RegModem = false;
            break;
          }
          case 5:
          {
            Serial.printf("Зарегистрирован, в роуминге\r\n");
            RegModem = true;
            RoumingModem = true;
            break;
          }
          default:
          {
            Serial.printf("Неизвестная ошибка\r\n");
            RegModem = true;
            break;
          }
        }  
        StatusCommand = 0;
        break;
      }
    }
    case 5:
    {//Активируем работу  интернета
      if(S.indexOf("OK") >= 0)
      {
        Serial.printf("GPRS ON\r\n");
        NetModem = true;
      }
      else
      {
        Serial.printf("GPRS OFF\r\n");
        NetModem = false;
        RegModem = false;
        //SIM800.printf("AT+CFUN=1\r\n");//AT+CFUN=1,1 делаем мягкую перезагрузку
      }
      StatusCommand = 0;
      break;
    }
    case 6:
    {
      if(S.indexOf("OK") >= 0) //AT+CIICR — активация PDP‑контекста.
      {
        CIICRModem = true;
        Serial.printf("CIICR OK\r\n");
      }
      else 
      {
        CIICRModem = false;
        NetModem = false;
        Serial.printf("CIICR ERROR\r\n");
        //SIM800.printf("AT+CFUN=1\r\n");//AT+CFUN=1,1 делаем мягкую перезагрузку
      }
      StatusCommand = 0;
      ModemNetOn();
      break;
    }
    case 7:
    {
      if(S.indexOf("ERROR") >= 0)
      {
        CIFSRModem = false;
        IPLOCAL = "0.0.0.0";
      }
      else
      {
        String TempIP = "";
        int i = 0;
        while(S[i+2] != '\r')
        {
          TempIP += S[i+2];
          i++;
        }
        IPLOCAL = TempIP;
        CIFSRModem = true;
      }
      Serial.printf("IP LOCAL:%s\r\n", IPLOCAL.c_str());
      StatusCommand = 0;
      break;
    }
    case 8:
    {
        if(S.indexOf("CONNECT OK") >= 0)
        {
          CIPSTARTModem = true;
          StatusCommand = 0;
        }
        else
        {
          CIPSTARTModem = false;
        } 
        Serial.printf("SERVER CONNECTED:%d\r\n", CIPSTARTModem);     
        break;
    }
    case 9:
    {
      if(S.indexOf("SEND OK") >= 0)
      {
        Serial.printf("SEND OK\r\n");
      }
      else
      {
        Serial.printf("SEND ERROR\r\n");
      }
      StatusCommand = 0; //Переводим статус комант в исполненое, нет команд
      break;
    }
    case 10:
    {
      if(S.indexOf("SEND OK") >= 0)
      {
        Serial.printf("SEND OK\r\n");
        StatusCommand = 10;
        break;
      }
      else
      {
        if(S.indexOf("#AL#1") >= 0)
        {
          Serial.printf("AUTH OK\r\n");
          StatusAUTH = true;
          StatusCommand = 0;
          break;
        }
        else
        {
          Serial.printf("AUTH ERROR\r\n");
          StatusAUTH = false;
          StatusCommand = 0;
          break;
        }        
        Serial.printf("SEND ERROR\r\n");
        StatusCommand = 0;
        break;
      }
      break;
    }
    case 11:
    {
      break;
    }
    case 12:
    {
      if(S.indexOf("OK") >= 0)
      {
        StatusCLTS = true;
      }
      else
      {
        StatusCLTS = false;
      }
      StatusCommand = 0;
      Serial.printf("CLTS=%d\r\n", StatusCLTS);
      break;
    }
    default:
    {
      Serial.printf("READ:%s", S.c_str());
      StatusCommand = 0; //Переводим статус комант в исполненое, нет команд
      break;
    }
  }
}

void SerialRead()
{
  //
  if(strncmp(Message, "AT+", 3) == 0)
  {
    SIM800.printf("%s", Message);
  }
  else if(strncmp(Message, "VERSION", 7) == 0)
  {
    SIM800.printf("VERSION:%s RELEASE:%s\r\n", VERSION, RELEASE);
  }
  else if(strncmp(Message, "RESET", 5) == 0)
  {
    esp_restart();
  }
  else if(strncmp(Message, "GETIMEI", 7) == 0)
  {
    Serial.printf("IMEI:%s\r\n", IMEI.c_str());
  }
  else if(strncmp(Message, "GETHOST", 7) == 0)
  {
    Serial.printf("HOST:%s\r\n", HOST.c_str());
  }
  else if(strncmp(Message, "GETPORT", 7) == 0)
  {
    Serial.printf("PORT:%d\r\n", PORT);
  }
  else if(strncmp(Message, "GETAPN", 6) == 0)
  {
    Serial.printf("APN:%s LOGIN:%s PASSWORD:%s\r\n", APN.c_str(), APN_LOGIN.c_str(), APN_PASSWORD.c_str());
  }
  else if(strncmp(Message, "PORT=", 5) == 0)
  {
    PORT = atoi(&Message[5]);
    prefs.putInt("PORT", PORT);
    Serial.printf("SET PORT:%d\r\n", PORT);
  }
  else if(strncmp(Message, "HOST=", 5) == 0)
  {
    int i = 0;
    char TempHOST[32];
    while(Message[i+5] != '\r')
    {
      TempHOST[i] = Message[i+5];
      i++;
    }
    TempHOST[i] = 0x00;
    HOST = String(TempHOST);
    Serial.printf("SET HOST:%s\r\n", HOST.c_str());
    prefs.putString("HOST", HOST);
  }
  else if(strncmp(Message, "APNLOGIN=", 9) == 0)
  {
    int i = 0;
    char TempAPN_LOGIN[32];
    while(Message[i+9] != '\r')
    {
      TempAPN_LOGIN[i] = Message[i+9];
      i++;
    }
    TempAPN_LOGIN[i] = 0x00;
    APN_LOGIN = String(TempAPN_LOGIN);
    Serial.printf("SET APN LOGIN:%s\r\n", APN_LOGIN.c_str());
    prefs.putString("APN_LOGIN", APN_LOGIN);
  }
  else if(strncmp(Message, "APNPASSWORD=", 12) == 0)
  {
    int i = 0;
    char TempAPN_PASSWORD[32];
    while(Message[i+12] != '\r')
    {
      TempAPN_PASSWORD[i] = Message[i+12];
      i++;
    }
    TempAPN_PASSWORD[i] = 0x00;
    APN_PASSWORD = String(TempAPN_PASSWORD);
    Serial.printf("SET APN PASSWORD:%s\r\n", APN_PASSWORD.c_str());
    prefs.putString("APN_PASSWORD", APN_PASSWORD);
  }
  else if(strncmp(Message, "APN=", 4) == 0)
  {
    int i = 0;
    char TempAPN[32];
    while(Message[i+4] != '\r')
    {
      TempAPN[i] = Message[i+4];
      i++;
    }
    TempAPN[i] = 0x00;
    APN = String(TempAPN);
    Serial.printf("SET APN:%s\r\n", APN.c_str());
    prefs.putString("APN", APN);
  }
  else if(strncmp(Message, "GETIP", 5) == 0)
  {
    Serial.printf("IP LOCAL:%s\r\n", IPLOCAL.c_str());
  }
  else if(strncmp(Message, "SENDM=", 5) == 0)//УДАЛИ ПОТОМ, ТОЛЬКО ДЛЯ ПРОВЕРКИ
  {
    String MSG = "";
    int i = 0;
    while(Message[i+6] != '\r')
    {
      MSG += Message[i+6];
      i++;
    }
    //Serial.printf("MSG:%s\r\n", MSG.c_str());
    SendMSG(MSG);
  }
  ClearMsg(); //Почистим  массив от мусора
}

void ClearMsg()
{
  for(int  i = 0; i < 64; i++)
  {
    Message[i] = 0x00;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(SERIALTIMEOUT);
  SIM800.begin(9600, SERIAL_8N1, RX2, TX2);
  SIM800.setTimeout(SERIALTIMEOUT);
  prefs.begin(DEVNAME, false);
  prefs.begin("IMEI", false);
  HOST = prefs.getString("HOST", "app.wialonlocal.com");
  PORT = prefs.getInt("PORT", 20332);
  APN = prefs.getString("APN", "internet.mts.ru");
  APN_LOGIN = prefs.getString("APN_LOGIN", "mts");
  APN_PASSWORD = prefs.getString("APN_PASSWORD", "mts");
  IMEI = prefs.getString("IMEI", "0");
  Serial.printf("HOST:%s PORT:%d IMEI:%s\r\n", HOST.c_str(), PORT, IMEI.c_str());
  Serial.printf("APN:%s LOGIN:%s PASSWORD:%s\r\n", APN.c_str(), APN_LOGIN.c_str(), APN_PASSWORD.c_str());
  if(IMEI.length() < 15)
  {
    StatusIMEI = false;
  }
  else
  {
    StatusIMEI = true;
  }
  Serial.printf("%s START OK\r\n", DEVNAME);
  ModemOk();
}

void loop()
{
  //
  if(millis() - Timer0 > 50)
  {
    Timer0 = millis();
    if(SIM800.available() > 0)
    {
      String MSGSIM800 = SIM800.readString(); // Считываем всё до таймаута
      //Serial.printf("Получено: %s\r\n", MSGSIM800.c_str());
      SIMRead(MSGSIM800);
    }
    if(Serial.available() > 0)
    {
      ZiseMessage = Serial.readBytes(Message, 64);
      SerialRead();
    }
  }

  if(millis() - Timer1 > 59999)
  {
    Timer1 = millis();
    ModemOk();  //Будем  проверять  не  отвалился ли модем
  }

  if(millis() - Timer2 > 1001)
  {
    Timer2 = millis();
    if(!StatusIMEI && StatusModem)
    {
      SetIMEI();
    }
    if(StatusModem && !RegModem)
    {
      ModemREG();
    }
    if(!StatusCLTS && StatusModem && RegModem)
    {
      ModemCLTS();
    }
    if(StatusModem && RegModem && !NetModem)
    {
      ModemNetOn();
    }
    if(NetModem && !CIICRModem) //Если модем зарегистрирован, но  нет активации
    {
      ModemCIICR();
    }
    if(!CIFSRModem && CIICRModem && NetModem)
    {
      ModemCIFSR();
    }
    if(StatusModem && RegModem && CIICRModem && CIFSRModem && NetModem && !CIPSTARTModem && !StatusSendConn)
    {
      ConnectServer("TCP");
    }
    if(CIPSTARTModem && StatusSendConn && !StatusAUTH)
    {
      SendLoginTCP();
    }
    if(StatusModem)
    {
      ModemSignal();  //Будем  проверять  не  отвалился ли модем
    }
  }
}

/*
void SendUDPPack()
{
  if(waitForResponse("AT+CGATT?", "+CGATT: 1", 5000))
  {
    Serial.printf("GPRS ON\r\n");
    if(waitForResponse("AT+CSTT=\"CMNET\"", "OK", 5000))
    {
      Serial.printf("APN OK\r\n");
    }
    if(waitForResponse("AT+CIICR", "OK", 5000))
    {
      waitForResponse("AT+CIFSR", "OK", 5000);
      if(waitForResponse("AT+CIPSTART=\"UDP\",\"app.wialonlocal.com\",\"20332\"", "CONNECT OK", 5000))
      {
        Serial.printf("Connected OK!\r\n");
        if(waitForResponse("AT+CIPSEND", ">", 5000))
        {
          SIM800.printf("911#M#%s\r\n", Message);
          SIM800.write(0x1A);
          delay(500);
          waitForResponse("AT+CIPCLOSE", "OK", 5000);
        }
      }
    }
  }
}
*/

/*
bool waitForResponse(const String& command, const String& expected, unsigned long timeout = 1000) 
{
  Serial.printf("Отправляю:%s\r\n", command);
  SIM800.write(command.c_str());
  SIM800.write('\r');  // Или '\n', в зависимости от устройства

  String response = "";
  unsigned long start = millis();

  while (millis() - start < timeout) 
  {
    while (SIM800.available()) 
    {
      char c = SIM800.read();
      response += c;
      // Ограничиваем длину буфера, чтобы не переполнить память
      if (response.length() > 256) 
      {
        response = response.substring(1);  // Удаляем первый символ
      }
    }

    // Проверяем, есть ли ожидаемая строка
    if (response.indexOf(expected) >= 0) 
    {
      Serial.printf("Найдено:%s\r\n", expected.c_str());
      Serial.printf("Полный ответ:%s\r\n", response.c_str());
      return true;
    }

    delay(10);  // Небольшая задержка для стабильности
  }

  // Таймаут
  Serial.println("Таймаут: ожидание '" + expected + "' превысило " + String(timeout) + " мс");
  Serial.println("Частичный ответ: " + response);
  return false;
}
*/