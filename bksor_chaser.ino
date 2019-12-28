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

#define SWITCH_ANIMATE_DELAY 4000     // number of milliseconds to play each animation type

#define zippyMapSize COLOUR_SEQUENCE_SIZE * 2 + LEDS_PER_STRIP;

// global variables
int animationType = ANIMATION_TYPE_CHASE;
int animationDelay = ANIMATION_DELAY_CHASE;

int colourSequenceIndex = 0;                // index into colour animation table
int animationTimer = SWITCH_ANIMATE_DELAY;  // time until animation type is updated
int glowDirection = 1;                      // glow animation direction (1 = fade up, -1 = fade down)

CRGB leds[NUM_ANIMATION_LEDS];              // RGB resources for rendering to the LED strips
CRGB colourSequence[COLOUR_SEQUENCE_SIZE];  // colour look up table for animation

int zippyMapDirection = 1;
int zippyMapIndex = COLOUR_SEQUENCE_SIZE - 1;
CRGB zippyMap[29];


/**
 * setup - Initialise LED strip and colour sequence
 */
void setup()
{
  FastLED.addLeds<WS2812, DATA_PIN_LED, GRB>(leds, NUM_ANIMATION_LEDS);

  for (int i = 0; i < 29; i++)
  {
    zippyMap[i] = CRGB(0, 0, 0);
  }  

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
}


/**
 * main update loop - update animation and rendering
 */
void loop()
{
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
      animationZippy();
      break;
    case ANIMATION_TYPE_ZIPPY_CROSS:
      animationZippyCross();
      break;
     default:
      break;
  }
  
  FastLED.show();

  delay(animationDelay);

  updateAnimationType(animationDelay);
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
 * Update the animation type being played
 */
void updateAnimationType(int frameDelay)
{
  // advance the count down
  animationTimer -= frameDelay;

  // once count down reaches zero, switch to the next animation type
  if (animationTimer <= 0)
  {
    animationTimer = SWITCH_ANIMATE_DELAY;
    
    animationType++;

    // if all animation types have been played, loop back to the start
    if (animationType > ANIMATION_TYPE_ZIPPY_CROSS)
    {
      animationType = ANIMATION_TYPE_CHASE;
    }

    initAnimationType(animationType);

    // default all LEDs to being off when entering new state
    resetLEDState();
  }  
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
      colourSequenceIndex = 0;
      break;
    case ANIMATION_TYPE_OFFSET_CHASE:
      animationDelay = ANIMATION_DELAY_OFFSET_CHASE;
      colourSequenceIndex = 0;
      break;
    case ANIMATION_TYPE_GLOW:
      animationDelay = ANIMATION_DELAY_GLOW;
      colourSequenceIndex = 0;
      glowDirection = 1;
      break;
    case ANIMATION_TYPE_CROSS:
      animationDelay = ANIMATION_DELAY_CROSS;
      colourSequenceIndex = 0;
      glowDirection = 1;
      break;
    case ANIMATION_TYPE_ZIPPY:
      animationDelay = ANIMATION_DELAY_ZIPPY;
      break;
    case ANIMATION_TYPE_ZIPPY_CROSS:
      animationDelay = ANIMATION_DELAY_ZIPPY_CROSS;
      break;
    default:
      // we shouldn't reach this
      break;
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
void animationZippy()
{
  if (zippyMapDirection == 1)
  {
    for (int i  = 0; i < COLOUR_SEQUENCE_SIZE; i++)
    {
      zippyMap[zippyMapIndex - i] = colourSequence[i];
    }

    zippyMapIndex += 1;

    if (zippyMapIndex == 24)
    {
      zippyMapDirection = 2;
      zippyMapIndex = 18;
    }
  }

  if (zippyMapDirection == 2)
  {
    for (int i  = 0; i < COLOUR_SEQUENCE_SIZE; i++)
    {
      zippyMap[zippyMapIndex + i] = colourSequence[i];
    }

    zippyMapIndex -= 1;

    if (zippyMapIndex == 4)
    {
      zippyMapDirection = 1;
      zippyMapIndex = 10;
    }
  }

  // copy buffer segment to the LED strip for rendering
  for (int i = 0; i < LEDS_PER_STRIP; i++)
  {
    leds[i] = zippyMap[COLOUR_SEQUENCE_SIZE + i];
    leds[NUM_ANIMATION_LEDS - 1 - i] = zippyMap[COLOUR_SEQUENCE_SIZE + i];
  }  
}


/**
 * Strobe up and down the LED strips, but each strip is the opposite
 */
void animationZippyCross()
{
  if (zippyMapDirection == 1)
  {
    for (int i  = 0; i < COLOUR_SEQUENCE_SIZE; i++)
    {
      zippyMap[zippyMapIndex - i] = colourSequence[i];
    }

    zippyMapIndex += 1;

    if (zippyMapIndex == 24)
    {
      zippyMapDirection = 2;
      zippyMapIndex = 18;
    }
  }

  if (zippyMapDirection == 2)
  {
    for (int i  = 0; i < COLOUR_SEQUENCE_SIZE; i++)
    {
      zippyMap[zippyMapIndex + i] = colourSequence[i];
    }

    zippyMapIndex -= 1;

    if (zippyMapIndex == 4)
    {
      zippyMapDirection = 1;
      zippyMapIndex = 10;
    }
  }

  // copy buffer segment to the LED strip for rendering
  for (int i = 0; i < LEDS_PER_STRIP; i++)
  {
    leds[i] = zippyMap[COLOUR_SEQUENCE_SIZE + i];
    leds[NUM_ANIMATION_LEDS - 1 - i] = zippyMap[COLOUR_SEQUENCE_SIZE + LEDS_PER_STRIP - i - 1];
  }  
}
