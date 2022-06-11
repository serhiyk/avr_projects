#include <Keyboard.h>
#include <Mouse.h>

void setup()
{
//  Serial1.begin(57600);
//  Serial.begin(57600);
  Keyboard.begin();
  Mouse.begin();
  randomSeed(analogRead(0));
}

void loop()
{
  Mouse.click(MOUSE_LEFT);
//  Mouse.press(MOUSE_LEFT);
//  Mouse.release(MOUSE_LEFT);
//  Keyboard.write('a');
//  Keyboard.press('a');
//  Keyboard.release('a');

  delay(1000*(50 + random(0, 20)));
}
