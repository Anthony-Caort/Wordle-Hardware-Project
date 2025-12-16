#include "timerISR-Fixed.h"
#include "spiAVR.h"
#include "acart040_helper_custom.h"
#include "acart040_character_sheet.h"
#include "serialATmega-4.h"
#include "acart040_wordlist.h"
#include "stdlib.h" // for randomness in choosing the word

// the beginning starting coordinates of the first frame
#define beginningX 23
#define beginningY 4

// the first coordinates of the first sent word, can derive from here
#define playStartX 23 // x is the same but I think its more intuitive to use a different name for different use cases
#define playStartY 108

#define NUM_TASKS 4

/*
Colors used: (B G R)
Background Gray: 19 18 18
Square Gray: 60 58 58
Square Yellow: 59 159 181
Square Green: 78 141 83
Text White: 255 255 255
*/

/*
Square Shape: 17 x 17
Font Size: 15 x 15
How the letters gonna work:
  - Not enough space to comfortably sit everything so the letters are gonna sit on the side (first letter on the bottom last letter on top)
  - On the 6th try, have a buzzer to alarm the player and then when the word is submitted the appropriate boxes will appear with the letters in them
*/

/*
  Starting positions of user letters (top to bottom) (x,y):
  (3, 101)
  (3, 82)
  (3, 63)
  (3, 44)
  (3, 25)
*/

/*
  Tasks present in this env:

  LCD  - Task 1
  Buttons - Task 2
  Wordle - Task 3
  Buzzer - Task 4

*/


// Global Variables for lcd (before the split)
int userLetterNums[] = {1, 1, 1, 1, 1}; // used to keep track what letter is what so far to draw easily
int attempt = 1; // the current attempt the player is on
int playerLetterY[] = {25, 44, 63, 82, 101}; // probably have to change this
int attemptX[] = {108, 87, 66, 45, 24}; // the starting X positions for each letter
int attemptY[] = {5, 26, 47, 68, 89, 110}; // the starting Y positions for each attempt
int printCount = 0; // tracking the prints for the current attempt
bool winningAttempt = true;
bool correct;
int stallCount = 0;


typedef struct _buttons{
  bool pressed = false;
  bool read = false;
} buttons;

buttons buttonActions[6]; // the current button states for all 6 buttons

// Global Variables for Wordle (before the split)
bool validated = false;
char currWord[5];
letterC letterCheck[5];  // the colors for each letter for the current attempt
char wordToFind[5];

// Global Variables for buzzer
bool lastA = false;
int buzzerCount = 0;

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;

task tasks[NUM_TASKS];

const unsigned long TASK1_PERIOD = 50;
const unsigned long TASK2_PERIOD = 200;
const unsigned long TASK3_PERIOD = 50;
const unsigned long TASK4_PERIOD = 50;
const unsigned long GCD_PERIOD = findGCD(TASK3_PERIOD, TASK2_PERIOD);


