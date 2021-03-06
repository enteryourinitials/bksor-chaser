/**
 * Black Knight: Sword of Rage - Ramp Chaser LED Project
 * 
 * Author: Andrew Burch
 * Notes: Arduino driven LED animation for the ramp on Black Knight: Sword of Rage pinball machine
 *    - Designed to animate the unlit ramp lighting bolt cutouts on Black Knight Pro models 
 *    - Controlled by Arduino Uno board - requires 12V input
 *    - Uses addressable WS2812 RGB LED strip cut in 2 - requires 5V
 *    - Reads sensor to switch between animation states - requires 5V
 *    
 *    - LED index layout
 *       13 | 14  <- top of ramp
 *       12 | 15
 *       11 | 16
 *       10 | 17
 *        9 | 18
 *        8 | 19
 *        7 | 20
 *        6 | 21
 *        5 | 22
 *        4 | 23
 *        3 | 24
 *        2 | 25
 *        1 | 26
 *        0 | 27  <- ramp entry
 */

#include "FastLED.h"

// project configuration values
#define NUM_ANIMATION_LEDS 28           // total number of LEDs required for animation

#define DATA_PIN_RANDOM 0               // use noise for random seed
#define DATA_PIN_LED 7                  // LED communicataion pin from Uno board
#define DATA_PIN_SENSOR 8               // sensor read pin (used flail state detection)

#define LEDS_PER_STRIP 14               // number of LEDs per strip
#define LEFT_STRIP_INDEX 0              // first LED index of left strip
#define RIGHT_STRIP_INDEX 14            // first LED index of right strip

#define COLOUR_SEQUENCE_SIZE 11         // number of colours in the animation sequence
#define COLOUR_SEQUENCE_SIZE_SHORT 7    // number of colours in the shorter animation sequence
#define COLOUR_DATA_LENGTH 22

#define ANIMATION_DELAY_CHASE 50        // delay (in milliseconds) between animation frame update
#define ANIMATION_DELAY_OFFSET_CHASE 40
#define ANIMATION_DELAY_GLOW 45
#define ANIMATION_DELAY_CROSS 45
#define ANIMATION_DELAY_ZIPPY 30
#define ANIMATION_DELAY_ZIPPY_CROSS 30
#define ANIMATION_DELAY_DECAY 0         // no delay here as we'll use the decayActiveDelays array
#define ANIMATION_DELAY_NONE 0

#define ANIMATION_TYPE_CHASE 0          // animation types
#define ANIMATION_TYPE_OFFSET_CHASE 1
#define ANIMATION_TYPE_GLOW 2
#define ANIMATION_TYPE_CROSS 3
#define ANIMATION_TYPE_ZIPPY 4
#define ANIMATION_TYPE_ZIPPY_CROSS 5
#define ANIMATION_TYPE_DECAY 6
#define ANIMATION_TYPE_NONE 7

#define ANIMATION_DIRECTION_UP 1
#define ANIMATION_DIRECTION_DOWN -1

#define FLAIL_STATE_UNKNOWN 0
#define FLAIL_STATE_OPEN 1
#define FLAIL_STATE_CLOSED 2
#define FLAIL_STATE_MOVING 3

#define ANIMATION_CHANGE_UNSTABLE 350
#define ANIMATION_CHANGE_COOLDOWN 600

// global variables
int animationType = ANIMATION_TYPE_NONE;      // the active animation type for rendering to the LED strip
int animationTypeDelay = 0;                   // the delay reset in milliseconds between animation updates for the active animation type
int animationDelay = 0;                       // the delay in milliseconds between animation updates

int colourScheme[] = {                        // red / green values for colour scheme - blue always 0.
    255, 154, 
    255, 122, 
    255, 90, 
    255, 0, 
    128, 0, 
    64, 0, 
    32, 0, 
    0, 0, 
    0, 0, 
    0, 0, 
    0, 0
  };

unsigned long currentMillis;
unsigned long lastFrameMillis;
unsigned long deltaTime;

int flailStateLockedIn = FLAIL_STATE_UNKNOWN; // current stable flail state in control of animation
int flailStateDesired = FLAIL_STATE_UNKNOWN;  // desired flail state based on current state of the flail returned by the sensor
int flailStateChangeCountdown = 0;            // duration (in ms) that the desired flail state must remain constant before animation will update
int flailStateUnstableDuration = 0;           // duration the flail has been moving constantly

