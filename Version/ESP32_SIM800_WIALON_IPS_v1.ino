#define VERSION "0.01"
#define RELEASE "18.02.2026"
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

//Статусы работы  для модема, и сети
bool StatusModem = false;     //Статус отвечает ли GSM модуль
int StatusCommand = 0;        //Статус какая команда была запущена
//0 Нет ни каких команд
//1 Была  отправлена  команда проверки отвечает ли сим модуль
//2 запрос можности сигнала
//3 Считывания имея для  установки по умолчанию

bool StatusIMEI = false;
int RSSIModem = 0;
char Message[64];
int ZiseMessage = 0;

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
        RSSIModem = num.toInt();  // преобразуем в число
        Serial.printf("SIGNAL POWER:%d\r\n", RSSIModem);
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
  else if(strncmp(Message, "PORT=", 5) == 0)
  {
    PORT = atoi(&Message[5]);
    prefs.putInt("PORT", PORT);
    Serial.printf("SET PORT:%d\r\n", PORT);
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