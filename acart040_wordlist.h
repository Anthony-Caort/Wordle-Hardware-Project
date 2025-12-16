//blehhhhhhhhhh
#include <avr/io.h>
#include <avr/interrupt.h>

const char* wordleWords[100] = {
    "aback", "abase", "abate", "ahead", "alarm",
    "alone", "amuse", "ankle", "awake", "batty",
    "bench", "bleak", "blush", "brink", "bunny",
    "cable", "candy", "cider", "chant", "cheer",
    "chest", "cigar", "climb", "clean", "coach",
    "colon", "could", "crept", "croak", "crust",
    "death", "digit", "evade", "feign", "fewer",
    "fifty", "flair", "flame", "floss", "flung",
    "focal", "forge", "found", "frost", "glory",
    "glove", "grade", "grant", "hasty", "heath",
    "helix", "honey", "humph", "input", "karma",
    "laden", "lager", "light", "limit", "loser",
    "lunar", "major", "marry", "mason", "model",
    "naval", "paper", "pilot", "power", "pride",
    "primo", "radar", "react", "rebut", "reply",
    "rhyme", "river", "serve", "shake", "shirt",
    "shore", "shown", "shook", "sissy", "snail",
    "solid", "spelt", "spill", "stink", "stool",
    "story", "straw", "stunk", "stung", "tinge",
    "vivid", "waist", "watch"
};

// also letter list with correspondance to a number 1-26 for easy buffer retrieval
char letterID[26] = {
    'a', 'b', 'c', 'd', 'e',
    'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y',
    'z'
};