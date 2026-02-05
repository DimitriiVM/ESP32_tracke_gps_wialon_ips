#define VERSION 0
#define DEVNAME "IRON_ROGER_TRACKER"
#define RX2 16
#define TX2 17
#define SERIALTIMEOUT 20

#include <HardwareSerial.h>
#include <Preferences.h>

HardwareSerial SIM800(2);
Preferences prefs;

uint32_t Timer0 = 0;

//app.wialonlocal.com:20332
String HOST;
int  PORT;
String APN;
String APN_LOGIN;
String APN_PASSWORD;
String IMEI;

//Статусы работы  для модема, и сети
int StatusModem = 0; //0 нет связи с модемом

char Message[64];
int ZiseMessage = 0;

bool waitForResponse(const String& command, const String& expected, unsigned long timeout = 1000) 
{
  Serial.println("Отправляю: " + command);
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
      Serial.println("Найдено: " + expected);
      Serial.println("Полный ответ: " + response);
      return true;
    }

    delay(10);  // Небольшая задержка для стабильности
  }

  // Таймаут
  Serial.println("Таймаут: ожидание '" + expected + "' превысило " + String(timeout) + " мс");
  Serial.println("Частичный ответ: " + response);
  return false;
}

void setup()
{
  //
  Start();
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
      Serial.printf("Получено: %s\r\n", MSGSIM800.c_str());
    }
    if(Serial.available() > 0)
    {
      ZiseMessage = Serial.readBytes(Message, 64);
      SerialRead();
    }
  }

}

void Start()
{
  Serial.begin(115200);
  Serial.setTimeout(SERIALTIMEOUT);
  SIM800.begin(9600, SERIAL_8N1, RX2, TX2);
  SIM800.setTimeout(SERIALTIMEOUT);
  prefs.begin(DEVNAME);
  HOST = prefs.getString("HOST", "app.wialonlocal.com");
  PORT = prefs.getInt("PORT", 20332);
  APN = prefs.getString("APN", "internet.mts.ru");
  APN_LOGIN = prefs.getString("APN_LOGIN", "mts");
  APN_PASSWORD = prefs.getString("APN_PASSWORD", "mts");
  IMEI= prefs.getString("IMEI", "911");

  Serial.printf("%s START OK\r\n", DEVNAME);
  if(waitForResponse("AT", "OK")) 
  {
    Serial.println("Модуль отвечает!");
  } 
  else 
  {
    Serial.println("Модуль не отвечает.");
  }
}

void SerialRead()
{
  //
  if(strncmp(Message, "AT+", 3) == 0)
  {
    SIM800.printf("%s", Message);
  }
  else if(strncmp(Message, "SEND=", 5) == 0)
  {
    SendUDPPack();
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

void SendUDPPack()
{
  if(waitForResponse("AT+CGATT?", "+CGATT: 1", 5000))
  {
    Serial.printf("GPRS ON\r\n");
    if(waitForResponse("AT+CSTT=\"CMNET\"", "OK", 5000))
    {
      Serial.printf("Start task and set APN OK\r\n");
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
    else
    {
      Serial.printf("Start task and set APN ERROR\r\n");
    }
  }
  else
  {
    Serial.printf("GPRS OFF\r\n");
  }
}






