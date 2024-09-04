/*
    ASSIGNMENT: Lab 2

        Author: Justin Dang
         Email: Jdang065@ucr.edu
    Discussion: B21
    Student ID: 862396638
    >--------------------------------------------
    FUNCTION:
    >--------------------------------------------
    Documentation:
        - Constructor: N/A
        - Variables:
        - Methods:
    >--------------------------------------------
*/
#include <avr/io.h>
#include <util/delay.h>
#include "timer.h"
#include <Arduino.h>
#include "irAVR.h"
#include "LiquidCrystal.h"
#include <EEPROM.h>

// Global Variables
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
decode_results irvalue;
int highscore;
int currentscore;
unsigned char animationFrame = 0;
unsigned long randomNum = 0;

#define GCDPERIOD 50

// For gameArea, 32 is a safe spot
byte gameArea[16] {32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32};
byte jump = 32;

#define checkNoCollision gameArea[1] == 32 || gameArea[1] == 0 || gameArea[1] == 1
#define player1 gameArea[1] == 0 || gameArea[1] == 1
#define obstacle0 gameArea[0] == 2 || gameArea[0] == 3 || gameArea[0] == 4 || gameArea[0] == 5 || gameArea[0] == 6

enum GameStates { menu, game, death } gameState;

#pragma region menus
void saveScore(int score){
  int8_t one = score & 0xFF;
  int8_t two = (score >> 8) & 0xFF;

  EEPROM.update(0, two);
  EEPROM.update(1, one);
}

void clearGameArea(){
  for(int i = 0; i < 16; i++){
    gameArea[i] = 32;
  }
}

int getHighScore(){
  int8_t one = EEPROM.read(1);
  int8_t two = EEPROM.read(0);

  return one + two;
}

void ShowMainScreen() {
  // Title
  lcd.setCursor(2, 0);
  lcd.print("Man vs World");

  // draw player
  lcd.setCursor(1, 1);
  lcd.write(byte(0));

  // draw random obstacles and pray no overlap
  for(int i = 0; i < 2; i++){
    lcd.setCursor(random(3, 7), 1);
    lcd.write(byte(random(2,  6)));
  }

  // HighScore
  lcd.setCursor(8, 1);
  lcd.print("Best:");
  lcd.print(highscore);
}

void printCurrentScore(){
  lcd.setCursor(7, 0);
  lcd.print("Score:");
  lcd.print(currentscore);
}

void showDeathScreen(){
  lcd.setCursor(4, 1);
  lcd.print("Game Over!");
  _delay_ms(2000);
  lcd.setCursor(3, 1);
  lcd.print("Prev Best:");
  lcd.setCursor(13, 1);
  lcd.print(highscore);
}

int menuSetup = 1;

void menuFNC(){
  if(menuSetup){
    // display highscore
    highscore = getHighScore();

    // Can do fancy title screen here
    ShowMainScreen();
    menuSetup = 0;
    _delay_ms(1000);
  }
  // maybe have an autoplaying bg as if dino is running indefinetly??
  // HAHAHA you are comedian, no

  if (IRdecode(&irvalue)) 
  {
    lcd.clear();
    gameState = game;
    menuSetup = 1;
  }
  IRresume();
}
#pragma endregion

#pragma region player/game
void updateDudeAnimation(){
  (animationFrame) ? animationFrame = 0 : animationFrame = 1;
}

void TrackScoreFNC(){
  if(obstacle0){
    currentscore++;
  }
}

// pick a number between 0 and obstacleSpawnChance. Needs to be between 2-6(5 possible chances) to spawn a obstacle.
// if obstacleSpawnChance = 10 -> 5 / 10 = 50% obstacle spawnchance
const byte obstacleSpawnChance = 15;

void SpawnObstacle(){
  // spawn obstacle
  randomNum = random(obstacleSpawnChance);
  if(randomNum <= 6 && randomNum >= 2){
    gameArea[15] = randomNum;
  }
  else {
    gameArea[15] = 32;
  }
}

void updateScreen(){
  for (int i = 0; i <= 15; i++) {
    lcd.setCursor(i, 1);
    lcd.write(gameArea[i]);
  }
  lcd.setCursor(1, 0);
  lcd.write(jump);
}

void ScrollLeft(){
  // shift bottom row to the left
  for (int i = 0; i <= 14; i++) {
    gameArea[i] = gameArea[i + 1];
  }
  if(gameArea[0] == 0 || gameArea[0] == 1){
    gameArea[0] = 32;
  }
  gameArea[15] = 32;
}

int jumpElapsedTime = 1000;
const int jumpLength = 1000;

