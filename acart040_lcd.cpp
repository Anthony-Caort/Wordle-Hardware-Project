#include "timerISR-Fixed.h"
#include "usart_ATmega328p.h" // giving me huge errors that i dont want to deal with until display works
#include "spiAVR.h"
#include "acart040_helper_custom.h"
#include "acart040_character_sheet.h"
#include "serialATmega-4.h"

// the beginning starting coordinates of the first frame
#define beginningX 23
#define beginningY 4

// the first coordinates of the first sent word, can derive from here
#define playStartX 23 // x is the same but I think its more intuitive to use a different name for different use cases
#define playStartY 108

#define NUM_TASKS 2
#define Validated 27 // the number that is receieved if the sent word i in the wordle list

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
  - Not enough space to comfortably sit everything so the letters are gonna sit where 6th try is 
  - On the 6th try, have a buzzer to alarm the player and then when the word is submitted the appropriate boxes will appear with the letters in them
*/

/*
  Starting positions bottom to top (x, y) (this is done algorithmically in code this just for me):
  (23, 108)
  (23, 87)
  (23, 46)
  (23, 25)
  (23, 4)
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
  USART Data Values SEND to wordle Ardunio(basically what number values mean):
  1-26: letters in order

  USART Data Values Recieving from wordleArduino
  0: gray
  1: yellow
  2: green
  27: White Checkmark on the bottom right if the word exists
  28: Red X on the bottom right if the word doesn't exist (reuse the letter X)
*/

/*
  Tasks present in this env:

  LCD  - Task 1
  Buttons - Task 2

*/


// Global Variables
uint8_t receivingBuffer;

int userLetterNums[] = {1, 1, 1, 1, 1}; // used to keep track what letter is what so far
                                        // also used to send data to wordle arduno for later
int attempt = 1; // the current attempt the player is on

int playerLetterY[] = {25, 44, 63, 82, 101}; // probably have to change this
int sendCount = 0;

typedef struct _buttons{
  bool pressed = false;
  bool read = false;
} buttons;

buttons buttonActions[6];

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
const unsigned long GCD_PERIOD = findGCD(TASK1_PERIOD, TASK2_PERIOD);


void TimerISR(){
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

enum LCDStates{startIntro, drawTemplate, userInput, sendWord, waitRetrieval, updateWord};
enum ButtonStates{standby, held, updateButton};

int TickFct_LCD(int state){
  switch(state){ // transitions
    case startIntro:
      if (buttonActions[5].pressed) {state = drawTemplate; buttonActions[5].read = true;}
      break;

    case drawTemplate:
      state = userInput;
      break;

    case userInput: // is the main state we play in
      //if (send) {state = sendWord;}
      break;

    case sendWord: // when testing for demo 2, do not uncommment from this point on in the state machine it is way too experimental
                   // related to usart stuff
      if (sendCount >= 5) {state = waitRetrieval;}
      break;

    case waitRetrieval:
      if (USART_HasReceived()) {
        receivingBuffer = USART_Receive();
        (receivingBuffer == Validated) ? state = updateWord : state = userInput;
      }
      break;

    case updateWord:
      break;
  }

  switch(state){ // actions
    case startIntro:
      break;

    case drawTemplate:
      drawStart(beginningX, beginningX + 16, beginningY, beginningY + 16); // draws the main playing screen
      break;

    case userInput:
      for (int i = 0; i < 5; ++i){
        if (buttonActions[i].pressed && !buttonActions[i].read){
          drawGuessColor(beginningX - 20, beginningX - 6, playerLetterY[i], playerLetterY[i] + 14, darkGray); // clear out the previous letter
          drawLetter(beginningX - 20, beginningX - 6, playerLetterY[i], playerLetterY[i] + 14, letters, userLetterNums[i], darkGray);
          userLetterNums[i]++;
          if (userLetterNums[i] >= 26) {userLetterNums[i] = 0;}
          buttonActions[i].read = true;
        }
      }
      break;

    case sendWord:
      USART_Send(userLetterNums[sendCount]);
      while(!USART_HasTransmitted()){} // doesn't continue unless the data has sent, error prone
      sendCount++;
      break;

    case waitRetrieval:
      break;

    case updateWord:

      break;
  }

  return state;
}

int TickFct_Buttons(int state){
  switch(state){ // transitions
    case standby:
      if (PINC > 0) {state = updateButton;}
      break;

    case held:
      if (!(PINC > 0)) {state = standby;}
      break;

    case updateButton:
      state = held;
      // reset button states so we don't overlap on inputs
      break;
  }

  switch(state){ // actions
    case standby:
      for (int i = 0; i < 6; ++i){
        if (buttonActions[i].read) {buttonActions[i].pressed = false; buttonActions[i].read = false;}
      }
      break;

    case held:
      break;

    case updateButton:
      for (int i = 0; i < 6; i++){ // exclusive, we're expecting the user to not hold down multiple buttons at once
        if (GetBit(PINC, i)) {buttonActions[i].pressed = true; serial_println(i); break;}
      } 
      break;
  }

  return state;
}

// RST is PORTB0
// A0 is PORTB1
// SS is PORTB2

int main(void){
  DDRB = 0x2F;
  PORTB = 0x10; 

  DDRC = 0x3C;
  PORTC = 0x00;

  SPI_INIT();
  ST7735_init();
  initUSART(); // comment out for demo 2
  serial_init(9600);


  // these functions are for testing and stuff
  drawBackground(0, 128, 0, 128);
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

  TimerSet(GCD_PERIOD);
  TimerOn();

  while(1){}
}