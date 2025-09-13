// Right before putting this code in the repository I found out about eeprom, I will be adding it at a later time though
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

const byte ROWS = 4;
const byte COLS = 4;

char keypadKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

const byte rowPins[ROWS] = {33, 35, 37, 39};
const byte colPins[COLS] = {41, 43, 45, 47};
Keypad keypad = Keypad(makeKeymap(keypadKeys), rowPins, colPins, ROWS, COLS);

const int pushButtonPin = 3;
int pushButtonState = 0;

enum ReaderState {NotStarted, Started, Choosing, Reading, Settings, ScrollSpeedSettings};
ReaderState currentReaderState = NotStarted;

enum ScrollMode {Manual, AutoScroll};
ScrollMode currentScrollMode = Manual;
int autoScrollDelay = 2000;
const int minScrollDelay = 500;
const int maxScrollDelay = 5000;
const int scrollStep = 50;

int bookColumn = 0;
const byte maxBooks = 5;
char* books[maxBooks] = {"Diary", "History", "Book", "Song", "Note"};
int currentBook = 0;

const char* bookText = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.";
const char* diaryText = "";
const char* historyText = "T";
const char* songText = "";
const char* noteText = "";

LiquidCrystal_I2C lcd(0x27, 16, 2);
int textPosition = 0;
unsigned long lastScrollTime = 0;
bool showWelcomeScreen = true;
unsigned long welcomeScreenTime = 0;

const char* getBookText(int bookIndex) {
  switch(bookIndex) {
    case 0: return diaryText;
    case 1: return historyText;
    case 2: return bookText;
    case 3: return songText;
    case 4: return noteText;
    default: return "";
  }
}

bool hasBookContent(int bookIndex) {
  const char* text = getBookText(bookIndex);
  return text != NULL && strlen(text) > 0;
}

void showScrollSpeedScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scroll Speed:");
  
  lcd.setCursor(0, 1);
  lcd.print(autoScrollDelay / 1000.0, 1);
  lcd.print("s ");
}

void handleScrollSpeedSettings() {
  char keypadButton = keypad.getKey();
  
  if (keypadButton == 'B') {
    autoScrollDelay -= scrollStep;
    if (autoScrollDelay < minScrollDelay) {
      autoScrollDelay = minScrollDelay;
    }
    showScrollSpeedScreen();
  }
  else if (keypadButton == 'C') {
    autoScrollDelay += scrollStep;
    if (autoScrollDelay > maxScrollDelay) {
      autoScrollDelay = maxScrollDelay;
    }
    showScrollSpeedScreen();
  }
  else if (keypadButton == 'A') {
    currentReaderState = Settings;
    showSettingsScreen();
  }
}

void showSettingsScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Settings:");
  
  lcd.setCursor(0, 1);
  if (currentScrollMode == Manual) {
    lcd.print(">Manual Scroll");
  } else {
    lcd.print(">Auto Scroll");
  }
}

void handleSettings() {
  char keypadButton = keypad.getKey();
  
  if (keypadButton == 'B' || keypadButton == 'C') {
    currentScrollMode = (currentScrollMode == Manual) ? AutoScroll : Manual;
    showSettingsScreen();
  }
  else if (keypadButton == 'A') {
    if (currentScrollMode == AutoScroll) {
      currentReaderState = ScrollSpeedSettings;
      showScrollSpeedScreen();
    } else {
      currentReaderState = Reading;
      lcd.clear();
    }
  }
  else if (keypadButton == 'D') {
    currentReaderState = Reading;
    lcd.clear();
  }
}

void readCurrentBook() {
  char keypadButton = keypad.getKey();
  
  if (!hasBookContent(currentBook)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No content");
    lcd.setCursor(0, 1);
    lcd.print("available");
    return;
  }
  
  const char* currentText = getBookText(currentBook);
  int currentTextLength = strlen(currentText);
  
  if (showWelcomeScreen) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reading:");
    lcd.setCursor(0, 1);
    lcd.print(books[currentBook]);
    
    if (millis() - welcomeScreenTime > 2000) {
      showWelcomeScreen = false;
      lcd.clear();
      lastScrollTime = millis();
    }
    return;
  }

  if (keypadButton == 'A') {
    currentReaderState = Settings;
    showSettingsScreen();
    return;
  }
  if (keypadButton == '1') {
    textPosition += 32;
    if (textPosition >= currentTextLength) {
      textPosition = 0;
    }
    lcd.clear();
    lastScrollTime = millis();
  }
  
  if (currentScrollMode == AutoScroll) {
    unsigned long currentTime = millis();
    if (currentTime - lastScrollTime > autoScrollDelay) {
      textPosition += 32;
      if (textPosition >= currentTextLength) {
        textPosition = 0;
      }
      lcd.clear();
      lastScrollTime = currentTime;
    }
  }
  int charsToDisplay = min(32, currentTextLength - textPosition);
  for (int i = 0; i < charsToDisplay; i++) {
    if (i == 16) {
      lcd.setCursor(0, 1); 
    }
    lcd.print(currentText[textPosition + i]);
  }
}
void showCurrentBook() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("> ");
  lcd.print(books[currentBook]);
  
  lcd.setCursor(0, 1);
  if (currentBook + 1 < maxBooks) {
    lcd.print("  ");
    lcd.print(books[currentBook + 1]);
  }
}

void chooseBook() {
  char keypadButton = keypad.getKey();
  
  if (keypadButton == 'C') {
    if (currentBook < maxBooks - 1) {
      currentBook++;
    } else {
      currentBook = 0;
    }
    showCurrentBook();
  } 
  else if (keypadButton == 'B') {
    if (currentBook > 0) {
      currentBook--;
    } else {
      currentBook = maxBooks - 1;
    }
    showCurrentBook();
  }
  
  pushButtonState = digitalRead(pushButtonPin);
  if (pushButtonState == LOW) {
    currentReaderState = Reading;
    textPosition = 0;
    showWelcomeScreen = true;
    welcomeScreenTime = millis();
  }
}

void pressToStart() {
  pushButtonState = digitalRead(pushButtonPin);
  
  switch (currentReaderState) {
    case NotStarted:
      if (pushButtonState == LOW) {
        lcd.clear();
        currentReaderState = Started;
        showCurrentBook();
        delay(100);
      }
      break;
      
    case Started:
      chooseBook();
      break;
      
    case Reading:
      readCurrentBook();
      
      pushButtonState = digitalRead(pushButtonPin);
      if (pushButtonState == LOW) {
        currentReaderState = Started;
        lcd.clear();
        showWelcomeScreen = true;
        showCurrentBook();
      }
      break;
      
    case Settings:
      handleSettings();
      break;
      
    case ScrollSpeedSettings:
      handleScrollSpeedSettings();
      break;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(pushButtonPin, INPUT_PULLUP);
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.print("Press to start");
}

void loop() {
  pressToStart();
  delay(50);
}
