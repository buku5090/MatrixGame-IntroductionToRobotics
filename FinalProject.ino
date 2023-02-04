#include "LedControl.h" // need the library
#include <LiquidCrystal.h>
#include <Arduino.h>
#include <EEPROM.h>

// Initializing main menu
int rows = 7;
int cols = 16;
int currRow = 1;
bool movedCursor = false; 
char **mainMenu;

const int buzzerPin = A4;
int buzzerTone = 1000;

// Initializing the subcategories
char **settingsMenu;
int currRowSettings = 1;
int pBari = 0;
int auxValue = -1;

// Game variables
int diff = 8;
long int score = 0;
int newHead[2] = {0, 0};
bool isGameEnded = false;
float timeEndScreen;

//Define The Snake as a Struct
typedef struct Snake Snake;
struct Snake{
  int head[2];     // the (row, column) of the snake head
  int body[40][2]; //An array that contains the (row, column) coordinates
  int len;         //The length of the snake 
  int dir[2];      //A direction to move the snake along
};

//Define The Apple as a Struct
typedef struct Apple Apple;
struct Apple{
  int rPos; //The row index of the apple
  int cPos; //The column index of the apple
};

byte pic[8] = {0,0,0,0,0,0,0,0};//The 8 rows of the LED Matrix

Snake snake = {{1,5},{{0,5}, {1,5}}, 2, {1,0}};//Initialize a snake object
Apple apple = {(int)random(0,8),(int)random(0,8)};//Initialize an apple object

//Variables To Handle The Game Time
float oldTime = 0;
float timer = 0;
float updateRate = 3;

int i,j;//Counters

// EEPROM variables
int currPosEEPROM = 0;
int MatrixBrightnessAddressEEPROM = 0;


int stateOfTheMenu = -1;
// -1 - welcome message
//  0 - main menu
//  1 - game
//  2 - highscore
//  3 - settings
//  4 - about
//  5 - how to play

int buttonState; 
int lastButtonState = LOW;
int lcdBrightValue = 255;

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50; 

const int lcdBright = A2;
const int dinPin = 12;
const int clockPin = 11;
const int loadPin = 10;
const int varXPin = A0;
const int varYPin = A1;
const int buttonPin = 2;

volatile int playerPos = 0;
volatile int playerScore = 0;
volatile int currFoodPosX = 0;
volatile int currFoodPosY = 0;
volatile int lastFoodPosX = 0;
volatile int lastFoodPosY = 0;
volatile int newFoodPosX = 0;
volatile int newFoodPosY = 0;

volatile byte readingButton = LOW;
int xValue = 0;
int yValue = 0;

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1); // DIN, CLK, LOAD, No. DRIVER
byte matrixBrightness = 2;
byte xPlayerPos = 0;

byte yPlayerPos = 0;
byte xLastPos = 0;
byte yLastPos = 0;
const int minThreshold = 200;
const int maxThreshold = 600;
const byte moveInterval = 100;
unsigned long long lastMoved = 0;
const byte matrixSize = 8;
bool matrixChanged = true;

const byte RS = 9;
const byte enable = 8;
const byte d4 = 7;
const byte d5 = 6;
const byte d6 = 5;
const byte d7 = 4;

LiquidCrystal lcd(RS, enable, d4, d5, d6, d7);

bool isAtGreeting = 1;
volatile bool startGame = 0;
byte matrix[matrixSize][matrixSize] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};
byte matrixByte[matrixSize] = {
  B00000000,
  B01000100,
  B00101000,
  B00010000,
  B00010000,
  B00010000,
  B00000000,
  B00000000
};

void setup() {
  pinMode(buzzerPin, OUTPUT); 

  mainMenu = new char*[rows];
  for (int i = 0; i < rows; i++) {
    mainMenu[i] = new char[cols];
  }

  settingsMenu = new char*[rows];
  for (int i = 0; i < rows; i++) {
    settingsMenu[i] = new char[cols];
  }

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  Serial.begin(9600);

  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(lcdBright, OUTPUT);

  // the zero refers to the MAX7219 number, it is zero for 1 chip
  lc.shutdown(0, false); // turn off power saving, enables display
  lc.setIntensity(0, matrixBrightness); // sets brightness (0~15 possible values)
  lc.clearDisplay(0);// clear screen

  matrix[xPlayerPos][yPlayerPos] = 1;
}

void loop() {
  analogWrite(lcdBright, lcdBrightValue); 

  // read the input
  readInput();
  
  // choose an option from menu
  stateOfMenu();
}

