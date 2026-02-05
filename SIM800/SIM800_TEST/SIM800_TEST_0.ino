#include <HardwareSerial.h>

#define  VERSION "0.01" //Версия  прошивки
#define RXD2 16 //RXX2 pin
#define TXD2 17 //TX2 pin
#define Timeout 50 //Ограничиваем время ожидания  конца пакетов

HardwareSerial SIM800(2);

uint32_t Timer0 = 0;
uint32_t Timer1 = 0;

char Message[64];
int LenBuf = 0;

void setup()
{
 Start();
}

void loop()
{

  if(millis() - Timer0 > 50)
  {
    Timer0 = millis();
    if(Serial.available())
    {
      LenBuf = Serial.readBytes(Message, 64);
      AcceptSerial();
      ClearMessage();
    }
    if(SIM800.available())
    {
      LenBuf = SIM800.readBytes(Message, 64);
      Serial.print(Message);
      ClearMessage();
    }
  }

}

void Start()
{
  Serial.begin(115200);
  Serial.setTimeout(Timeout);
  SIM800.begin(9600, SERIAL_8N1, RXD2, TXD2);
  SIM800.setTimeout(Timeout);
  Serial.printf("Start ok.VERSION:%s\r\n", VERSION);
}

void AcceptSerial()
{
  /*
  for(int  i = 0; i < LenBuf; i++)//Принудительно переводим все в нижний регистр
  {
    if(Message[i]<= 0x5a && Message[i] >= 0x41)
    { //Проверяем большая ли буква
       Message[i] = Message[i] + 32; //Переводим  в нижний  регистр
    }
  }
  */
  if(strncmp(Message, "VERSION", 7) == 0)
  {
    Serial.printf("VERSION:%s\r\n",VERSION);
  }
  else if(strncmp(Message, "AT", 2) == 0)
  {
    SIM800.printf(Message);
  }
  else if(strncmp(Message, "apn", 3) == 0)
  {
    SIM800.printf("AT+CSTT=\"internet.mts.ru\",\"mts\",\"mts\"\r");
  }
  else if(strncmp(Message, "conn", 4) == 0)
  {
    SIM800.printf("AT+CIPSTART=\"TCP\",\"app.wialonlocal.com\",\"20332\"\r");
  }
  else if(strncmp(Message, "cipstatus", 9) == 0)
  {
    SIM800.printf("AT+CIPSTATUS\r");
  }
  else if(strncmp(Message, "ciicr", 5) == 0)
  {
    SIM800.printf("AT+CIICR\r");
  }
  else if(strncmp(Message, "login", 5) == 0)
  {
    SIM800.printf("AT+CIPSEND=11\r");
    //#L#911;NA
  }
  else if(strncmp(Message, "sendlogin", 9) == 0)
  {
    Serial.printf("Отправка пакета  логина\r\n");
    char pack[] = {0x23, 0x4C, 0x23, 0x39, 0x31, 0x31, 0x3B, 0x4E, 0x41, 0x0D, 0x0A};
    SIM800.printf(pack);
    SIM800.print(0x1a);
    Serial.printf("Отправлено...\r\n");
    //#L#911;NA
  }
  else if(strncmp(Message, "ciprxget", 8) == 0)
  {
    SIM800.printf("AT+CIPRXGET=2,64\r");
  }
  else if(strncmp(Message, "cgatt?", 6) == 0)
  {
    SIM800.printf("AT+CGATT?\r");
  }
  else if(strncmp(Message, "pack", 4) == 0)
  {
    SIM800.printf("AT+CIPSEND=75\r");
  }
  else if(strncmp(Message, "sendpack", 8) == 0)
  {
    SIM800.printf("#D#050226;095129;NA;NA;NA;NA;0;0;0;0;0;NA;NA;;123123123;IN205:2:971260.99\r\n");
    SIM800.print(0x1A);
  }
  
}

/*Алгоритм работы модема
AT+CSTT="internet.mts.ru","mts","mts"
AT+CIPSTART="TCP","nl.gpsgsm.org","20332" установить коннект
AT+CIPSTART="TCP","trackhub.wialonlocal.com","20332" установить коннект
trackhub.wialonlocal.com:20332
AT+CIPSTART="TCP","5.34.210.2","20332"
AT+CIPSTART="UDP","trackhub.wialonlocal.com","20332"
AT+CIPCLOSE Закть коннет
911#SD#160725;133000;NA;NA;NA;NA;10;15;20;1\r\n

*/

void ClearMessage()
{
  for(int i = 0; i < 64; i++)
  {
    Message[i] = 0x00;
  }
}
