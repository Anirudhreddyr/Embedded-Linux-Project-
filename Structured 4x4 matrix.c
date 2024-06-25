/* Structured Programming in Chapter 5 of "The C Companion"
  to develop a 4x4 keyboard scannig program from scratch */

#include <stdio.h>
#include <unistd.h> //for unsleep function

//define the row and column of the 4x4 matrix
#define ROWS 4
#define COLS 4

//define the key values for the 4x4 matrix keyboard
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

//simulate setting the rows and columns (for the purpose of this example)
void setROWHigh(int row){
    //simulate setting setting a row high
    printf("Row %d is set high\n", row);
}

int readColumn(int col){
    //simulate reading a column (for demonstration purposes, always return LOW)
    return 0; //Change to 1 to simulate a key press
}

void debounce(void){
    usleep(5000);  //5ms debounce delay
}

/*Scanning logic*/
int scanKeboard(char *key){
    int row, col;

    setALLColsLOW(); //set all columns low

    for (row = 0; row < ROWS; row++){
        setROWHigh(row); //set a row high
        
        for (col = 0; col < COLS; col++){
            if (readColumn(col)) {
                debounce();
                    if (readColumn(col)){ 
                    // confirm key press
                    *key = keys[row][col];
                    return 1; //key pressed
                }
            }
        }
    
    }   
    return 0; //no key press detected 
}

/*Debouce Mechanism is used to implement a delay to ensure stable key press detection*/

int main(){
    char key = '\0';

    if (scanKeyboard(&key)){
        printf("Key pressed: %c\n", key);
    } else {
        printf("No key pressed\n");
    }
    return 0;
}