void readInput() {
  readingButton = !digitalRead(buttonPin);
  xValue = analogRead(varXPin);
  yValue = analogRead(varYPin);
}

void stateOfMenu() {
  switch (stateOfTheMenu) {
    case -1: // welcome message - displayed for 2s
      startLCD();

      changeState(0);

      break;
    case 0: // the main menu
      moveCursorMainMenu();
      
      changeState(currRow);
      currRowSettings = 1;

      break;

    case 1: // start the game
      startTheGame();

      changeState(0);

      break;

    case 2: // highscore
      showHighscore();

      changeState(0);

      break;

    case 3: // settings
      settings();

      if (currRowSettings < 6 and currRowSettings > 1) {
        changeState(currRowSettings + 4);
      }

      if (currRowSettings == 1) {
        changeState(0);
      }
      
      break;

    case 4: // about
      about();

      changeState(0);

      break;

    case 5: // how to play
      howToPlay();
      
      changeState(0);

      break;

    case 6: // difficulty
      difficulty();

      changeState(3);

      break;

    case 7: // lcd brightness
      LCDBrightness();

      changeState(3);

      break;

    case 8: // mat brightness
      matBrightness();

      if (matrixBrightness != auxValue) { // if the values are not equal
        EEPROM.put(MatrixBrightnessAddressEEPROM, matrixBrightness);
      }

      EEPROM.get(MatrixBrightnessAddressEEPROM, auxValue);

      changeState(3);

      break;

    case 9: // sound
      soundVolume();

      changeState(3);

      break;

    case 10: // back to menu
      //returnToMainMenu();

      // changeState(3);

      break;

    default:
      // statements
      break;
  }
}

void difficulty() {
  lcd.setCursor(0, 0);
  lcd.print("Difficulty");  

  lcd.setCursor(0, 1);
  lcd.print(diff);

  if (xValue < minThreshold && movedCursor == false) {
    tone(buzzerPin, 500, 50);
    movedCursor = true;
    if (diff < cols) {
      diff = diff + 2;  
      lcd.clear();
    } 
  }

  if (xValue > maxThreshold && movedCursor == false) {
    movedCursor = true;
    tone(buzzerPin, 500, 50);
    if (diff > 3) {
      diff = diff - 2; 
      lcd.clear();
    } 
  }

  if (xValue < maxThreshold && xValue > minThreshold) {
    movedCursor = false;
  }
}

void LCDBrightness() {
  lcd.setCursor(0, 0);
  lcd.print("LCD - ");
  lcd.setCursor(6, 0);
  lcd.print(lcdBrightValue); 

  if (xValue < minThreshold && movedCursor == false) {
    movedCursor = true;
    tone(buzzerPin, 500, 50);
    if (lcdBrightValue < 255) {
      lcdBrightValue = lcdBrightValue + 50; 
      lcd.clear();
    } 
  }

  if (xValue > maxThreshold && movedCursor == false) {
    movedCursor = true;
    tone(buzzerPin, 500, 50);
    if (lcdBrightValue > 60) {
      lcdBrightValue = lcdBrightValue - 50; 
      lcd.clear();
    } 
  }

  if (xValue < maxThreshold && xValue > minThreshold) {
    movedCursor = false;
  }

  pBari=map(lcdBrightValue, 0, 255, 0, 17);

  for (int i=0; i < pBari; i++) {
    lcd.setCursor(i, 1);   
    lcd.write(byte(0));  
  }
}

void matBrightness() {
  lcd.setCursor(0, 0);
  lcd.print("Matrix - ");
  lcd.setCursor(9, 0);
  lcd.print(matrixBrightness);

  if (xValue < minThreshold && movedCursor == false) {
    movedCursor = true;
    tone(buzzerPin, 500, 50);
    if (matrixBrightness < 15) {
      matrixBrightness = matrixBrightness + 1; 
      lcd.clear();
    } 
  }

  if (xValue > maxThreshold && movedCursor == false) {
    movedCursor = true;
    tone(buzzerPin, 500, 50);
    if (matrixBrightness > 2) {
      matrixBrightness = matrixBrightness - 1; 
      lcd.clear();
    } 
  }

  if (xValue < maxThreshold && xValue > minThreshold) {
    movedCursor = false;
  }

  pBari = map(matrixBrightness, 1, 15, 0, 17);

  for (int i=0; i < pBari; i++) {
    lcd.setCursor(i, 1);   
    lcd.write(byte(0));  
  }
}

void soundVolume() {
  lcd.setCursor(0, 0);
  lcd.print("sound"); 
}

void startLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Hello! SnakeGame");

  lcd.setCursor(0, 1);
  lcd.print("Enjoy! By Buki");

  if (millis() > 5000) {
    stateOfTheMenu = 0;
    lcd.clear();
  }
}

void changeState(int state) {
  if (readingButton != lastButtonState) {
        lastDebounceTime = millis();
      }

      if ((millis() - lastDebounceTime) > debounceDelay) {
        if (readingButton != buttonState) {
          buttonState = readingButton;

          if (buttonState == HIGH) {
            stateOfTheMenu = state;
            lcd.clear();
          }
        }
      }

      lastButtonState = readingButton;
}

void howToPlay() {
  lcd.setCursor(0, 0);
  lcd.print("Joystick-Move");

  lcd.setCursor(0, 1);
  lcd.print("Button-Pause");
}

void about() {
  lcd.setCursor(0, 0);
  lcd.print("by Buki");

  lcd.setCursor(0, 1);  
  lcd.print("GitHub: buku5090");
}

void settings() {
  settingsMenu[1] = "<-Main Menu";
  settingsMenu[2] = "a)Difficulty";
  settingsMenu[3] = "b)LCD Brightness";
  settingsMenu[4] = "c)MAT Brightness";
  settingsMenu[5] = "d)Sound";
  settingsMenu[6] = " ";

  lcd.setCursor(0, 0);
  lcd.print(settingsMenu[currRowSettings]);

  lcd.setCursor(15, 0);
  lcd.print("<");
  
  lcd.setCursor(0, 1);
  lcd.print(settingsMenu[currRowSettings + 1]);

  int last_value = 2;  
  moveInMenu(currRowSettings, last_value);  
}

void startTheGame() {
  if (isGameEnded == false) {
    lcd.setCursor(0, 0);
    lcd.print("Enjoy the Game!");

    lcd.setCursor(0, 1);
    lcd.print("Score:");

    lcd.setCursor(7, 1);
    lcd.print(score);
  }

  runGame();
}

void runGame() {
  float deltaTime = calculateDeltaTime();
  timer += deltaTime;

  //Check For Inputs
  int xVal = analogRead(varXPin);
  int yVal = analogRead(varYPin);
  
  if(xVal < 100 && snake.dir[1]==0){
    snake.dir[0] = 0;
    snake.dir[1] = 1;
  }else if(xVal > 920 && snake.dir[1]==0){
    snake.dir[0] = 0;
    snake.dir[1] = -1;
  }else if(yVal< 100 && snake.dir[0]==0){
    snake.dir[0] = 1;
    snake.dir[1] = 0;
  }else if(yVal > 920 && snake.dir[0]==0){
    snake.dir[0] = -1;
    snake.dir[1] = 0;
  }
  
  //Update
  if(timer > 1000/diff){
    timer = 0;
    Update();

    if (score > 0 && snake.len == 2) {
      lcd.clear();

      if (isGameEnded == true) {
        timeEndScreen = millis();
      }

      endGame();
    }
  }
  
  //Render
  Render();
   
}

// function to move in the menu
void moveCursorMainMenu() {
  mainMenu[1] = "1. Start Game";
  mainMenu[2] = "2. Highscore";
  mainMenu[3] = "3. Settings";
  mainMenu[4] = "4. About";
  mainMenu[5] = "5. How to play";
  mainMenu[6] = " ";

  lcd.setCursor(0, 0);
  lcd.print(mainMenu[currRow]);

  lcd.setCursor(15, 0);
  lcd.print("<");
  
  lcd.setCursor(0, 1);
  lcd.print(mainMenu[currRow + 1]);

  int lastValue = 2;
  moveInMenu(currRow, lastValue);
}

void showHighscore() {
  lcd.setCursor(0, 0);
  lcd.print("highscores");
}

void generateFood() {
  // lastFoodPos = currentPos;
  // newFoodPos = random(ceva);
  // matrix[lastFoodPos] = 0;
  // matrix[newFoodPos] = 1;
  lastFoodPosX = currFoodPosX;
  lastFoodPosY = currFoodPosY;

  newFoodPosX = random(7);
  newFoodPosY = random(7);

  currFoodPosX = newFoodPosX;
  currFoodPosY = newFoodPosY;

  matrix[lastFoodPosX][lastFoodPosY] = 0;
  matrix[currFoodPosX][currFoodPosY] = 1;

  matrixChanged = true;
}
void updateByteMatrix() {
  for (int row = 0; row < matrixSize; row++) {
    lc.setRow(0, row, matrixByte[row]);
  }
}

