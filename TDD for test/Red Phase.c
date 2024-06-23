#include <stdio.h>
#include <assert.h>

//mock registers
volatile unit32_t SPI1_CR1 = 0;
volatile unit32_t SPI1_CR2 = 0;

void SPI1_Init(void);

void 
test_SPI_Init_ShouldInitializeSPI1Registers(void){
    SPI1_Init();
    assert((SPI1_CR1 & 0x0340) == 0x0340); //check if the Mode, datat size, and other settings are correct
    assert((SPI1_CR2 & 0x0700) == 0x0700); //check if the NSSP, SSOE settings are correct
}

int main(void){
    test_SPI_Init_ShouldInitializeSPI1Registers();
    return 0;
}