void PlayerFNC(int elapsedTime){
  // while we are on the ground
  if(!IRdecode(&irvalue) && (jumpElapsedTime >= 350)){
    jumpElapsedTime = jumpLength;
  }
  if (jumpElapsedTime >= jumpLength) {
    if (checkNoCollision) {
      gameArea[1] = animationFrame;
      jump = 32;

      // jump
      if (IRdecode(&irvalue) && (player1)) {
        // check if game[1] is player
        // set it to an empty/safe spot
        gameArea[1] = 32;
        jump = 0;
        jumpElapsedTime = 0;
      }
    } 
    else {
      gameState = death;
    }
  }
  IRresume();
  jumpElapsedTime += elapsedTime;
}

int deathFuncComplete = 0;
void deathFNC(){
  if(!deathFuncComplete){
    // Death UI
    showDeathScreen();

    // If highscore, save to eeprom
    if(currentscore > highscore){
      highscore = currentscore;
      saveScore(highscore);
    }
    deathFuncComplete = 1;
  }

  // Any button input = change gamestate to game
  if (IRdecode(&irvalue)) 
  {
    gameState = menu;
    deathFuncComplete = 0;
    clearGameArea();
    lcd.clear();
    currentscore = 0;
  }
  IRresume();
}
#pragma endregion

#define OBSTACLESPAWNCOOLDOWN 1000
#define ANIMATIONSPEED 250
int playerTickRate = 100;
int gameTickSpeed = 500;
int gameElapsedTime = gameTickSpeed;
int playerElapsedTime = playerTickRate;
int obstacleElapsedTime = 0;
int animationElapsedTime = ANIMATIONSPEED;

void GameStateFNC(int elapsedTime){
  switch(gameState){
    case menu:
      menuFNC();
      break;
    case game:
      if(animationElapsedTime >= ANIMATIONSPEED){
        updateDudeAnimation();
        animationElapsedTime = 0;
      }
      if(gameElapsedTime >= gameTickSpeed){
        ScrollLeft();
        TrackScoreFNC();
        printCurrentScore();
        gameElapsedTime = 0;
      }
      if(obstacleElapsedTime >= OBSTACLESPAWNCOOLDOWN){
        SpawnObstacle();
        obstacleElapsedTime = 0;
      }
      if(playerElapsedTime >= playerTickRate){
        PlayerFNC(elapsedTime);
        updateScreen();
        playerElapsedTime = 0;
      }
      gameElapsedTime += elapsedTime;
      playerElapsedTime += elapsedTime;
      obstacleElapsedTime += elapsedTime;
      animationElapsedTime += elapsedTime;
      break;
    case death:
      deathFNC();
      break;
  }
}

#pragma  region BitMasks
byte dude1[8] = {
  0b01101,
  0b01101,
  0b11111,
  0b10100,
  0b10111,
  0b00101,
  0b11101,
  0b00001
};

byte dude2[8] = {
  0b01100,
  0b01100,
  0b00100,
  0b00111,
  0b01100,
  0b00110,
  0b01001,
  0b10001
};

byte Tree[8] = {
  0b01110,
  0b10111,
  0b01110,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b11111
};

byte Antenna[8] = {
  0b00100,
  0b01010,
  0b01010,
  0b01010,
  0b10101,
  0b10101,
  0b10101,
  0b10101
};

byte Mountain[8] = {
  0b00100,
  0b01110,
  0b01110,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};

byte Smile[8] = {
  0b00000,
  0b01010,
  0b01010,
  0b01010,
  0b10001,
  0b11111,
  0b00000,
  0b00000
};
byte Money[8] = {
  0b00100,
  0b01111,
  0b10100,
  0b01110,
  0b00101,
  0b11110,
  0b00100,
  0b00000
};

byte Heart[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};
#pragma endregion

int main()
{
  // DDR Setup 
  DDRC = 0x00;
  DDRD = 0b11111111;
  DDRB = 0B11111110;

  // Setup
  TimerSet(GCDPERIOD);
  TimerOn();
  IRinit(&PORTB, &PINB, PORTB0);
  Serial.begin(9600);

  lcd.begin(16,2);

  lcd.createChar(0, dude1);
  lcd.createChar(1, dude2);
  lcd.createChar(2, Antenna);
  lcd.createChar(3, Mountain);
  lcd.createChar(4, Smile);
  lcd.createChar(5, Heart);
  lcd.createChar(6, Money);

  lcd.clear();
  
  
  while (true)
  {
    GameStateFNC(GCDPERIOD);

    while (!TimerFlag);
    TimerFlag = false;
  }

  return 0; // <-- this return statement is never reached
}