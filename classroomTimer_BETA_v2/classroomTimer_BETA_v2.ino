#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Encoder.h>

// LED Matrix configuration
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8  // 8 panels of 8x8 LEDs
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 10

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Rotary Encoder Pins
#define ROT_CLK 2
#define ROT_DT 3
#define ROT_SW 4
#define DEBOUNCE_DELAY 2  // Adjust this value as needed
Encoder myEnc(ROT_CLK, ROT_DT);
long oldPosition  = -999;

int lastStateCLK;
int currentStateCLK;
unsigned long lastButtonPress = 0;
int selectedTimeIndex = 0;
unsigned long lastDebounceTime = 0;

const unsigned long timeOptions[] = {
  5, 10, 30, 60, 120, 150, 180, 210, 240, 270, 300, 330, 360,
  390, 420, 450, 480, 510, 540, 570, 600, 630, 660, 690, 720,
  900, 1200, 1500, 1800, 2100, 2400, 2700, 3000, 3300, 3600, 5400, 7200
}; 
const int numOptions = sizeof(timeOptions) / sizeof(timeOptions[0]);

unsigned long startTime = 0;
bool timerRunning = false;

#define CHAR_SPACING  1
char message[10];

bool animationPlaying = false;
unsigned long animationStartTime;

void setup() {
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY / 2);
  mx.clear();

  pinMode(ROT_CLK, INPUT);
  pinMode(ROT_DT, INPUT);
  pinMode(ROT_SW, INPUT_PULLUP);
  lastStateCLK = digitalRead(ROT_CLK);

  Serial.begin(9600);
  displayTimeOption();
}




void loop() {
   long newPosition = myEnc.read() / 4;  // Dividing by 4 helps smooth out readings
  if (newPosition != oldPosition) {
    oldPosition = newPosition;

    if (newPosition > oldPosition) {
      selectedTimeIndex++;
      Serial.println("Clockwise");
    } else {
      selectedTimeIndex--;
      Serial.println("Counter-clockwise");
    }

    // Constrain index to valid range
    selectedTimeIndex = constrain(selectedTimeIndex, 0, numOptions - 1);
    displayTimeOption();
  }


  
  int clkState = digitalRead(ROT_CLK);
  unsigned long currentTime = millis();

  // Check if enough time has passed to consider it a valid change
  if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (clkState != lastStateCLK) {
      // Read the DT pin to determine rotation direction
      int dtState = digitalRead(ROT_DT);
      if (clkState == HIGH) {
        if (dtState != clkState) {
          // Counter-clockwise rotation
          Serial.println("Counter-clockwise");
          // Adjust your value down
        } else {
          // Clockwise rotation
          Serial.println("Clockwise");
          // Adjust your value up
        }
      }
      lastDebounceTime = currentTime;
    }
  }
  lastStateCLK = clkState;




  if (!timerRunning) {
    if (animationPlaying) {
      runAnimation();
    } else {
      handleEncoder();
      if (digitalRead(ROT_SW) == LOW && (millis() - lastButtonPress > 50)) {
        lastButtonPress = millis();
        startTimer();
      }
    }
  } else {
    updateLoadingBar();
  }
}

void handleEncoder() {
  int newCLK = digitalRead(ROT_CLK);
  int newDT = digitalRead(ROT_DT);

  if (newCLK != lastStateCLK) {
    if (newCLK == LOW && newDT == HIGH) {
      selectedTimeIndex++;
      Serial.println("Clockwise");
    } else if (newCLK == LOW && newDT == LOW) {
      selectedTimeIndex--;
      Serial.println("Counter-clockwise");
    }

    // Constrain the index within bounds
    selectedTimeIndex = constrain(selectedTimeIndex, 0, numOptions - 1);
    displayTimeOption();
  }

  lastStateCLK = newCLK;
}

void displayTimeOption() {
  unsigned long selectedTime = timeOptions[selectedTimeIndex];
  int minutes = selectedTime / 60;
  int seconds = selectedTime % 60;

  snprintf(message, sizeof(message), "%02d:%02d", minutes, seconds);
  mx.clear();

  int textLength = strlen(message) * 6;  // 6 pixels per character
  int startingColumn = (64 - textLength) / 2;  // Center horizontally
  int verticalOffset = 0;  // Top-aligned vertically

  printText(startingColumn, verticalOffset, message);
}

void startTimer() {
  timerRunning = true;
  startTime = millis();
  mx.clear();
  Serial.println("Timer started!");
}

void updateLoadingBar() {
  unsigned long elapsedTime = (millis() - startTime) / 1000;
  unsigned long totalTime = timeOptions[selectedTimeIndex];

  float progress = (float)elapsedTime / totalTime;
  int columnsToFill = progress * 64;  // Total width is 64 pixels

  for (int col = 0; col < columnsToFill; col++) {
    for (int row = 0; row < 8; row++) {
      mx.setPoint(row, col, true);
    }
  }

  if (elapsedTime >= totalTime) {
    timerRunning = false;
    animationPlaying = true;
    animationStartTime = millis();
    mx.clear();
    Serial.println("Timer finished!");
  }
}

void runAnimation() {
  if (millis() - animationStartTime < 3000) {
    int t = millis() / 100;
    int col = abs((t % 128) - 64);  // Bounce within the 64-pixel width

    mx.clear();
    for (int row = 0; row < 8; row++) {
      mx.setPoint(row, col, true);
    }
  } else {
    animationPlaying = false;
    mx.clear();
  }
}

void printText(int startCol, int verticalOffset, char *pMsg) {
  uint8_t cBuf[8];
  int16_t col = startCol;

  mx.control(0, MAX_DEVICES - 1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  while (*pMsg != '\0') {
    int showLen = mx.getChar(*pMsg++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
    for (int i = 0; i < showLen; i++) {
      for (int row = 0; row < 8; row++) {
        bool pixelOn = bitRead(cBuf[i], row);
        // Adjust row and column for correct orientation
        mx.setPoint(7 - row, col, pixelOn);  // Flip vertically
      }
      col++;
    }
    col += CHAR_SPACING;
  }

  mx.control(0, MAX_DEVICES - 1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}