void TimerISR(){
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

enum LCDStates{startIntro, drawTemplate, userInput, waitRetrieval, updateWord, stallEnd};
enum ButtonStates{standby, updateButton, held};
enum WordleStates{waitW, validation, correction};
enum buzzerstates{waitB, lastAttempt, fail, win};

int TickFct_LCD(int state){
  switch(state){ // transitions
    case startIntro:
      if (buttonActions[5].pressed) {state = drawTemplate; buttonActions[5].read = true;}
      break;

    case drawTemplate:
      state = userInput;
      break;

    case userInput: // is the main state we play in
      if (buttonActions[5].pressed && !buttonActions[5].read) {
        state = waitRetrieval;
        buttonActions[5].read = true;
      }
      break;

    case waitRetrieval:
        if (validated) {
          state = updateWord;
          drawLetter(beginningX - 20, beginningX - 6, 6, 20, letters, 26, darkGray); // draws a white checkmark to show it validated
        } 
        else {
          state = userInput;
          drawLetter(beginningX - 20, beginningX - 6, 6, 20, letters, 23, red); // draws a red X to show it didn't validate correctly
        }
      break;

    case updateWord:
      if (printCount >= 6) {
        serial_println("attempt");
        serial_println(attempt);
        if (winningAttempt) {correct = true;}
        attempt++;
        if (attempt > 6 || correct){
          state = stallEnd;
          attempt = 1;
          for (int i = 0; i < 5; ++i){
            userLetterNums[i] = 1;
          }
        }
        else{
          state = userInput;
        }
        printCount = 0;
        resetColors(letterCheck);
        winningAttempt = true;
      }
      break;

    case stallEnd: // stall for 2 seconds so you can see results and stuff
      if (stallCount >= 40){ // restart game
        stallCount = 0;
        state = startIntro;
        drawBackground(0, 128, 0, 128);
        drawIntroText(attemptX);
        correct = false;
      }
      break;
  }

  switch(state){ // actions
    case startIntro:
      break;

    case drawTemplate:
      drawBackground(0, 128, 0, 128);
      drawStart(beginningX, beginningX + 16, beginningY, beginningY + 16); // draws the main playing screen
      break;

    case userInput:
      for (int i = 0; i < 5; ++i){
        if (buttonActions[i].pressed && !buttonActions[i].read){
          serial_println("changing character");
          drawGuessColor(beginningX - 20, beginningX - 6, playerLetterY[i], playerLetterY[i] + 14, darkGray); // clear out the previous letter
          drawLetter(beginningX - 20, beginningX - 6, playerLetterY[i], playerLetterY[i] + 14, letters, userLetterNums[i], darkGray);
          userLetterNums[i]++;
          if (userLetterNums[i] >= 26) {userLetterNums[i] = 0;}
          buttonActions[i].read = true;
        }
      }
      break;

    case waitRetrieval:
      break;

    case updateWord:
      if(printCount > 0){
        drawLetter(attemptX[printCount - 1], attemptX[printCount - 1] + 14, attemptY[attempt - 1], attemptY[attempt - 1] + 14, letters, userLetterNums[printCount - 1] - 1, letterCheck[printCount - 1].currentColor);
        if (letterCheck[printCount - 1].currentColor != green) {winningAttempt = false;}
      }
      printCount++;
      break;

    case stallEnd:
      stallCount++;
      break;
  }
  return state;
}

int TickFct_Buttons(int state){
  switch(state){ // transitions
    case standby:
      if (PINC > 0) {state = updateButton;}
      break;

    case updateButton:
      state = held;
      break;

    case held:
      if (!(PINC > 0)) {state = standby;}
      break;
  }

  switch(state){ // actions
    case standby:
      for (int i = 0; i < 6; ++i){
        if (buttonActions[i].read) {buttonActions[i].pressed = false; buttonActions[i].read = false;} // reset button states so we don't overlap on inputs
      }
      break;

    case updateButton:
      for (int i = 5; i >= 0; i--){ // exclusive, we're expecting the user to not hold down multiple buttons at once
        if (GetBit(PINC, i)) {
            buttonActions[i].pressed = true; 
            break;
        }
      } 
      break;

    case held:
      break;
  }
  return state;
}

int TickFct_Wordle(int state){
    switch(state){ // transitions
        case waitW:
            if (tasks[0].state == waitRetrieval) {state = validation;}
            break;

        case validation:
            if (validated) {state = correction;}
            else {state = waitW;}
            break;

        case correction:
            state = waitW;
            break;
    }

    switch(state){ // actions
        case waitW:
            break;

        case validation:
            // convert ints into letters for the validation process
            for (int i = 0; i < 5; ++i){
                currWord[i] = letterID[userLetterNums[i] - 1];
                serial_println(currWord[i]);
            }
            validated = validationProcess(currWord, wordleWords);
            break;

        case correction:
            correctionChecks(currWord, wordToFind, letterCheck);
            break;
    }

  return state;
}

int TickFct_Buzzer(int state){
  switch(state){ // transitions
    case waitB:
      if (attempt == 6 && !lastA) {state = lastAttempt;}
      else if (correct) {
        state = win;
        lastA = false;
      }
      else if (tasks[0].state == stallEnd && !correct) {state = fail;}
      else {state = waitB; lastA = false;}

      break;

    case lastAttempt:
      if (buzzerCount >= 13) {state = waitB; buzzerCount = 0; lastA = true;}
      break;

    case fail:
      if (buzzerCount >= 12) {state = waitB; buzzerCount = 0;}
      break;

    case win:
      if (buzzerCount >= 32) {state = waitB; buzzerCount = 0; }
      break;
  }

  switch(state){ // actions
    case waitB:
      OCR1A = 0;
      break;

    case lastAttempt: // C5, A4, Ab4, D5
      serial_println("lastAttempt buzzer");
      OCR1A = ICR1 / 4; // 25% duty cycle

      if (buzzerCount < 3) {ICR1 = 3822;}
      else if (buzzerCount < 5) {ICR1 = 4545;}
      else if  (buzzerCount < 7) {ICR1 = 4816;}
      else if (buzzerCount < 13) {ICR1 = 3405;}

      buzzerCount++;
      break;

    case fail:  // C1, B0
      serial_println("fail buzzer");
      OCR1A = ICR1 / 4;
      if (buzzerCount < 6) {ICR1 = 61156;}
      else if (buzzerCount < 12) {ICR1 = 64792;}

      buzzerCount++;
      break;

    case win: // G3, C3, E3, G3, C4, E4, G4, E4, 
      serial_println("win buzzer");
      OCR1A = ICR1 / 4;
      if (buzzerCount < 4) {ICR1 = 10204;} // G3, the if statements are the notes in order 
      else if (buzzerCount < 8) {ICR1 = 15288;}
      else if (buzzerCount < 12) {ICR1 = 12135;}
      else if (buzzerCount < 16) {ICR1 = 10204;}
      else if (buzzerCount < 20) {ICR1 = 7644;}
      else if (buzzerCount < 24) {ICR1 = 6067;}
      else if (buzzerCount < 28) {ICR1 = 5406;}
      else if (buzzerCount < 32) {ICR1 = 6067;}

      buzzerCount++;
      break;
  }
  return state;
}

// RST is PORTB0
// A0 is PORTB1
// SS is PORTB2

int main(void){
  DDRB = 0x2F;
  PORTB = 0x30; 

  DDRC = 0x3F;
  PORTC = 0x00;

  DDRD = 0xC0;
  PORTD = 0x00;

  SPI_INIT();
  ST7735_init();
  serial_init(9600);

  //buzzer initialization

  TCCR1A = 0;
  TCCR1B = 0;

  TCCR1A |= (1 << COM1A1);// use Channel A
  TCCR1A |= (1 << WGM11);

  TCCR1B |= (1 << WGM12) | (1 << WGM13); // fast PWM mode
  TCCR1B |= (1 << CS11); // prescalar of 8


  ICR1 = 39999; //20ms pwm period
  OCR1A = 0;

  // these functions are for testing and stuff
  drawBackground(0, 128, 0, 128);
  drawIntroText(attemptX);
  // drawStart(beginningX, beginningX + 16, beginningY, beginningY + 16);
  // drawGuessColor(beginningX + 1, beginningX + 15, beginningY + 1, beginningY + 15);
  // drawLetter(beginningX + 1, beginningX + 15, beginningY + 1, beginningY + 15, letters, 3);
  // drawStart(23, 39, 4, 20);
  // drawLetter(24, 38, 5, 19, letters); // testing stuff blehhhh :P

  unsigned i = 0;
  tasks[i].period = TASK1_PERIOD;
  tasks[i].state = startIntro;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_LCD;
  i++;
  tasks[i].period = TASK2_PERIOD;
  tasks[i].state = standby;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_Buttons;
  i++;
  tasks[i].period = TASK3_PERIOD;
  tasks[i].state = waitW;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_Wordle;
  i++;
  tasks[i].period = TASK4_PERIOD;
  tasks[i].state = waitB;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_Buzzer;

  TimerSet(GCD_PERIOD);
  TimerOn();

  resetColors(letterCheck);

  //int randomWord = rand() % 100;
  int randomWord = 0 ;
  for (int i = 0; i < 5; ++i){
    wordToFind[i] = wordleWords[randomWord][i];
    serial_println(wordToFind[i]);
  }
  
  while(1){}
}