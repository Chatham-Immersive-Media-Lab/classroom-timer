#include <MD_MAX72xx.h>
#include <SPI.h>

// LED Matrix configuration
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8  // 8 panels of 8x8 LEDs
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 10

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Button Pins
#define BTN_UP 2          // Button for increasing value
#define BTN_DOWN 3        // Button for decreasing value
#define BTN_SELECT 4      // Button for selecting

const unsigned long holdTimeUp = 200;       // Hold time for UP button in ms
const unsigned long holdTimeDown = 200;     // Hold time for DOWN button in ms
const unsigned long holdTimeSelect = 100;   // Hold time for SELECT button in ms

int selectedTimeIndex = 0;    // Tracks selected option index
const unsigned long debounceDelay = 50;  // Debounce time in ms
const unsigned long idleTimeout = 20000; // 20 seconds for idle timeout

unsigned long lastButtonPressUp = 0;
unsigned long lastButtonPressDown = 0;
unsigned long lastButtonPressSelect = 0;
unsigned long lastActivityTime = 0;

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

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  Serial.begin(9600);
  displayTimeOption();
  lastActivityTime = millis(); // Start tracking activity
}

void loop() {
  if (!timerRunning) {
    if (animationPlaying) {
      runAnimation();
    } else if (millis() - lastActivityTime > idleTimeout) {
      playIdleAnimation(); // Start idle animation if idle
    } else {
      handleButtons();
    }
  } else {
    updateLoadingBar();
  }
}

void handleButtons() {
  if (animationPlaying) {
    animationPlaying = false;  // Stop animation if a button is pressed
    mx.clear();
  }

  unsigned long currentTime = millis();

  // Check BTN_UP for increment after hold time
  if (digitalRead(BTN_UP) == LOW) {
    if (currentTime - lastButtonPressUp >= holdTimeUp) { 
      lastButtonPressUp = currentTime;   // Reset to avoid multiple triggers on hold
      selectedTimeIndex++;
      selectedTimeIndex = constrain(selectedTimeIndex, 0, numOptions - 1);
      displayTimeOption();
      Serial.println("Button Up Held - Increment");
      lastActivityTime = currentTime;  // Reset idle timer
    }
  } else {
    lastButtonPressUp = currentTime;   // Reset if not held
  }

  // Check BTN_DOWN for decrement after hold time
  if (digitalRead(BTN_DOWN) == LOW) {
    if (currentTime - lastButtonPressDown >= holdTimeDown) {
      lastButtonPressDown = currentTime;
      selectedTimeIndex--;
      selectedTimeIndex = constrain(selectedTimeIndex, 0, numOptions - 1);
      displayTimeOption();
      Serial.println("Button Down Held - Decrement");
      lastActivityTime = currentTime;  // Reset idle timer
    }
  } else {
    lastButtonPressDown = currentTime;
  }

  // Check BTN_SELECT for starting the timer after hold time
  if (digitalRead(BTN_SELECT) == LOW) {
    if (currentTime - lastButtonPressSelect >= holdTimeSelect) {
      lastButtonPressSelect = currentTime;
      startTimer();
      Serial.println("Button Select Held - Timer Start");
      lastActivityTime = currentTime;  // Reset idle timer
    }
  } else {
    lastButtonPressSelect = currentTime;
  }
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
    selectedTimeIndex = 0; // Reset to first time option
    displayTimeOption();   // Show the reset time option
    Serial.println("Timer finished and reset!");
    lastActivityTime = millis();
  }
}

void playIdleAnimation() {
  if (!animationPlaying) {
    animationPlaying = true;
    animationStartTime = millis();  // Initialize the start time for animation
  }

  int t = (millis() - animationStartTime) / 200;  // Use elapsed time for smoother effect
  int col = abs((t % 256) - 128);  // Bounce within the 128-pixel width (since 8 panels = 64 columns * 2)

  mx.clear();
  
  // Loop through all 8 rows and set the appropriate point
  for (int row = 0; row < 8; row++) {
    mx.setPoint(row, col % 64, true);  // Modulo 64 ensures the column wraps around after 64
  }
}

void runAnimation() {
  unsigned long elapsedTime = millis() - animationStartTime;
  if (elapsedTime < 3000) {
    // Continue the animation for 10 seconds
    int elapsed = elapsedTime % 800;  // Loop every 800 ms
    int radius = (elapsed / 100);  // Expand radius smoothly in 8 steps (0 to 7)
    
    mx.clear();
    
    // Draw a circle expanding from the center across all 8 panels
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 64; x++) {  // 64 columns to span all 8 panels
        int dx = abs(x - 32);  // Distance from the center (32 is the middle column)
        int dy = abs(y - 3);   // Distance from the center vertically (3 is the middle row)
        int dist = max(dx, dy); // Use max to create a square-like radius

        // Set pixels within the current radius
        if (dist == radius) {
          mx.setPoint(y, x, true);
        }
      }
    }
  } else {
    // Animation has finished, reset the state
    animationPlaying = false;
    mx.clear();
    lastActivityTime = millis();
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
        mx.setPoint(7 - row, col, pixelOn);
      }
      col++;
    }
    col += CHAR_SPACING;
  }

  mx.control(0, MAX_DEVICES - 1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}
