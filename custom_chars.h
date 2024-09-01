// custom_chars.h
#ifndef CUSTOM_CHARS_H
#define CUSTOM_CHARS_H

// Custom characters for the LCD display

// Custom character for pound (£) sign
byte poundChar[8] = {
    0b00111,
    0b01100,
    0b01000,
    0b11110,
    0b01000,
    0b01000,
    0b11111,
    0b00000,
};

// Empty square character
byte emptySquare[8] = {
    0b11111,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111,
    0b00000,
    0b00000,
};

// Filled square character
byte filledSquare[8] = {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b00000,
    0b00000,
};

// Empty square with underline
byte emptySquareBottomUnderlined[8] = {
    0b11111,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111,
    0b00000,
    0b11111,
};

// Filled square with underline
byte filledSquareBottomUnderlined[8] = {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b00000,
    0b11111,
};

#endif // CUSTOM_CHARS_H