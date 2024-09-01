#include <WiFi.h>
#include <LiquidCrystal.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cstring>
#include "config.h"

// Array of cryptocurrency symbols
const char *cryptoSymbols[] = {"BTC", "ETH", "SOL", "AVAX", "DOT", "ADA", "DOGE"};
const int numCryptos = sizeof(cryptoSymbols) / sizeof(cryptoSymbols[0]);

// Fiat currency symbol (e.g., "GBP" or "USD")
const char *FIAT_SYMBOL = "USD";

// LCD and Button Configurations
#define POUND_CHAR 0
#define BTC_CHAR 1
#define EMPTY_SQUARE_CHAR 2
#define FILLED_SQUARE_CHAR 3
#define EMPTY_SQUARE_BOTTOM_FILLED_CHAR 4
#define FILLED_SQUARE_BOTTOM_FILLED_CHAR 5
#define BUTTON_PIN 33
#define X_CELLS 16
#define Y_CELLS 2

LiquidCrystal lcd(13, 12, 25, 26, 27, 14); // RS, E, D4, D5, D6, D7

bool buttonPressed = false;   // Variable to keep track of button state
bool isTickerRunning = false; // Flag to indicate if the ticker has started
int currentCryptoIndex = 0;   // Start with the first cryptocurrency (BTC)

unsigned long previousMillis = 0;        // Stores the last time API was polled
unsigned long countdownMillis = 0;       // Stores the last time the countdown was updated
const long pollInterval = 5000;          // Interval at which to poll the API (5 seconds)
const long countdownInterval = 1000;     // Interval at which to update countdown (1 second)
const long cryptoSwitchInterval = 25000; // Interval at which to switch cryptocurrency (25 seconds)
int countdownProgress = 0;               // Tracks progress of 5-second countdown
int currentIntervalStep = 0;             // Tracks the number of 5-second intervals (0 to 4)

byte btcChar[8] = {
    0b01010,
    0b11111,
    0b01001,
    0b01111,
    0b01001,
    0b11111,
    0b01010,
    0b00000,
};

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

// Custom characters for visual countdown
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

byte emptySquareBottomFilled[8] = {
    0b11111,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111,
    0b00000,
    0b11111,
};

byte filledSquareBottomFilled[8] = {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b00000,
    0b11111,
};

void setup()
{
  Serial.begin(115200); // 115200 baud rate

  lcd.begin(X_CELLS, Y_CELLS);
  lcd.createChar(POUND_CHAR, poundChar);
  lcd.createChar(BTC_CHAR, btcChar);
  lcd.createChar(EMPTY_SQUARE_CHAR, emptySquare);
  lcd.createChar(FILLED_SQUARE_CHAR, filledSquare);
  lcd.createChar(EMPTY_SQUARE_BOTTOM_FILLED_CHAR, emptySquareBottomFilled);
  lcd.createChar(FILLED_SQUARE_BOTTOM_FILLED_CHAR, filledSquareBottomFilled);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Display initial "Ready" message
  lcd.setCursor(0, 0);
  lcd.print("Ready!");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    lcd.setCursor(0, 1); // Move the cursor to the second row
    lcd.print("Connecting...");
  }
  Serial.println("\nWiFi connected.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Click button");
  lcd.setCursor(0, 1);
  lcd.print("to start tracker");
}

