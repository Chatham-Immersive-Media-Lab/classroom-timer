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

void setup() {
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  // Play each note in the melody
  for (int i = 0; i < sizeof(melody) / sizeof(melody[0]); i++) {
    int noteDuration = noteDurations[i];
    tone(buzzerPin, melody[i], noteDuration);

    // Pause between notes for a slight gap
    int pauseBetweenNotes = noteDuration * 1.3;
    delay(pauseBetweenNotes);

    // Stop the tone playing on the buzzer
    noTone(buzzerPin);
  }

  // Pause before repeating the melody
  delay(2000);
}
