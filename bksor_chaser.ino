/**
 * Black Knight: Sword of Rage - Ramp Chaser LED Project
 * 
 * Author: Andrew Burch
 * Notes: Arduino driven LED animation for the ramp on Black Knight: Sword of Rage pinball machine
 *    - Designed to animate the unlit ramp lighting bolt cutouts on Black Knight Pro models 
 *    - Controlled by Arduino Uno board - requires 12V input
 *    - Uses addressable WS2812 RGB LED strip cut in 2
 *    - TODO: Add sensor to detect flail movement and change animation type
 *    
 *    - LED index layout
 *        6 | 7   <- top of ramp
 *        5 | 8
 *        4 | 9
 *        3 | 10
 *        2 | 11
 *        1 | 12
 *        0 | 13  <- ramp entry
 */

#include "FastLED.h"

// project configuration values
#define NUM_ANIMATION_LEDS 14           // total number of LEDs required for animation

#define DATA_PIN_LED 7                  // LED communicataion pin from Uno board
#define DATA_PIN_SENSOR 8               // sensor read pin (used flail state detection)

#define LEDS_PER_STRIP 7                // number of LEDs per strip
#define LEFT_STRIP_INDEX 0              // first LED index of left strip
#define RIGHT_STRIP_INDEX 7             // first LED index of right strip

#define COLOUR_SEQUENCE_SIZE 11         // number of colours in the animation sequence

#define ANIMATION_DELAY_CHASE 50        // delay (in milliseconds) between animation frame update
#define ANIMATION_DELAY_OFFSET_CHASE 50
#define ANIMATION_DELAY_GLOW 50
#define ANIMATION_DELAY_CROSS 50
#define ANIMATION_DELAY_ZIPPY 40
#define ANIMATION_DELAY_ZIPPY_CROSS 40

#define ANIMATION_TYPE_CHASE 0
#define ANIMATION_TYPE_OFFSET_CHASE 1
#define ANIMATION_TYPE_GLOW 2
#define ANIMATION_TYPE_CROSS 3
#define ANIMATION_TYPE_ZIPPY 4
#define ANIMATION_TYPE_ZIPPY_CROSS 5
#define ANIMATION_TYPE_NONE 6

#define ZIPPY_DIRECTION_UP 1
#define ZIPPY_DIRECTION_DOWN 2

#define SWITCH_ANIMATE_DELAY 5000     // number of milliseconds to play each animation type


// global variables
int animationType = ANIMATION_TYPE_CHASE;   // current animation type to render
int animationTypeDelay = ANIMATION_DELAY_CHASE;
int animationDelay = ANIMATION_DELAY_CHASE; // delay (in milliseconds) between each update
int animationTimer = SWITCH_ANIMATE_DELAY;  // time until animation type is updated

unsigned long deltaTime;
unsigned long currentMillis;
unsigned long lastFrameMillis;

int colourSequenceIndex = 0;                // index into colour animation table
int glowDirection = 1;                      // glow animation direction (1 = fade up, -1 = fade down)
int zippyMapDirection = ZIPPY_DIRECTION_UP;
int zippyMapIndex = COLOUR_SEQUENCE_SIZE - 1;
CRGB zippyMap[29];

CRGB leds[NUM_ANIMATION_LEDS];              // RGB resources for rendering to the LED strips
CRGB colourSequence[COLOUR_SEQUENCE_SIZE];  // colour look up table for animation


/**
 * setup - Initialise LED strip and colour sequence
 */
void setup()
{
  for (int i = 0; i < 29; i++)
  {
    zippyMap[i] = CRGB(0, 0, 0);
  }  

  FastLED.addLeds<WS2812, DATA_PIN_LED, GRB>(leds, NUM_ANIMATION_LEDS);

  // init colour sequence
  colourSequence[0] = CRGB(255, 154, 0);
  colourSequence[1] = CRGB(255, 122, 0);
  colourSequence[2] = CRGB(255, 90, 0);
  colourSequence[3] = CRGB(255, 0, 0);
  colourSequence[4] = CRGB(128, 0, 0);
  colourSequence[5] = CRGB(64, 0, 0);
  colourSequence[6] = CRGB(32, 0, 0);
  colourSequence[7] = CRGB(0, 0, 0);
  colourSequence[8] = CRGB(0, 0, 0);
  colourSequence[9] = CRGB(0, 0, 0);
  colourSequence[10] = CRGB(0, 0, 0);

  // start all leds as black (off)
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
      animationChase();
      break;
    case ANIMATION_TYPE_OFFSET_CHASE:
      animationOffsetChase();
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
     default:
      break;
  }
    
  FastLED.show();

  animationDelay += animationTypeDelay;
}