void loop()
{
  unsigned long currentMillis = millis();

  // Check if the button is pressed
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    if (!buttonPressed)
    {
      buttonPressed = true;
      Serial.println("Button pressed!");

      if (!isTickerRunning)
      {
        // Start ticker mode
        isTickerRunning = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(cryptoSymbols[currentCryptoIndex]);
        pollAPI();                       // Immediately fetch and display the price
        countdownMillis = currentMillis; // Initialize countdown timer
        previousMillis = currentMillis;  // Initialize poll timer
      }
    }
  }
  else
  {
    buttonPressed = false;
  }

  if (isTickerRunning)
  {
    // Check if it's time to poll the API
    if (currentMillis - previousMillis >= pollInterval)
    {
      previousMillis = currentMillis;
      pollAPI(); // Poll the API every 5 seconds

      // After each poll, reset the countdown progress
      countdownProgress = 0;
      currentIntervalStep++; // Increment the interval step

      // Update the visual countdown for 5-second intervals
      updateVisualCountdown();
    }

    // Check if it's time to update the countdown
    if (currentMillis - countdownMillis >= countdownInterval)
    {
      countdownMillis = currentMillis;
      countdownProgress++;
      updateVisualCountdown(); // Update the visual countdown
    }

    // Check if it's time to switch the cryptocurrency
    if (currentIntervalStep >= (cryptoSwitchInterval / pollInterval))
    {
      currentIntervalStep = 0;
      countdownProgress = 0;
      lcd.clear();
      currentCryptoIndex = (currentCryptoIndex + 1) % numCryptos;
      lcd.setCursor(0, 0);
      lcd.print(cryptoSymbols[currentCryptoIndex]);
      pollAPI(); // Immediately fetch and display the new price
    }
  }

  // Short delay to prevent a busy-wait loop
  delay(100);
}

void pollAPI()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    // Dynamically construct the API URL for the selected cryptocurrency and FIAT symbol
    String serverName = "https://api.pro.coinbase.com/products/";
    serverName += cryptoSymbols[currentCryptoIndex];
    serverName += "-";
    serverName += FIAT_SYMBOL;
    serverName += "/ticker";

    http.begin(serverName);

    // Send the GET request
    int httpResponseCode = http.GET();

    // Check the response code
    if (httpResponseCode > 0)
    {
      String payload = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(payload);

      // Parse JSON
      StaticJsonDocument<200> doc; // Adjust the size based on your JSON structure

      DeserializationError error = deserializeJson(doc, payload);

      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("JSON Error");
        return;
      }

      // Extract the price value
      float price = doc["price"]; // Assuming the API returns a JSON object with a "price" key

      Serial.print(cryptoSymbols[currentCryptoIndex]);
      Serial.print("/");
      Serial.print(FIAT_SYMBOL);
      Serial.print(" Price: ");
      Serial.println(price);

      // Display the price on the LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(cryptoSymbols[currentCryptoIndex]);
      lcd.print("/");
      lcd.print(FIAT_SYMBOL);
      lcd.print(":");
      lcd.setCursor(0, 1);
      if (strcmp(FIAT_SYMBOL, "GBP") == 0)
      {
        lcd.write(byte(POUND_CHAR)); // Display custom Pound character
      }
      else
      {
        lcd.print("$"); // Display Dollar sign
      }
      lcd.print(price);
    }
    else
    {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("HTTP Error:");
      lcd.setCursor(0, 1);
      lcd.print(httpResponseCode);
    }

    // Free resources
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Disconnected");
  }
}

void updateVisualCountdown()
{
  // Iterate over the last 5 cells of the LCD
  for (int i = 0; i < 5; i++)
  {
    lcd.setCursor(X_CELLS - 5 + i, 0); // Display in the last 5 cells

    if (i < countdownProgress)
    {
      // If seconds have passed within the current 5-second interval
      if (i < currentIntervalStep)
      {
        // If the 5-second block has passed for this cell and it needs an underline
        lcd.write(byte(FILLED_SQUARE_BOTTOM_FILLED_CHAR)); // Filled with underline
      }
      else
      {
        // Only fill without underline
        lcd.write(byte(FILLED_SQUARE_CHAR)); // Filled square for elapsed time
      }
    }
    else
    {
      // If seconds have not yet passed within the current 5-second interval
      if (i < currentIntervalStep)
      {
        // If the 5-second block has passed but within the 25-second interval
        lcd.write(byte(EMPTY_SQUARE_BOTTOM_FILLED_CHAR)); // Empty with underline
      }
      else
      {
        // No time has passed and no underline is needed
        lcd.write(byte(EMPTY_SQUARE_CHAR)); // Empty square
      }
    }
  }
}