void updateMatrix() {
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, matrix[row][col]);
    }
  }
}

void updatePositions() {
  int xValue = analogRead(varXPin);
  int yValue = analogRead(varYPin);
  xLastPos = xPlayerPos;
  yLastPos = yPlayerPos;

  if (xValue < minThreshold) {
    if (xPlayerPos < matrixSize - 1) {
      xPlayerPos++;
    }
    else {
      xPlayerPos = 0;
    }
  }

  if (xValue > maxThreshold) {
    if (xPlayerPos > 0) {
      xPlayerPos--;
    }
    else {
      xPlayerPos = matrixSize - 1;
    }
  }

  if (yValue > maxThreshold) {
    if (yPlayerPos < matrixSize - 1) {
      yPlayerPos++;
    }
    else {
      yPlayerPos = 0;
    }
  }

  if (yValue < minThreshold) {
    if (yPlayerPos > 0) {
      yPlayerPos--;
    }
    else {
        yPlayerPos = matrixSize - 1;
    }
  }

  if (xPlayerPos != xLastPos || yPlayerPos != yLastPos) {
    matrixChanged = true;
    matrix[xLastPos][yLastPos] = 0;
    matrix[xPlayerPos][yPlayerPos] = 1;
  }
}

void moveInMenu(int &row, int val) {
  if (xValue < minThreshold && movedCursor == false) {
    movedCursor = true;
    Serial.println("up");
    tone(buzzerPin, 500, 50);
    if (row > 1) {
      row = row - 1;  
      lcd.clear();
    } 
  }

  if (xValue > maxThreshold && movedCursor == false) {
    movedCursor = true;
    Serial.println("down");
    tone(buzzerPin, 500, 50);
    if (row < rows - val) {
      row = row + 1;  
      lcd.clear();
    } 
  }

  if (xValue < maxThreshold && xValue > minThreshold) {
    movedCursor = false;
  }
}


float calculateDeltaTime(){
  float currentTime = millis();
  float dt = currentTime - oldTime;
  oldTime = currentTime;
  return dt;
}

void reset(){
  for(int j=0;j<8;j++){
    pic[j] = 0;
  }
}

void Update(){
  reset();//Reset (Clear) the 8x8 LED matrix
  
  int newHead[2] = {snake.head[0]+snake.dir[0], snake.head[1]+snake.dir[1]};

  //Handle Borders
  if(newHead[0]==8) {
    newHead[0]=0;
  }else if(newHead[0]==-1) {
    newHead[0] = 7;
  }else if(newHead[1]==8) {
    newHead[1]=0;
  }else if(newHead[1]==-1) { 
    newHead[1]=7;
  }
  
  //Check If The Snake hits itself
  for(j=0;j<snake.len;j++){
    if(snake.body[j][0] == newHead[0] && snake.body[j][1] == newHead[1]){
      //Pause the game for 1 sec then Reset it
      snake = {{1,5},{{0,5}, {1,5}}, 2, {1,0}};//Reinitialize the snake object
      apple = {(int)random(0,8),(int)random(0,8)};//Reinitialize an apple object
      
      return;
    }
  }

  //Check if The snake ate the apple
  if(newHead[0] == apple.rPos && newHead[1] ==apple.cPos){
    snake.len = snake.len+1;
    apple.rPos = (int)random(0,8);
    apple.cPos = (int)random(0,8);

    score = score + diff;
  }else{
    removeFirst();//Shifting the array to the left
  }
  
  snake.body[snake.len-1][0]= newHead[0];
  snake.body[snake.len-1][1]= newHead[1];
  
  snake.head[0] = newHead[0];
  snake.head[1] = newHead[1];
  
  //Update the pic Array to Display(snake and apple)
  for(j=0;j<snake.len;j++){
    pic[snake.body[j][0]] |= 128 >> snake.body[j][1];
  }
  pic[apple.rPos] |= 128 >> apple.cPos;
  
}

void Render(){
  
   for(i=0;i<8;i++){
    lc.setRow(0,i,pic[i]);
   }
}

void removeFirst(){
  for(j=1;j<snake.len;j++){
    snake.body[j-1][0] = snake.body[j][0];
    snake.body[j-1][1] = snake.body[j][1];
  }
}

void endGame() {
  isGameEnded = true;
  lcd.setCursor(0, 0);
  lcd.print("Game over!");

  lcd.setCursor(0, 1);
  lcd.print(score);

  if (millis() - timeEndScreen > 2000) {
    isGameEnded = false;    
  }  
}