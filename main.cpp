#include "I2C.h"
#include "mbed.h"
#include "PN532.h"
#include <inttypes.h>
#include "PN532_HSU.h"
#include "PN532_SPI.h"
#include "PN532_I2C.h"

I2C pn532_i2c(D14, D15);
PN532_I2C pn532_if (pn532_i2c);

PN532 nfc(pn532_if);

DigitalOut ledRed(D5);
DigitalOut ledGreen(D3);
DigitalOut rstNFC(LED1);


PwmOut buz(PB_1);

Serial pc(USBTX, USBRX);

 
/*==============================================================================
 * Настройка датчика
 */
void setup(void)
{
    ledRed = 0;
    ledGreen = 0;
 
    uint32_t versiondata = 0;
    pc.baud(115200);
    pc.printf ("Hello!\n");
 
    while (1) {
        nfc.begin();
        //nfc.SAMConfig();
        versiondata = nfc.getFirmwareVersion();
        if (! versiondata) {
            pc.printf("Didn't find PN53x board\n\n");
            wait_ms(2000);
        } else {
            break;
        }
    }
 
    // Всё ок!
    pc.printf ("Found chip PN5%02X , Firmware ver. %d.%d\n",
               (versiondata>>24) & 0xFF,
               (versiondata>>16) & 0xFF,
               (versiondata>>8) & 0xFF);
 
    // Максимальное количество повторных попыток считывания с карты
    nfc.setPassiveActivationRetries(0xFF);
 
    // Конфигурация для чтения карт.
    nfc.SAMConfig();
 
    pc.printf ("\nWaiting for an ISO14443A card\n");
}
 
 
/*==============================================================================
 * Нахождение метки
 */
void loop(void)
{
    bool success;
    int access;
    int master_access;
    
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Буфер для хранения возвращаемого UID метки
    uint8_t uidLength;  // Длина UID метки
 
    // configure board to read RFID tags
//    nfc.SAMConfig();
 
    // Нахождение метки
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
 
 
    if (success) {
//        tone (buz, 800);        // turn on the buzzer
        ledGreen = 0;           // led off
        ledRed = 0;             // led off
 
        pc.printf("\n\nFound a card!\n");
 
        pc.printf("UID Length: %d bytes\n", uidLength);
        pc.printf("UID Value: ");
 
        for (uint8_t i=0; i < uidLength; i++){pc.printf(" 0x%02X", uid[i]);}
        
        uint8_t masterkey[] = {0x04, 0x1C, 0x65, 0x9A, 0xEE, 0x32, 0x80};
        
        uint8_t userkey[] = {0xB0, 0xC9, 0x6E, 0xA3};
        
        int var = 0;
        
        for (uint8_t i=0; i < uidLength; i++){
            if (uid[i] != masterkey[i]) {master_access=0;}
                else {master_access=1;}
            if (uid[i] != userkey[i]) {access=0;}
                else {access=1;}
            if ((master_access==0) & (access==0)){i = uidLength;}
        }
        
        if (access || master_access){pc.printf("\n Authorization done");}
            else{pc.printf("\n Authorization failed");}
        
        while(var<50){
            if(success){
                if (master_access){
                    ledGreen = 1;
                    wait_ms(50);
                    ledGreen = 0;
                    wait_ms(50);
                    ++var;
                }
                else {var=101;}
            }
            else {var=101;}
 
//            tone (buz, 0);          // turn off the buzzer
 
            // Включение указывающих диодов
            success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
        }
        
        if (var==50){
            pc.printf("\n\n Master-authorization done");
            
            while (success){
                ledGreen = 1;
                ledRed = 1;
                success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
            }      
        }
        

         // wait until the card is taken away
        while (success) {
            success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
            if(access){
                ledGreen = 1;
                wait_ms (50);
                ledGreen = 0;
                wait_ms (50);
            }
                else{
                    ledRed = 1;
                    wait_ms (50);
                    ledRed = 0;
                    wait_ms (50); 
                }
        }
    }
    
        else {
            //Пока не найдена карта будет гореть красный диод
//          pc.printf("\nTimed out waiting for a card\n");
            ledGreen = 0;
            ledRed = 1;
        }
}
 
 

int main()
{
    setup();
 
    while (1)
        loop ();
}
 
