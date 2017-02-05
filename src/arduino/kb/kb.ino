#include <Keyboard.h>
#include <Mouse.h>
#define INCOMING_BUF_SIZE 8
uint8_t incomingByte = 0;
uint8_t incomingBuf[INCOMING_BUF_SIZE];
uint8_t incomingIndex = 0;

uint8_t HexToBin(uint8_t tmp1, uint8_t tmp0)
{
  if(tmp0 > 0x39) tmp0 += 9;
  if(tmp1 > 0x39) tmp1 += 9;
  tmp0 &= 0x0F;
  tmp0 |= tmp1 << 4;

  return tmp0;
}

void setup()
{
  Serial1.begin(57600);
  Serial.begin(57600);
  Keyboard.begin();
  Mouse.begin();
}

void loop()
{
  while (Serial.available())
  {
    if (Serial.available() > 0)
    {
      incomingByte = Serial.read();
      if(incomingByte == 1)
      {
        incomingIndex = 0;
      }
      else if(incomingByte == 0)
      {
        switch(incomingBuf[0])
        {
          case 'w': Keyboard.write(HexToBin(incomingBuf[1], incomingBuf[2])); break;  //This is similar to pressing and releasing a key on your keyboard.
          case 'p': Keyboard.press(HexToBin(incomingBuf[1], incomingBuf[2])); break;
          case 'r': Keyboard.release(HexToBin(incomingBuf[1], incomingBuf[2])); break;
          case 'm': Mouse.move(HexToBin(incomingBuf[1], incomingBuf[2]), HexToBin(incomingBuf[3], incomingBuf[4]), 0); break;  // Moves the cursor on a connected computer.
          case 'c': Mouse.click(); break; //Sends a momentary click to the computer at the location of the cursor.
          case 'd': Mouse.click(); Mouse.click();break;
        }
        incomingBuf[0] = 0;
      }
      else if(incomingIndex < INCOMING_BUF_SIZE)
      {
        incomingBuf[incomingIndex++] = incomingByte;
      }
    }
  }
}