int colourSequenceIndex = 0;                  // index into colour animation table
int glowDirection = ANIMATION_DIRECTION_UP;   // glow animation direction (1 = fade up, -1 = fade down)
int zippyDirection = ANIMATION_DIRECTION_UP;  // zip animation directon (1 = strobe up strip, -1 = strobe down strip)
int zippyAnimateIndex = 0;

int decayActiveDelays[NUM_ANIMATION_LEDS];    // data tables for decay animation type
int decayResetDelays[NUM_ANIMATION_LEDS];
int decayColourIndexes[NUM_ANIMATION_LEDS];

CRGB leds[NUM_ANIMATION_LEDS];                // RGB resources for rendering to the LED strips
CRGB colourSequence[COLOUR_SEQUENCE_SIZE];    // colour look up table for animation


/**
 * setup - initialise LED strip and colour sequence
 */
void setup()
{
  //Serial.begin(9600);   // enable for console logging

  FastLED.addLeds<WS2812, DATA_PIN_LED, GRB>(leds, NUM_ANIMATION_LEDS);

  // init colour sequence
  for (int i = 0; i < COLOUR_DATA_LENGTH; i+=2)
  {
    colourSequence[i >> 1] = CRGB(colourScheme[i], colourScheme[i + 1], 0);
  }

  pinMode(DATA_PIN_SENSOR, INPUT);

  randomSeed(analogRead(DATA_PIN_RANDOM));

  resetLEDState();

  FastLED.show();

  lastFrameMillis = millis();
}


/**
 * main update loop - calculate delta time and call update functions
 */
void loop()
{
  currentMillis = millis();
  
  deltaTime = currentMillis - lastFrameMillis;

  updateAnimation(deltaTime);

  updateAnimationType(deltaTime);

  lastFrameMillis = currentMillis;
}


/**
 * apply delta time to animation delay and update animation to LED strip
 */
void updateAnimation(unsigned long deltaTime)
{
  animationDelay -= deltaTime;

  if (animationDelay > 0)
  {
    return;
  }

  switch (animationType)
  {
    case ANIMATION_TYPE_CHASE:
      animationChase(false);
      break;
    case ANIMATION_TYPE_OFFSET_CHASE:
      animationChase(true);
      break;
    case ANIMATION_TYPE_GLOW:
      animationGlow();
      break;
    case ANIMATION_TYPE_CROSS:
      animationCross();
      break;
    case ANIMATION_TYPE_ZIPPY:
      animationZippy(false);
      break;
    case ANIMATION_TYPE_ZIPPY_CROSS:
      animationZippy(true);
      break;
    case ANIMATION_TYPE_DECAY:
      animationDecay(deltaTime);  
      break;
    case ANIMATION_TYPE_NONE:
      break;
  }
    
  FastLED.show();

  animationDelay += animationTypeDelay;
}


/**
 * evaluate sensor state detection and switch to desired animation type
 */
void updateAnimationType(unsigned long deltaTime)
{
  // determine if the sensor can currently detect the flail in its path
  int flailStateDetected = FLAIL_STATE_OPEN;

  if (digitalRead(DATA_PIN_SENSOR) == LOW)
  {
    flailStateDetected = FLAIL_STATE_CLOSED;
  }

  // if detected flail state matches the last stable flail state and there is no change countdown in progress, assume no change in progress.
  if (flailStateDetected == flailStateLockedIn && flailStateChangeCountdown == 0)
  {
    return;
  }

  int switchToAnimationType = ANIMATION_TYPE_NONE;

  if (flailStateDetected == flailStateDesired)
  {
    flailStateChangeCountdown -= deltaTime;

    // if the flail state has remained consistent for long enough, switch the aniation type
    if (flailStateChangeCountdown <= 0)
    {
      flailStateChangeCountdown = 0;
      flailStateUnstableDuration = 0;
      flailStateLockedIn = flailStateDesired;
      flailStateDesired = FLAIL_STATE_UNKNOWN;

      switchToAnimationType = flailStateLockedIn == FLAIL_STATE_CLOSED ? ANIMATION_TYPE_GLOW : ANIMATION_TYPE_OFFSET_CHASE;
    }
  }
  else
  {
    flailStateDesired = flailStateDetected;
    flailStateUnstableDuration += (1 - (flailStateChangeCountdown / ANIMATION_CHANGE_COOLDOWN)) * 45; // dampening for unstable flail animation
    flailStateChangeCountdown = ANIMATION_CHANGE_COOLDOWN;
    
    // detect if the flail movement is unstable enough to switch the animation type
    if ((flailStateUnstableDuration > ANIMATION_CHANGE_UNSTABLE) && (animationType != ANIMATION_TYPE_DECAY))
    {
      switchToAnimationType = ANIMATION_TYPE_DECAY;
    }
  }

  if (switchToAnimationType == ANIMATION_TYPE_NONE)
  {
    return;
  }

  // kick off a new animation state for the LED strips
  animationType = switchToAnimationType;

  initAnimationType(animationType);

  resetLEDState();
}


