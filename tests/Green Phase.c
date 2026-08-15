#include <stdio.h>
#include <assert.h>

#define SPI_CR1_MSTR (1 << 2)
#define SPI_CR1_SPE (1 << 6)
#define SPI_CR1_BR (7 << 3)
#define SPI_CR1_SSI (1 << 8)
#define SPI_CR1_SSM (1 << 9)
#define SPI_CR1_DFF (1 << 11)
#define SPI_CR2_SSOE (1 << 2)
#define SPI_CR2_FRF (1 << 4)

//mock registers
volatile unit32_t SPI1_CR1 = 0;
volatile unit32_t SPI1_CR2 = 0;

void SPI1_Init(void){
    // configure SPI1_CR1 register
    SPI1_CR1 = SPI_CR1_MSTR | SPI_CR1_SPE | SPI_CR1_SSI | SPI_CR1_SSM;

    // configure SPI1_CR2 register
    SPI1_CR2 = SPI_CR2_SSOE | SPI_CR2_FRF;
}

void test_SPI_Init_ShouldInitializeSPI1Registers(void){
    SPI1_Init();
    assert((SPI1_CR1 & 0x0340) == 0x0340); //check if the Mode, datat size, and other settings are correct
    assert((SPI1_CR2 & 0x0700) == 0x0700); //check if the NSSP, SSOE settings are correct
}

int main(void){
    test_SPI_Init_ShouldInitializeSPI1Registers();
    return 0;
}