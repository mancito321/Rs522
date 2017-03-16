
#include <SPI.h>
#include <RC522.h>
#define SS_PIN 10
#define RST_PIN 9
RC522 rfid(SS_PIN, RST_PIN);

String cardID;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  RC522  inicio();
}



void HayTarjeta()
{
  if (rfid.HayTarjeta())
  {
    Serial.println("Se ha detectado una tarjeta cerca del lector");
    if (rfid.LeerSerialTarjeta())
    {
      cardID = String(rfid.serNum[0]) + String(rfid.serNum[1]) + String(rfid.serNum[2]) + String(rfid.serNum[3]) + String(rfid.serNum[4]);
      Serial.println(cardID);
      //
    }
}
}
void loop()
{
  HayTarjeta();
}
