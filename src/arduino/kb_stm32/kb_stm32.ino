#include <USBComposite.h>

USBHID HID;
USBCompositeSerial CompositeSerial;
HIDKeyboard Keyboard(HID);
HIDMouse Mouse(HID);

#define INCOMING_BUF_SIZE 16
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

char get_mouse_button(uint8_t param)
{
  if (param == 'r')
  {
    return MOUSE_RIGHT;
  }
  else
  {
    return MOUSE_LEFT;
  }
}

void move(int16_t x, int16_t y)
{
  bool sx = x < 0;
  if (sx)
  {
    x = -x;
  }
  bool sy = y < 0;
  if (sy)
  {
    y = -y;
  }
  bool swapped;
  if (y > x)
  {
    int16_t t = x;
    x = y;
    y = t;
    swapped = true;
  }
  else
  {
    swapped = false;
  }
  int16_t dx=0;
  int16_t dy;
  int16_t dx_prev=0;
  int16_t dy_prev=0;
  while (dx < x)
  {
    dx += random(3, 5);
    if (dx > x)
    {
      dx = x;
    }
    dy = (int32_t)dx * y / x;
    int8_t _x, _y;
    _x = dx - dx_prev;
    dx_prev = dx;
    _y = dy - dy_prev;
    dy_prev = dy;
    if (swapped)
    {
      int8_t t = _x;
      _x = _y;
      _y = t;
    }
    if (sx)
    {
      _x = -_x;
    }
    if (sy)
    {
      _y = -_y;
    }
    Mouse.move(_x, _y, 0);
    delay(10);
  }
  Mouse.move(0, 0, 0);
}

void setup()
{
  USBComposite.setProductId(0x1602);
  USBComposite.setVendorId(0x04d9);
  HID.begin(CompositeSerial, HID_KEYBOARD_MOUSE);
  delay(1000);
}

void loop()
{
   while (CompositeSerial.available())
   {
     if (CompositeSerial.available() > 0)
     {
       incomingByte = CompositeSerial.read();
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
           case 'C': Mouse.click(get_mouse_button(incomingBuf[1])); break; //Sends a momentary click to the computer at the location of the cursor.
           case 'P': Mouse.press(get_mouse_button(incomingBuf[1])); break;
           case 'R': Mouse.release(get_mouse_button(incomingBuf[1])); break;
           case 'M':
           {
             int16_t _x = (HexToBin(incomingBuf[1], incomingBuf[2]) << 8) | HexToBin(incomingBuf[3], incomingBuf[4]);
             int16_t _y = (HexToBin(incomingBuf[5], incomingBuf[6]) << 8) | HexToBin(incomingBuf[7], incomingBuf[8]);
             move(_x, _y);
             break;
           }
         }
         CompositeSerial.write((byte)0x01);
         CompositeSerial.write(incomingBuf, incomingIndex);
         CompositeSerial.write((byte)0x00);
         incomingBuf[0] = 0;
       }
       else if(incomingIndex < INCOMING_BUF_SIZE)
       {
         incomingBuf[incomingIndex++] = incomingByte;
       }
     }
   }
}
