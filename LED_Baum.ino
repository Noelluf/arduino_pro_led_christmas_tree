// Define pins
const int buttonPin = 3;      // Push button pin
const int potPin = A0;        // Potentiometer pin
const int warmWhitePin = 9;   // Warm white LED pin (PWM)
const int coolWhitePin = 10;  // Cool white LED pin (PWM)

// Variables for button state
bool mode = false;  // false: dimming mode, true: CCT mode
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Default startup values
const float defaultCCTRatio = 1.0;  // Only warm white (100%)
const int defaultBrightness = 85;   // 33% brightness

// Variables to store CCT values
int warmWhiteValue;
int coolWhiteValue;
float cctRatio = defaultCCTRatio;  
int brightness = defaultBrightness;  
int lastPotValue = -1;  
const int potThreshold = 10;  
bool potInitialized = false;  // Ignore pot until first change

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(warmWhitePin, OUTPUT);
  pinMode(coolWhitePin, OUTPUT);
  Serial.begin(9600);

  // Set default startup values
  warmWhiteValue = brightness * cctRatio;
  coolWhiteValue = brightness * (1 - cctRatio);

  // Apply default LED settings before startup sequence
  analogWrite(warmWhitePin, warmWhiteValue);
  analogWrite(coolWhitePin, coolWhiteValue);

  // Run startup sequence
  startupSequence();

  // Read initial potentiometer value but don't apply it
  lastPotValue = readStablePot(potPin);
}

int readStablePot(int pin) {
  const int samples = 5;
  int sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(5);
  }
  return sum / samples;
}

void startupSequence() {
  Serial.println("Starting up...");

  for (int i = 0; i < 3; i++) {
    analogWrite(warmWhitePin, 50);
    delay(400);
    analogWrite(warmWhitePin, 0);
    delay(400);
  }

  for (int i = 0; i <= 255; i++) {
    analogWrite(warmWhitePin, i);
    delay(5000 / 255);
  }

  // Ensure default values are set after startup
  analogWrite(warmWhitePin, warmWhiteValue);
  analogWrite(coolWhitePin, coolWhiteValue);
  delay(500);
}

void loop() {
  int buttonState = digitalRead(buttonPin);

  if (buttonState != lastButtonState) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (buttonState == LOW) {
        mode = !mode;
        Serial.print("Mode changed to: ");
        Serial.println(mode ? "CCT" : "Dimming");
      }
      lastDebounceTime = millis();
    }
  }
  lastButtonState = buttonState;

  int potValue = readStablePot(potPin);

  // Ignore potentiometer changes until a significant difference is detected
  if (!potInitialized) {
    if (abs(potValue - lastPotValue) > potThreshold) {
      potInitialized = true;  // Now allow changes
      Serial.println("Potentiometer initialized.");
    } else {
      return;  // Ignore loop execution until first real change
    }
  }

  if (abs(potValue - lastPotValue) > potThreshold) {
    lastPotValue = potValue;

    if (mode) {
      float newWarm = map(potValue, 0, 1023, 255, 0);
      float newCool = map(potValue, 0, 1023, 0, 255);

      cctRatio = newWarm / (newWarm + newCool + 1);

      warmWhiteValue = brightness * cctRatio;
      coolWhiteValue = brightness * (1 - cctRatio);

      analogWrite(warmWhitePin, warmWhiteValue);
      analogWrite(coolWhitePin, coolWhiteValue);

      Serial.print("CCT Mode - Warm: ");
      Serial.print(warmWhiteValue);
      Serial.print(" Cool: ");
      Serial.println(coolWhiteValue);
    } else {
      brightness = map(potValue, 0, 1023, 0, 255);

      warmWhiteValue = brightness * cctRatio;
      coolWhiteValue = brightness * (1 - cctRatio);

      analogWrite(warmWhitePin, warmWhiteValue);
      analogWrite(coolWhitePin, coolWhiteValue);

      Serial.print("Dimming Mode - Brightness: ");
      Serial.print(brightness);
      Serial.print(" Warm: ");
      Serial.print(warmWhiteValue);
      Serial.print(" Cool: ");
      Serial.println(coolWhiteValue);
    }
  }
}
