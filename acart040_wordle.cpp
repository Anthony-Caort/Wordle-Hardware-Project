#include "timerISR-Fixed.h"
#include "usart_ATmega328p.h" // giving me huge errors that i dont want to deal with until display works
#include "acart040_helper_custom.h"
#include "acart040_wordlist.h"
#include "stdlib.h" // for randomness in choosing the word

/*
    Wordle Rules and logic stuff:
    - 6 tries, 5 letter word (obvious)
    - Word is chosen randomly throughout a list of words ill get somewhere
    - if the same letter is repeated, goes through some checks
        - If there is two of the same letter in it, yellow them out appropriately (idk how to spell that word lol)
        - If only one letter is present but two of the same letter is present, yellow out the first appearance of said letter than gray out the second appearance
        - If the correct placement of the letter is found, gray out the wrong one and green out the correct one
    - Recieve inputted letters and form it into a word
    - If the combined word is not a valid word, display an X on the bottom right by comparing the word to its copy in the defined list
    - If the combined word is a valid word check the letter placement by seperating the input and output word as two arrays, each letter being an index
        - If the input is correct, we just send all green back
        - If the word is not correct, we can compare each index and check for previously established rules
    - When done, send back the correct colors in a new array of chars, either being gray, yellow, or green
    - The recieving lcd arduino will put it in a stack so i can easily color from there (letter 5 -> letter 1)
*/

/*
    Other things happening on the second arduino:
    - Controlling multiple LED RGB lights to display the last attempt, will use global variables ffrom worlde task
    - stupid buzzer
*/

/*
    Tasks present in this env:
    Wordle - Task 1
    LED PWM - Task 2
    Buzzer - Task 3
*/

// Global States
int recieveCount = 0;
bool validated = false;
uint8_t buffer;
char currWord[5];
int dutyHigh = 5;
int dutyLow = 5;
letterC letterCheck[5];
uint8_t prevBuffer;
char wordToFind[5];
int dataCount = 0;
int pwmCount = 0;

#define NUM_TASKS 3

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;

task tasks[NUM_TASKS];

const unsigned long TASK1_PERIOD = 50;
const unsigned long TASK2_PERIOD = 1;
const unsigned long TASK3_PERIOD = 50;
const unsigned long GCD_PERIOD = 1;

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

enum WordleStates{waitW, receiving, validation, correction};
enum LEDStates{high, low};
enum buzzerStates{waitB, lastAttempt, fail, win};

int TickFct_Wordle(int state){
    switch(state){ // transitions
        case waitW:
            if (USART_HasReceived()){state = receiving;}
            break;

        case receiving:
            if (recieveCount >= 5) {state = validation;}
            break;

        case validation:
            if (validated) {
                state = correction;
                USART_Send(27); 
                // send a checkmark to lcd screen (make sure checkmark is white)
            }
            else {
                state = waitW;
                USART_Send(28);
                // send a red X to lcd screen
            }
            break;

        case correction:
            state = waitW;
            break;
    }

    switch(state){ // actions
        case waitW:
            break;

        case receiving:
            buffer = USART_Receive();
            letterCheck[recieveCount].letter = buffer;

            if (prevBuffer == buffer) {letterCheck[recieveCount].isRepeated = true;} // identified that this is the second time the letter has appeared

            currWord[recieveCount] = letterID[buffer - 1];
            prevBuffer = buffer;
            recieveCount++;
            break;

        case validation:
            validated = validationProcess(currWord, wordleWords);
            break;

        case correction:
            correctionChecks(currWord, wordToFind, letterCheck);
            for (int i = 4; i < 0; --i){ // send in backward order due to how drawing works on the lcd to simplify it
                USART_Send(letterCheck[i].currentColor);
            }
            break;
    }

    return state;
}

int TickFct_LEDs(int state){ // to get yellow, turn red and green on
    switch(state){
        case high:
            if (pwmCount >= dutyHigh) {
                state = low;
                for (int i = 0; i < 5; ++i){
                    if (letterCheck[i].currentColor != gray){
                        turnOnLED(i, letterCheck);
                    }
                }
                pwmCount = 0;
            }
            pwmCount++;
            break;

        case low:
            if (pwmCount >= dutyLow) {
                state = high;
                turnOffLEDS();
                pwmCount = 0;
            }
            pwmCount++;
            break;
    }

    return state;
}

int TickFct_Buzzer(int state){
    return state;
}

int main(void){

    ADC_init();
    initUSART(); // initializes usart i think
    USART_Flush(); // sanity check to flush the data register

    //buzzer initialization
    
    OCR0A = 255; //sets duty cycle to 50% since TOP is always 256
    // 255 turns off the buzzer, 128 turns on the buzzer 

    TCCR0A |= (1 << COM0A1);// use Channel A
    TCCR0A |= (1 << WGM01) | (1 << WGM00);// set fast PWM Mode
    // TCCR0B = (TCCR0B & 0xF8) | 0x02; //set prescaler to 8
    // TCCR0B = (TCCR0B & 0xF8) | 0x03;//set prescaler to 64
    // TCCR0B = (TCCR0B & 0xF8) | 0x04;//set prescaler to 256
    // TCCR0B = (TCCR0B & 0xF8) | 0x05;//set prescaler to 1024

    unsigned i = 0;
    tasks[i].period = TASK1_PERIOD;
    tasks[i].state = waitW;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_Wordle;
    i++;
    tasks[i].period = TASK2_PERIOD;
    tasks[i].state = high;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_LEDs;
    i++;
    tasks[i].period = TASK3_PERIOD;
    tasks[i].state = waitB;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_Buzzer;

    int randomWord = rand() % 100;
    for (int i = 0; i < 5; ++i){
        wordToFind[i] = wordleWords[randomWord][i];
    }

    TimerSet(GCD_PERIOD);
    TimerOn();

    while(1){}
}