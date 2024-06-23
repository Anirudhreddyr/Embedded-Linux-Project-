#include <stdio.h>
#include <assert.h>
#include <cstdint>

#define SPI_CR1_MSTR (1 << 2)
#define SPI_CR1_SPE (1 << 6)
#define SPI_CR1_BR (7 << 3)
#define SPI_CR1_SSI (1 << 8)
#define SPI_CR1_SSM (1 << 9)
#define SPI_CR1_DFF (1 << 11)
#define SPI_CR2_SSOE (1 << 2)
#define SPI_CR2_FRF (1 << 4)
#define SPI_SR_TXE (1 << 1)

//mock registers
volatile unit32_t SPI1_CR1 = 0;
volatile unit32_t SPI1_CR2 = 0;

volatile unit32_t SPI1_DR = 0;
volatile unit32_t SPI1_SR = 0;


void SPI1_Init(void){
    // configure SPI1_CR1 register
    SPI1_CR1 = SPI_CR1_MSTR | SPI_CR1_SPE | SPI_CR1_SSI | SPI_CR1_SSM;

    // configure SPI1_CR2 register
    SPI1_CR2 = SPI_CR2_SSOE | SPI_CR2_FRF;
}


void SPI1_Transmit(uint8_t data){
    while (!(SPI1_SR & SPI_SR_TXE));
    SPI1_DR = data;  
}

void test_SPI_Init_ShouldInitializeSPI1Registers(void){
    SPI1_Init();
    assert((SPI1_CR1 & 0x0340) == 0x0340); //check if the Mode, datat size, and other settings are correct
    assert((SPI1_CR2 & 0x0700) == 0x0700); //check if the NSSP, SSOE settings are correct
}

void test_SPI_Transmit_ShouldTransmitData(void){
    unit8_t data =  0xAA;
    SPI1_SR = SPI_SR_TXE; //Set TXE to indicate the transmitter is empty

    SPI_Transmit(data);

    assert(SPI1_DR == data); //check if the data is transmitted (written to the data register)
}

int main(void){
    test_SPI_Init_ShouldInitializeSPI1Registers();
    test_SPI_Transmit_ShouldTransmitData();
    return 0;
}