/**
 * initialise animation type variables
 */
void initAnimationType(int animType)
{
  switch (animType)
  {
    case ANIMATION_TYPE_CHASE:
      animationDelay = ANIMATION_DELAY_CHASE;
      animationTypeDelay = ANIMATION_DELAY_CHASE;
      colourSequenceIndex = 0;
      break;

    case ANIMATION_TYPE_OFFSET_CHASE:
      animationDelay = ANIMATION_DELAY_OFFSET_CHASE;
      animationTypeDelay = ANIMATION_DELAY_OFFSET_CHASE;
      colourSequenceIndex = 0;
      break;

    case ANIMATION_TYPE_GLOW:
      animationDelay = ANIMATION_DELAY_GLOW;
      animationTypeDelay = ANIMATION_DELAY_GLOW;
      colourSequenceIndex = 0;
      glowDirection = ANIMATION_DIRECTION_UP;
      break;

    case ANIMATION_TYPE_CROSS:
      animationDelay = ANIMATION_DELAY_CROSS;
      animationTypeDelay = ANIMATION_DELAY_CROSS;
      colourSequenceIndex = 0;
      glowDirection = ANIMATION_DIRECTION_UP;
      break;

    case ANIMATION_TYPE_ZIPPY:
      animationDelay = ANIMATION_DELAY_ZIPPY;
      animationTypeDelay = ANIMATION_DELAY_ZIPPY;
      zippyDirection = ANIMATION_DIRECTION_UP;
      zippyAnimateIndex = 0;
      break;

    case ANIMATION_TYPE_ZIPPY_CROSS:
      animationDelay = ANIMATION_DELAY_ZIPPY_CROSS;
      animationTypeDelay = ANIMATION_DELAY_ZIPPY_CROSS;
      zippyDirection = ANIMATION_DIRECTION_UP;
      zippyAnimateIndex = 0;
      break;

    case ANIMATION_TYPE_DECAY:
      animationDelay = ANIMATION_DELAY_DECAY;
      animationTypeDelay = ANIMATION_DELAY_DECAY;

      for (int i = 0; i < NUM_ANIMATION_LEDS; i++)
      {
        int newDelay = 35 + (random(0, 6) * 5);

        decayActiveDelays[i] = newDelay;
        decayResetDelays[i] = newDelay;
        decayColourIndexes[i] = random(0, 9);        
      }
      break;

    default:
      // we shouldn't reach this
      break;
  }
}


/**
 * Reset all LEDs to off
 */
void resetLEDState()
{
  for (int i = 0; i < NUM_ANIMATION_LEDS; i++)
  {
    leds[i] = CRGB(0, 0, 0);
  }  
}


/**
 * Cycle colour sequence up the 2 LED strips like a chase effect
 */
void animationChase(bool animateWithOffset)
{
  // cycle existing colours up the left strip (uses LED's 0 -> 7)
  for (int i = LEDS_PER_STRIP - 2; i >= 0; i--)
  {
    leds[i + 1] = leds[i];
  }

  // cycle existing colours up the right strip (uses LED's 15 -> 8)
  for (int i = 0; i <= LEDS_PER_STRIP - 2; i++)
  {
    leds[RIGHT_STRIP_INDEX + i] = leds[RIGHT_STRIP_INDEX + i + 1];
  }

  // if right strip is offset, calculate alternative colour sequence index
  int rightLEDStripColourIndex = colourSequenceIndex;

  if (animateWithOffset)
  {
    rightLEDStripColourIndex = colourSequenceIndex + COLOUR_SEQUENCE_SIZE_SHORT;

    if (rightLEDStripColourIndex >= COLOUR_SEQUENCE_SIZE)
    {
      rightLEDStripColourIndex = rightLEDStripColourIndex - (COLOUR_SEQUENCE_SIZE - 1);
    }   
  }

  // feed next colour from the sequence into the beginning of both strips
  leds[LEFT_STRIP_INDEX] = colourSequence[colourSequenceIndex];
  leds[NUM_ANIMATION_LEDS - 1] = colourSequence[rightLEDStripColourIndex];

  colourSequenceIndex++;

  // loop colour sequence index once all colours rendered
  if (colourSequenceIndex == COLOUR_SEQUENCE_SIZE)
  {
    colourSequenceIndex = 0;
  }
}


