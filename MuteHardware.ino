#include <Keyboard.h>

/*
  Red and green LEDs controlled via serial input - 1 for red, 2 for green, any other int for off.
  Sends keyboard command to toggle mute status on button press.
*/

// LED setup:
const int redPin = 9;
const int greenPin = 10;
const int greenBrightness = 15; // 0-255

// Mute button setup
const int muteButtonPin = 2;

// Time after button press during which status update from computer is ignored
// to avoid flapping due to out of date reported status 
unsigned long buttonPressTime = 0;
unsigned long buttonPressStateUpdateLockout = 1500;

// Keystroke setup to toggle mute state on computer
const char commandKeyStrokes[3] = {KEY_LEFT_GUI, KEY_LEFT_SHIFT, 'a'};

// State variable: 0=off, 1=muted,  2=unmuted
byte state = 0;
bool toggleMute = false;

void setup() {

    // Initialize serial:
    Serial.begin(9600);

    // Pin setup
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(muteButtonPin, INPUT_PULLUP);

    // Keyboard
    Keyboard.begin();

}

void loop() {

    // Ignore button presses if Zoom meeting inactive    
    if (state != 0) {
        buttonHandler();
    }

    serialHandler();

    outputsHandler();

}

void serialHandler() {

    while (Serial.available() > 0) {

        // Status sent as integer
        int statusCode = Serial.parseInt();
     

        // Statuses delimited by newline
        if (
            (Serial.read() == '\n')
            // Drop status updates sent immediately after button press
            && ((millis() - buttonPressTime) > buttonPressStateUpdateLockout)
        ) {
            if (statusCode == 1) {
                state = 1;
            }
            else if (statusCode == 2) {
                state = 2;
            }
            else {
                state = 0;
            }
        }
    }
}

void buttonHandler() {
/*
Monitor and debounce button state, and set state for output
*/

    static int buttonState;
    static int lastButtonState = HIGH;
    static unsigned long lastBounceTime = 0;
    static unsigned long debounceWait = 20;
    static const char newStates[2] = {2,1};

    unsigned long timeNow = millis();

    int reading = digitalRead(muteButtonPin);    
    
    if (reading != lastButtonState) {
        lastBounceTime = timeNow;;        
    }
    lastButtonState = reading;

    if ((timeNow - lastBounceTime) > debounceWait) {
        // Steady enough to allow state to flip
        // Toggle state only if button pressed and Zoom is active
        if (
            (buttonState != reading) 
            && (reading == LOW)
        ) {
            toggleMute = true;
            state = newStates[state - 1];
            buttonPressTime = timeNow; 
        }

        buttonState = reading;
    }
}

void sendMuteKeystrokes() {
/*
Send keystrokes to toggle mute state in Zoom
*/

    static int i;

    for (i = 0; i < sizeof(commandKeyStrokes); i++) {
        Keyboard.press(commandKeyStrokes[i]);
    }

    delay(50);

    Keyboard.releaseAll();

}

void outputsHandler() {
/*
Set outputs based on current state
*/

    if (state == 1) {
        digitalWrite(redPin, HIGH);
        digitalWrite(greenPin, LOW);
    }
    else if (state == 2) {
        digitalWrite(redPin, LOW);
        // Green LED is vastly brighter than red, so PWM it down
        analogWrite(greenPin, greenBrightness);
    }
    else {
        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, LOW);
    }

    if (toggleMute) {
        sendMuteKeystrokes();
        toggleMute = false;
    }
}
