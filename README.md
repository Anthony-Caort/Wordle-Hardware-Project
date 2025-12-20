# Final Submission

Most of the work completed for the final submission focused on stabilizing the core gameplay and optimizing memory usage within a single-Arduino configuration. Due to time constraints, the original dual-Arduino design was reduced to one board after experiencing inconsistent results from SPI communication between the arduinos.

Although the SPI Arduino communication was removed, the **Wordle game logic remained unchanged** and functioned correctly during testing. This included input validation and letter evaluation logic developed earlier in the project. The communication layer is a component that could be reintroduced in the future to further reduce memory constraints.

The final implementation operates within approximately **90–94% RAM usage**, which remained stable despite frequent LCD screen redraws during runtime. To compensate for the removed secondary Arduino, a **buzzer feedback system** was added. Distinct sound patterns are played when the player reaches the final attempt, loses the game, or successfully guesses the word.

With additional time, the SPI communication issues would be fully diagnosed to allow support for a larger word set without exceeding memory limits. Further cleanup of the codebase would also be performed, as some testing logic and inputs remains from earlier development stages.

Overall, this project served as a practical application of embedded systems concepts, particularly **memory management**, hardware limitations, and modular design, while recreating the core mechanics of the *Wordle* game.
