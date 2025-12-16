# Demo Check-In #2

Most of the progress in this check-in occurred behind the scenes. On the visual side, a **15×15 pixel font** was created using a custom printing algorithm based on inputted `unsigned short` bit patterns.

This check-in demonstrates the original implementation idea: using **two Arduino boards communicating over UART** to reduce memory usage. One Arduino was responsible for handling the LCD display, while the other handled the *Wordle* game logic and computations.

On the *Wordle* Arduino, a **binary search** algorithm was used to validate input words. The program then iterated through both the correct word and the user’s input to detect errors and assign color feedback to each letter, replicating the behavior of the original *Wordle* game.
