#include "RC522.h"

//----- Construcción ------
RC522::RC522 (uint8_t inici, uint8_t reseteo)
{
  //inicio = inici;
  pinMode(inici, OUTPUT); // Pin digital para activar
  digitalWrite(inici, HIGH);

  //reseteo = reseteo;
  pinMode(reseteo, OUTPUT); //Pin para resetear
  digitalWrite(reseteo, HIGH);
}

// ---- Definición de pines -----
RC522::RC522 (uint8_t inicio, uint8_t reseteo, uint8_t mosiPin, uint8_t misoPin)
{
  // inicio = inicio;
  // reseteo = reseteo;
  // misoPin = misoPin;
  // mosiPin = mosiPin;
  //_clockPin = clockPin;

  pinMode(inicio, OUTPUT);
  digitalWrite(inicio, LOW);
  pinMode(reseteo,OUTPUT);
  digitalWrite(reseteo,HIGH);

  //pinMode(_clockPin, OUTPUT);
  pinMode(mosiPin, OUTPUT);
  pinMode(misoPin, INPUT);
}

// ----- Lectura 1 (Si hay tarjeta en el campo)-----

bool RC522::HayTarjeta()
{
uint8_t Estado;
uint8_t Est[LargoMax];
SPI.beginTransaction(SPISettings (20000000,MSBFIRST,SPI_MODE0));
Estado = Pregunta1(PICC_REQIDL, Est);

   if (Estado == MI_OK)
  {
    SPI.endTransaction();
   return true;

  }
 else
 {
   SPI.endTransaction();
   return false;
 }
}

// ----- Leectura 2 - Lectura del serial -----

uint8_t RC522::LeerSerialTarjeta()
{
  SPI.beginTransaction(SPISettings (20000000,MSBFIRST,SPI_MODE0));
  uint8_t Estado;
  uint8_t Est[LargoMax];

Estado = anticoli(Est); // Para evitar colisión, devolver el número de la serie (4 bytes)
memcpy(serNum, Est, 5); // Generar copia binaria de los datos

 if (Estado == MI_OK)
 {
   SPI.endTransaction();
  return true;
 }
 else
 {
   SPI.endTransaction();
   return false;
 }

}

// ----- Inicio -----

void RC522::inicio()
{
    digitalWrite(_NRSTPD,HIGH);
  	reset();

	//Timer: TPrescaler*TreloadVal/6.78MHz = 24ms
  //   writeMFRC522(TModeReg, 0x8D);		//Tauto=1; f(Timer) = 6.78MHz/TPreScaler
  //   writeMFRC522(TPrescalerReg, 0x3E);	//TModeReg[3..0] + TPrescalerReg
  //   writeMFRC522(TReloadRegL, 30);
  //   writeMFRC522(TReloadRegH, 0);
  //
	// writeMFRC522(TxAutoReg, 0x40);		//100%ASK
	// writeMFRC522(ModeReg, 0x3D);		// CRC valor inicial de 0x6363

	//ClearBitMask(Status2Reg, 0x08);	//MFCrypto1On=0
	//writeMFRC522(RxSelReg, 0x86);		//RxWait = RxSelReg[5..0]
	//writeMFRC522(RFCfgReg, 0x7F);   	//RxGain = 48dB

	antennaOn();		//Abre  la antena


}

// ----- Reseteo -----

void RC522::reset()
{
	write522(ComReg, PCD_RESETPHASE);
}

// ----- Escritura 1 ----

void RC522::write522(uint8_t addr, uint8_t val)
{
  SPI.beginTransaction(SPISettings (20000000,MSBFIRST,SPI_MODE0));
    digitalWrite(inici, LOW);
    SPI.transfer((addr<<1)&0x7E); // Formato direccion X000000X
    SPI.transfer(val);
    digitalWrite(inici, HIGH);
    SPI.endTransaction();

}

// ----- Lectura 3 (Byte de registro) -----
uint8_t RC522::read522(uint8_t addr)
{
    uint8_t val;
    SPI.beginTransaction(SPISettings (20000000,MSBFIRST,SPI_MODE0));
    digitalWrite(inici, LOW);
    SPI.transfer(((addr<<1)&0x7E) | 0x80);
    val = SPI.transfer(0x00);
    digitalWrite(inici, HIGH);
    SPI.endTransaction();
    return val;	// Retorna byte de lectura
}

// ----- Encender Antena -----
void RC522::antennaOn(void)
{
	uint8_t temp;

	temp = read522(TxControlReg);
	if (!(temp & 0x03))
	{
		BitMask(TxControlReg, 0x03);
	}
}

// ----- Poner Mascara de lectura -----
void RC522::BitMask(uint8_t reg, uint8_t mask)
{
    uint8_t temp;
    SPI.beginTransaction(SPISettings (20000000,MSBFIRST,SPI_MODE0));
    temp = read522(reg);
    write522(reg, temp | mask);  // Establecer la máscara
    SPI.endTransaction();
}

// ----- Limpiar máscara -----
void RC522::clearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t temp;
    SPI.beginTransaction(SPISettings (20000000,MSBFIRST,SPI_MODE0));
    temp = read522(reg);
    write522(reg, temp & (~mask));  // Limpiar  bit mask
    SPI.endTransaction();
}

// ----- CRC -----

