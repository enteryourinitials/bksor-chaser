/**
 * Black Knight: Sword of Rage - Ramp Chaser LED Project
 * 
 * Author: Andrew Burch
 * Notes: Arduino driven LED animation for the ramp on Black Knight: Sword of Rage pinball machine
 *    - Designed to animate the dark ramp on Black Knight Pro models 
 *    - Controlled by Arduino Uno board - requires 12V input
 *    - Addressable WS2812 RGB LED strip cut in 2 that animates
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

#define ANIMATION_DELAY 50              // delay (in milliseconds) between animation update

#define ANIMATION_TYPE_CHASE 0
#define ANIMATION_TYPE_OFFSET_CHASE 1
#define ANIMATION_TYPE_GLOW 2
#define ANIMATION_TYPE_CROSS 3

#define COLOUR_SEQUENCE_SIZE 11         // number of colours in the animation sequence


// global variables
int animationType = ANIMATION_TYPE_CHASE;
int colourSequenceIndex = 0;            // index into colour animation table
int changeAnimationTypeCountdown = 80;  // duration to play animation type
float glowDirection = 1;                // glow animation direction (1 = fade up, -1 = fade down)

CRGB leds[NUM_ANIMATION_LEDS];              // RGB resources for rendering to the LED strips
CRGB colourSequence[COLOUR_SEQUENCE_SIZE];  // colour look up table for animation


/**
 * setup - Initialise LED strip and colour sequence
 */
void setup()
{
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
     default:
      break;
  }
  
  FastLED.show();

  delay(ANIMATION_DELAY);

  updateAnimationType();
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
void updateAnimationType()
{
  // advance the count down
  changeAnimationTypeCountdown--;

  // once count down reaches zero, switch to the next animation type
  if (changeAnimationTypeCountdown == 0)
  {
    // reset the count down and colour index for the next animation type
    changeAnimationTypeCountdown = 80;
    colourSequenceIndex = 0;

    // default all LEDs to being off when entering new state
    resetLEDState();

    animationType++;

    // if all animation types have been played, loop back to the start
    if (animationType > ANIMATION_TYPE_CROSS)
    {
      animationType = ANIMATION_TYPE_CHASE;
    }
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
  
  for (int i  = 0; i < LEDS_PER_STRIP - 1; i++)
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
