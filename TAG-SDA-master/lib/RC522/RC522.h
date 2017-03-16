#ifndef RC522_h
#define RC522_h

#include <Arduino.h>
#include <SPI.h>
//#include <SS.h>

#define LargoMax 16

// ---- Opciones de comunicación -----
#define MI_OK                 0
#define MI_NO                 1
#define MI_ERR                2

// ----- Comandos -----
#define ComReg                0x01
#define CommIEnReg            0x02
#define CommIrqReg            0x04
#define PCD_RESETPHASE        0x0F// Reajustar
#define DivIrqReg             0x05
#define ErrorReg              0x06
#define FIFOLevelReg          0x0A
#define FIFODataReg           0x09
#define PCD_CALCCRC           0x03 // Calcular CRC
#define CRCResultRegL         0x22
#define CRCResultRegM         0x21
#define BitFramingReg         0x0D
#define PCD_TRANSCEIVE        0x0C// Enviar y recibir datos
#define PICC_ANTICOLL         0x93
#define PICC_READ             0x30// leer bloque
#define PICC_WRITE            0xA0// Escribir en el bloque
#define PICC_REQIDL           0X26
#define PCD_AUTHENT           0x0E //Autenticar performence
#define ControlReg            0x0C
#define PCD_IDLE              0x00

class RC522
{
public:
  RC522(uint8_t inici, uint8_t reseteo);
  RC522(uint8_t inici,uint8_t reseteo, uint8_t mosiPin, uint8_t misoPin);
	bool HayTarjeta();
	uint8_t LeerSerialTarjeta();
  uint8_t inici;

  void inicio();
  void reset();
  void AntennaOn();
  void write522(uint8_t dir, uint8_t val);
  uint8_t Pregunta1(uint8_t Modo, uint8_t *TagType);
  uint8_t anticoli(uint8_t *serNum);
  uint8_t read522(uint8_t addr);
  void BitMask(uint8_t reg, uint8_t mask);
  //void setBitMask(uint8_t reg, uint8_t mask);
  void clearBitMask(uint8_t reg, uint8_t mask);
	void calculateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData);
  //uint8_t RC522Req(uint8_t reqMode, uint8_t *TagType);
  uint8_t RC522ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen);
	//uint8_t auth(uint8_t authMode, uint8_t BlockAddr, uint8_t *Sectorkey, uint8_t *serNum);
	uint8_t read(uint8_t blockAddr, uint8_t *recvData);
	uint8_t write(uint8_t blockAddr, uint8_t *writeData);
	//void halt();
	uint8_t serNum[5];       // Constante para guardar el numero de serie leido.
	//uint8_t AserNum[5];      // Constante para guardar el numero d serie de la secion actual.
  //uint8_t PICCDetection();
  //uint8_t PICCReading();
  //uint8_t Return();

private:
uint8_t _ElegirPin;
uint8_t _reseteo;
uint8_t _mosiPin;
uint8_t _misoPin;
uint8_t _NRSTPD;
//uint8_t _clockPin;

};

#endif
