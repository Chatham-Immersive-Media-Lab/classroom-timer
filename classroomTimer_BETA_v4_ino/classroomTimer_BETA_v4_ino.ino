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

const int buzzerPin = 8; // Pin connected to the active buzzer

// Notes and their frequencies for the intro lick of "Jerk It Out"
#define NOTE_Gsharp 830
#define NOTE_Fsharp 740
#define NOTE_Asharp 932
#define NOTE_E 668
#define NOTE_B 987

// Melody and corresponding note durations (in ms)
int melody[] = { NOTE_Gsharp, NOTE_B, NOTE_Gsharp, NOTE_B, NOTE_B, NOTE_Gsharp, NOTE_Fsharp, NOTE_Asharp, NOTE_Fsharp, NOTE_E };
int noteDurations[] = { 300, 150, 150, 150, 150, 300, 300, 150, 150, 400 };

//TEST EACH NOTE
// int melody[] = { NOTE_Gsharp, NOTE_Fsharp, NOTE_Asharp, NOTE_E, NOTE_B };
// int noteDurations[] = { 250, 250, 250, 250, 300 };

//TEST E
//int melody[] = { NOTE_E, NOTE_B };
//int noteDurations[] = { 500, 500 };


// List of random phrases to display
const char* phrases[] = {
  "Where are we",
  "IMM is lit",
  "Hunter can you unlock the storage closet",
  "The coffee machine is broken",
  "It's time to take a break",
  "Who's turn is it to make lunch?",
  "Can we get more snacks?",
  "Have you seen my notebook?",
  "Lunch at 12:30?"
};
const int numPhrases = sizeof(phrases) / sizeof(phrases[0]);  // Number of phrases

unsigned long lastScrollTime = 0;
int scrollDelay = 100;  // Adjust delay for scrolling speed
int scrollPosition = 64;  // Start with the phrase off the screen



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
bool melodyPlayed = false;  // Flag to check if melody has played

#define CHAR_SPACING  1
char message[10];

bool animationPlaying = false;
unsigned long animationStartTime;

void setup() {
  pinMode(buzzerPin, OUTPUT);
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
      runAnimation();  // Run the animation when it's playing
    } else if (millis() - lastActivityTime > idleTimeout) {
      playIdleAnimation();  // Trigger the idle animation after timeout
    } else {
      handleButtons();  // Handle button presses if there's activity
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
  melodyPlayed = false;  // Reset the flag to allow melody playback
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

    // Play melody once the timer ends
    if (!melodyPlayed) {
      playMelody();
      melodyPlayed = true;  // Set flag to prevent replaying melody
    }
  }
}

void playIdleAnimation() {
  if (!animationPlaying) {
    animationPlaying = true;
    animationStartTime = millis();  // Initialize the start time for animation
  }

  // Get a random phrase from the list
  int randomIndex = random(numPhrases);
  const char* phrase = phrases[randomIndex];

  unsigned long currentMillis = millis();
  
  // Scroll the text every few milliseconds
  if (currentMillis - lastScrollTime >= scrollDelay) {
    lastScrollTime = currentMillis;
    scrollPosition--;  // Move the text to the left

    if (scrollPosition < -(strlen(phrase) * 6)) {  // If the text has completely scrolled off screen
      scrollPosition = 64;  // Reset to the start position
    }
  }

  // Display the phrase
  mx.clear();  // Clear the display before drawing the new frame
  int x = scrollPosition;  // Starting column for the phrase

  for (int i = 0; phrase[i] != '\0'; i++) {
    uint8_t cBuf[8];
    mx.getChar(phrase[i], sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
    for (int j = 0; j < 5; j++) {  // Each character is 5 columns wide
      for (int row = 0; row < 8; row++) {
        bool pixelOn = bitRead(cBuf[j], row);
        mx.setPoint(row, x + j, pixelOn);
      }
    }
    x += 6;  // Move over by 6 columns for the next character (5 for the char + 1 for spacing)
  }
}




void runAnimation() {
  unsigned long elapsedTime = millis() - animationStartTime;
  if (elapsedTime < 18000) {
    // Continue the animation for 10 seconds
    int elapsed = elapsedTime % 3200;  // Loop every 800 ms
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


void playMelody() {
  for (int i = 0; i < sizeof(melody) / sizeof(melody[0]); i++) {
    int noteDuration = noteDurations[i];
    tone(buzzerPin, melody[i], noteDuration);

    // Pause between notes for a slight gap
    int pauseBetweenNotes = noteDuration * 1.3;
    delay(pauseBetweenNotes);

    // Stop the tone playing on the buzzer
    noTone(buzzerPin);
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
