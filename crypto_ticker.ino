#include <WiFi.h>
#include <LiquidCrystal.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cstring>
#include "config.h"

const char *cryptoSymbols[] = {"BTC", "ETH", "SOL", "AVAX", "DOT", "ADA", "DOGE"};
const int numCryptos = sizeof(cryptoSymbols) / sizeof(cryptoSymbols[0]);
// GBP or USD supported
const char *FIAT_SYMBOL = "USD";

#define POUND_CHAR
#define EMPTY_SQUARE_CHAR 2
#define FILLED_SQUARE_CHAR 3
#define EMPTY_SQUARE_BOTTOM_FILLED_CHAR 4
#define FILLED_SQUARE_BOTTOM_FILLED_CHAR 5
#define BUTTON_PIN 33
#define X_CELLS 16
#define Y_CELLS 2

LiquidCrystal lcd(13, 12, 25, 26, 27, 14); // RS, E, D4, D5, D6, D7

bool buttonPressed = false;
bool isTickerRunning = false;
int currentCryptoIndex = 0;

unsigned long previousMillis = 0;        // last time API was polled
unsigned long countdownMillis = 0;       // last time the countdown was updated
const long pollInterval = 5000;          // poll api
const long countdownInterval = 1000;     // interval for loading stepper
const long cryptoSwitchInterval = 25000; // interval to switch crypto, move to next
int countdownProgress = 0;               // progress of 5-second countdown
int currentIntervalStep = 0;             // the number of 5-second intervals (0 to 4)

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

void setup()
{
  Serial.begin(115200); // baud rate

  lcd.begin(X_CELLS, Y_CELLS);
  lcd.createChar(POUND_CHAR, poundChar);
  lcd.createChar(EMPTY_SQUARE_CHAR, emptySquare);
  lcd.createChar(FILLED_SQUARE_CHAR, filledSquare);
  lcd.createChar(EMPTY_SQUARE_BOTTOM_FILLED_CHAR, emptySquareBottomFilled);
  lcd.createChar(FILLED_SQUARE_BOTTOM_FILLED_CHAR, filledSquareBottomFilled);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  lcd.setCursor(0, 0);
  lcd.print("Setting up...");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    lcd.setCursor(0, 1);
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

  // button is pressed
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    if (!buttonPressed)
    {
      buttonPressed = true;
      Serial.println("Button pressed!");

      if (!isTickerRunning)
      {
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
    if (currentMillis - previousMillis >= pollInterval)
    {
      previousMillis = currentMillis;
      pollAPI();
      countdownProgress = 0;
      currentIntervalStep++;
      updateVisualCountdown();
    }
    if (currentMillis - countdownMillis >= countdownInterval)
    {
      countdownMillis = currentMillis;
      countdownProgress++;
      updateVisualCountdown();
    }
    if (currentIntervalStep >= (cryptoSwitchInterval / pollInterval))
    {
      currentIntervalStep = 0;
      countdownProgress = 0;
      lcd.clear();
      currentCryptoIndex = (currentCryptoIndex + 1) % numCryptos;
      lcd.setCursor(0, 0);
      lcd.print(cryptoSymbols[currentCryptoIndex]);
      pollAPI();
    }
  }

  // delay to prevent a busy-wait loop
  delay(100);
}

void pollAPI()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String serverName = "https://api.pro.coinbase.com/products/";
    serverName += cryptoSymbols[currentCryptoIndex];
    serverName += "-";
    serverName += FIAT_SYMBOL;
    serverName += "/ticker";
    http.begin(serverName);

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
      String payload = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(payload);
      StaticJsonDocument<200> doc;
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

      float price = doc["price"];

      Serial.print(cryptoSymbols[currentCryptoIndex]);
      Serial.print("/");
      Serial.print(FIAT_SYMBOL);
      Serial.print(" Price: ");
      Serial.println(price);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(cryptoSymbols[currentCryptoIndex]);
      lcd.print("/");
      lcd.print(FIAT_SYMBOL);
      lcd.print(":");
      lcd.setCursor(0, 1);
      if (strcmp(FIAT_SYMBOL, "GBP") == 0)
      {
        lcd.write(byte(POUND_CHAR));
      }
      else
      {
        lcd.print("$");
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