/**
 * Glow LED's forwards and backwards through the colour sequence
 */
void animationGlow()
{
  // apply current colour from the animation sequence to the LEDs
  for (int i  = 0; i < NUM_ANIMATION_LEDS; i++)
  {
    leds[i] = colourSequence[colourSequenceIndex];
  }

  // advance colour index based on the direction
  colourSequenceIndex += glowDirection;

  // if a barrier is reached, swap the animation direction
  if (colourSequenceIndex < 0 || (colourSequenceIndex == COLOUR_SEQUENCE_SIZE))
  {
    glowDirection *= -1;
    colourSequenceIndex += glowDirection; // get the index back into a valid state
  }
}


/**
 * Cross fade the 2 LED strips
 */
void animationCross()
{
  int inverseColourSequenceIndex = (COLOUR_SEQUENCE_SIZE - 1) - colourSequenceIndex;
  
  for (int i  = 0; i < LEDS_PER_STRIP; i++)
  {
    leds[i] =  colourSequence[colourSequenceIndex];
    leds[RIGHT_STRIP_INDEX + i] = colourSequence[inverseColourSequenceIndex];
  }

  colourSequenceIndex += glowDirection;

  // if a barrier is reached, swap the animation direction
  if (colourSequenceIndex < 0 || (colourSequenceIndex == COLOUR_SEQUENCE_SIZE))
  {
    glowDirection *= -1;
    colourSequenceIndex += glowDirection; // get the index back into a valid state
  }
}


/**
 * Strobe up and down the LED strips
 */
void animationZippy(bool oppositeStrips)
{
  int readColourIndexStart = zippyAnimateIndex >= LEDS_PER_STRIP ? (zippyAnimateIndex - 13) : 0;
  int leftLEDStripWriteIndex = zippyDirection == ANIMATION_DIRECTION_UP ? min(13, zippyAnimateIndex) : max(13 - zippyAnimateIndex, 0);
  int writeAdvance = -zippyDirection;
  int colourSequenceReadIndex;
  int rightLEDStripWriteIndex;

  for (int i = readColourIndexStart; i <= zippyAnimateIndex; i++)
  {
    colourSequenceReadIndex = min(i, 13);
    rightLEDStripWriteIndex = oppositeStrips ? (RIGHT_STRIP_INDEX + leftLEDStripWriteIndex) : (27 - leftLEDStripWriteIndex);

    leds[leftLEDStripWriteIndex] = colourSequence[colourSequenceReadIndex];
    leds[rightLEDStripWriteIndex] = colourSequence[colourSequenceReadIndex];

    leftLEDStripWriteIndex += writeAdvance;
  }

  zippyAnimateIndex++;

  // detect if entire colour sequence has played across LED strip, if so, switch directions
  if (zippyAnimateIndex == 30)
  {
    zippyDirection *= -1;
    zippyAnimateIndex = 0;    
  }
}


/**
 * decay colours from bright to black on each LED
 */
void animationDecay(unsigned long deltaTime)
{
  for (int i = 0; i < NUM_ANIMATION_LEDS; i++)
  {
    decayActiveDelays[i] -= (int) deltaTime;

    if (decayActiveDelays[i] > 0)
    {
      continue;
    }

    decayColourIndexes[i]++;

    if (decayColourIndexes[i] == 10)
    {
      decayColourIndexes[i] = 0;

      decayResetDelays[i] = 35 + (random(0, 6) * 5);
    }

    leds[i] = colourSequence[decayColourIndexes[i]];

    decayActiveDelays[i] = decayResetDelays[i];
  }
}