/**
 * apply delta time to animation timer and update the animation type being played
 */
void updateAnimationType(unsigned long deltaTime)
{
  animationTimer -= deltaTime;

  if (animationTimer > 0)
  {
    return;
  }

  animationTimer = SWITCH_ANIMATE_DELAY;
  
  animationType++;

  // if all animation types have been played, loop back to the start
  if (animationType == ANIMATION_TYPE_NONE)
  {
    animationType = ANIMATION_TYPE_CHASE;
  }

  initAnimationType(animationType);

  // default all LEDs to being off when entering new state
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
      glowDirection = 1;
      break;
    case ANIMATION_TYPE_CROSS:
      animationDelay = ANIMATION_DELAY_CROSS;
      animationTypeDelay = ANIMATION_DELAY_CROSS;
      colourSequenceIndex = 0;
      glowDirection = 1;
      break;
    case ANIMATION_TYPE_ZIPPY:
      animationDelay = ANIMATION_DELAY_ZIPPY;
      animationTypeDelay = ANIMATION_DELAY_ZIPPY;
      zippyMapDirection = ZIPPY_DIRECTION_UP;
      break;
    case ANIMATION_TYPE_ZIPPY_CROSS:
      animationDelay = ANIMATION_DELAY_ZIPPY_CROSS;
      animationTypeDelay = ANIMATION_DELAY_ZIPPY_CROSS;
      zippyMapDirection = ZIPPY_DIRECTION_UP;
      break;
    default:
      // we shouldn't reach this
      break;
  }
}


/**
 * Reset all LEDs to black (off)
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
void animationChase()
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

  // feed next colour from the sequence into the beginning of both strips
  leds[LEFT_STRIP_INDEX] = colourSequence[colourSequenceIndex];
  leds[NUM_ANIMATION_LEDS - 1] = colourSequence[colourSequenceIndex];

  colourSequenceIndex++;

  // loop colour sequence index once all colours rendered
  if (colourSequenceIndex == COLOUR_SEQUENCE_SIZE)
  {
    colourSequenceIndex = 0;
  }
}


/**
 * Cycle colour sequence up the 2 LED strips like a chase effect
 */
void animationOffsetChase()
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

  int offsetIndex = colourSequenceIndex + 7;

  if (offsetIndex > COLOUR_SEQUENCE_SIZE)
  {
    offsetIndex = offsetIndex - (COLOUR_SEQUENCE_SIZE - 1);
  }

  // feed next colour from the sequence into the beginning of both strips
  leds[LEFT_STRIP_INDEX] = colourSequence[colourSequenceIndex];
  leds[NUM_ANIMATION_LEDS - 1] = colourSequence[offsetIndex];

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
  if (zippyMapDirection == ZIPPY_DIRECTION_UP)
  {
    for (int i  = 0; i < COLOUR_SEQUENCE_SIZE; i++)
    {
      zippyMap[zippyMapIndex - i] = colourSequence[i];
    }

    zippyMapIndex++;

    if (zippyMapIndex == 24)
    {
      zippyMapDirection = ZIPPY_DIRECTION_DOWN;
      zippyMapIndex = 18;
    }
  }

  if (zippyMapDirection == ZIPPY_DIRECTION_DOWN)
  {
    for (int i  = 0; i < COLOUR_SEQUENCE_SIZE; i++)
    {
      zippyMap[zippyMapIndex + i] = colourSequence[i];
    }

    zippyMapIndex--;

    if (zippyMapIndex == 4)
    {
      zippyMapDirection = ZIPPY_DIRECTION_UP;
      zippyMapIndex = 10;
    }
  }

  // copy buffer segment to the LED strip for rendering
  for (int i = 0; i < LEDS_PER_STRIP; i++)
  {
    leds[i] = zippyMap[COLOUR_SEQUENCE_SIZE + i];

    // read index is different if the 2 strips are opposite
    int rightStripIndex = oppositeStrips ? (COLOUR_SEQUENCE_SIZE + LEDS_PER_STRIP - i - 1) : (COLOUR_SEQUENCE_SIZE + i);

    leds[NUM_ANIMATION_LEDS - 1 - i] = zippyMap[rightStripIndex];
  }  
}