void RC522::calculateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData)
{
    uint8_t i, n;
SPI.beginTransaction(SPISettings (20000000,MSBFIRST,SPI_MODE0));
    clearBitMask(DivIrqReg, 0x04);			//CRCIrq = 0
    BitMask(FIFOLevelReg, 0x80);			//Puntero FIFO


	//----- Escribir datos en el FIFO	-----
    for (i=0; i<len; i++)
    {
		write522(FIFODataReg,*(pIndata+i));
	  }

    write522(ComReg, PCD_CALCCRC);

	// Esperar el cálculo de CRC
    i = 0xFF;//255
    do
    {
        n = read522(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));//CRCIrq = 1

	//----- Lectura 4 (CRC) -----
    pOutData[0] = read522(CRCResultRegL);
    pOutData[1] = read522(CRCResultRegM);
    SPI.endTransaction();
}

// ----- Transcribir a la tarjeta (certificación) -----

uint8_t RC522::RC522ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen)
{
    uint8_t Estado = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
	  uint8_t lastBits;
    uint8_t n;
    uint16_t i;

    switch (command)
    {
        case PCD_AUTHENT:		// Tarjetas de certificación cerca
		{
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case PCD_TRANSCEIVE:	//La transmisión de datos FIFO
		{
			irqEn = 0x77;
			waitIRq = 0x30;
			break;
		}
		default:
			break;
    }
write522(CommIEnReg, irqEn|0x80);	//Solicitar pausa
clearBitMask(CommIrqReg, 0x80);// Borrar todos lo de antes
BitMask(FIFOLevelReg, 0x80);	//FlushBuffer
write522(ComReg, PCD_IDLE);	//Si NO hay acción cancelar el comando

 //-----Escribir datos en el FIFO
   for (i=0; i<sendLen; i++)
   {
   write522(FIFODataReg, sendData[i]);
 }

 write522(ComReg, command);
   if (command == PCD_TRANSCEIVE)
   {
   BitMask(BitFramingReg, 0x80);//Inicio transmisión de datos
 }

 // Esperar
 i = 1800;
   do
   {
       n = read522(CommIrqReg);
       i--;
   }
   while ((i!=0) && !(n&0x01) && !(n&waitIRq));

   clearBitMask(BitFramingReg, 0x80);//StartSend=0

   if (i != 0)
   {
       if(!(read522(ErrorReg) & 0x1B))//BufferOvfl Collerr CRCErr ProtecolErr
       {
           Estado = MI_OK;
           if (n & irqEn & 0x01)
           {
       Estado = MI_NO;
     }

           if (command == PCD_TRANSCEIVE)
           {
               n = read522(FIFOLevelReg);
               lastBits = read522(ControlReg) & 0x07;
               if (lastBits)
               {
         *backLen = (n-1)*8 + lastBits;
       }
               else
               {
         *backLen = n*8;
       }

               if (n == 0)
               {
         n = 1;
       }
               if (n > LargoMax)
               {
         n = LargoMax;
       }

       //----- Lea los datos recibidos en el FIFO
               for (i=0; i<n; i++)
               {
         backData[i] = read522(FIFODataReg);
       }
           }
       }
       else
       {
     Estado = MI_ERR;
   }

   }

     return Estado;
}
// ---- Solicitar datos ----

uint8_t RC522::Pregunta1(uint8_t Modo, uint8_t *TagType)
{
	uint8_t Estado;
	uint16_t backBits;//Recibir los bits (datos)
  SPI.beginTransaction(SPISettings (20000000,MSBFIRST,SPI_MODE0));

	write522(BitFramingReg, 0x07);
	TagType[0] = Modo;
	Estado = RC522ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((Estado != MI_OK) || (backBits != 0x10))
	{
		Estado = MI_ERR;
	}
  SPI.endTransaction();
	return Estado;
}
// ----- Anticolición (lectura número de tarjeta) ----

uint8_t RC522::anticoli(uint8_t *serNum)
{
    uint8_t Estado;
    uint8_t i;
	  uint8_t serNumCheck=0;
    uint16_t unLen;
    SPI.beginTransaction(SPISettings (20000000,MSBFIRST,SPI_MODE0));

	write522(BitFramingReg, 0x00);

    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    Estado = RC522ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (Estado == MI_OK)
	{
		for (i=0; i<4; i++)
		{
		 	serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[i])
		{
			Estado = MI_ERR;
		}
  }
  SPI.endTransaction();
  return Estado;
}
// ----- Lectura 5 (Bloque) -----

uint8_t RC522::read(uint8_t blockAddr, uint8_t *recvData)
{
    uint8_t Estado;
    uint16_t unLen;
    recvData[0] = PICC_READ;
    recvData[1] = blockAddr;

    calculateCRC(recvData,2, &recvData[2]);
    Estado = RC522ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

    if ((Estado != MI_OK) || (unLen != 0x90))
    {
        Estado = MI_ERR;
    }

    return Estado;
}
// ----- Escritura 2 (Bloque) -----

uint8_t RC522::write(uint8_t blockAddr, uint8_t *writeData)
{
    uint8_t Estado;
    uint16_t recvBits;
    uint8_t i;
	  uint8_t buff[18];

    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    calculateCRC(buff, 2, &buff[2]);
    Estado = RC522ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

    if ((Estado != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
    {
		Estado = MI_ERR;
   	}

    if (Estado == MI_OK)
    {
        for (i=0; i<16; i++)
        {
        	buff[i] = *(writeData+i);
        }
        calculateCRC(buff, 16, &buff[16]);
        Estado = RC522ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);
      }

		if ((Estado != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
        {
			Estado = MI_ERR;
	    	}

    return Estado;
}
