#include "stm8s.h"
#include "main.h"
#include "milis.h"

#include <stdio.h>
#include <stdbool.h>
#include "uart1.h"

#define EEPROM_START FLASH_DATA_START_PHYSICAL_ADDRESS

/*---         Hlavičky funkcí                                                     ---*/
int8_t check_ncoder(void);
void eeprom_write(uint32_t address, int32_t number);
int32_t eeprom_read(uint32_t address);
void eeprom_write_union_way(uint32_t address, int32_t number);
int32_t eeprom_read_union_way(uint32_t address);

void setup(void)
{
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);      // taktovani MCU na 16MHz

    GPIO_Init(LED_PORT,  LED_PIN,  GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(CLK_PORT,  CLK_PIN,  GPIO_MODE_IN_FL_NO_IT);
    GPIO_Init(DATA_PORT, DATA_PIN, GPIO_MODE_IN_FL_NO_IT);
    GPIO_Init(SW_PORT,   SW_PIN,   GPIO_MODE_IN_FL_NO_IT);
    GPIO_Init(BTN_PORT,  BTN_PIN,  GPIO_MODE_IN_FL_NO_IT);

    // odemčeme přístup k paměti dat ("EEPROM")
    FLASH_Unlock(FLASH_MEMTYPE_DATA);   
    // nastavíme programovací čas
    FLASH_SetProgrammingTime(FLASH_PROGRAMTIME_STANDARD);  

    init_milis();
    init_uart1();
}


int8_t CLK_last_state;

int main(void)
{
    uint32_t time = 0;
    int32_t cislo = 0;
    uint8_t SW_state=0; 


    setup();
    CLK_last_state = READ(CLK);  // pro korektní inicializaci práce s n-coderem

    printf("\nStart...\n");
    cislo = eeprom_read(EEPROM_START);
    /*cislo = eeprom_read_union_way(EEPROM_START);*/

    while (1) {
        cislo += check_ncoder();

        // při stisku tlačítka 
        if (PUSH(SW) && !SW_state) {
            eeprom_write(EEPROM_START, cislo);
            /*eeprom_write_union_way(EEPROM_START, cislo);*/
            puts("Hotnota zapsána\n");
        }
        SW_state = PUSH(SW);

        if (milis() - time > 333 ) {
            REVERSE(LED); 
            time = milis();
            printf("\r %10ld   ", cislo);
        }

    }
}


/**
  * @brief  Funkce obsluhuje ncoder
  * @param  None
  * @retval -1 nebo 0 nebo 1 podele toho jestli došlo k otočení a na kterou stranu
  */
int8_t check_ncoder(void)
{
    int8_t now_state = READ(CLK);
    int8_t result = 0;

    // náběžná hrana
    if (CLK_last_state == 0 && now_state == 1 ) {
        if (READ(DATA)) {
            result = -1;
        } else {
            result = 1;
        }
    }
    // setupná hrana
    if (CLK_last_state == 1 && now_state == 0 ) {
        if (READ(DATA)) {
            result = 1;
        } else {
            result = -1;
        }
    }

    CLK_last_state = now_state;
    return result;
}


/**
  * @brief  Write 32-b number to EEPROM 
  * @note   standard  way
  * @param  address: address to start write
  * @param  number: number for write
  * @retval none
  */
void eeprom_write(uint32_t address, int32_t number)
{
    uint8_t byte[4];
    
    byte[3] = number & 0x000000FFL;
    byte[2] = (number & 0x0000FF00L) >> 8;
    byte[1] = (number & 0x00FF0000L) >> 16;
    byte[0] = (number & 0xFF000000L) >> 24;
    
    for (short i = 0; i < 4; ++i) {
        FLASH_ProgramByte(address + i, byte[i]);
    }
}


/**
  * @brief  Read 32-b number from EEPROM 
  * @note   standard  way
  * @param  address: address to start write
  * @retval readed number
  */
int32_t eeprom_read(uint32_t address)
{
    uint8_t byte[4];
    
    for (short i = 0; i < 4; ++i) {
        byte[i] = FLASH_ReadByte(address + i);
    }

    return ((int32_t)byte[0] << 24) |
           ((int32_t)byte[1] << 16) |
           ((int32_t)byte[2] << 8 ) |
           ((int32_t)byte[3]) ;
}


typedef union {
    uint32_t whole;
    uint8_t byte[4];
} Data_t;

/**
  * @brief  Write 32-b number to EEPROM 
  * @note   (Union way)
  * @param  address: address to start write
  * @param  number: number for write
  * @retval none
  */
void eeprom_write_union_way(uint32_t address, int32_t number)
{
    Data_t data;

    data.whole = number;
    FLASH_ProgramByte(address + 0, data.byte[0]);
    FLASH_ProgramByte(address + 1, data.byte[1]);
    FLASH_ProgramByte(address + 2, data.byte[2]);
    FLASH_ProgramByte(address + 3, data.byte[3]);
}


/**
  * @brief  Read 32-b number from EEPROM 
  * @note   (Union way)
  * @param  address: address to start write
  * @retval readed number
  */
int32_t eeprom_read_union_way(uint32_t address)
{
    Data_t data;

    data.byte[0] = FLASH_ReadByte(address+0);
    data.byte[1] = FLASH_ReadByte(address+1);
    data.byte[2] = FLASH_ReadByte(address+2);
    data.byte[3] = FLASH_ReadByte(address+3);
    return data.whole;
}
/*-------------------------------  Assert -----------------------------------*/
#include "__assert__.h"
