#!/usr/bin/env python3
from PIL import Image, ImageFont, ImageDraw

# parameters
font_name = './AdonisC-Italic.ttf'
font_height = 9  # check after generation, can be different
characters_range = (',', '9')
# characters_range = (chr(0xb0), chr(0xb0))
invert_pixels = True
reverse_bits = True

# characters_range = (characters_range[0].encode().decode('koi8-r'), characters_range[1].encode().decode('cp1251'))
print(characters_range)
font_size = int(font_height / 0.75)
font_color = '#000000'  # HEX Black
font = ImageFont.truetype(font_name, font_size)
height = 0
characters = []
for c in range(ord(characters_range[0]), ord(characters_range[1]) + 1):
    character = chr(c)
    width, height = font.getsize(character)
    img = Image.new("RGBA", (width, height))
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), str(character), font=font, fill=font_color)
    characters.append((img, width, character))
height_bytes_num = ((height - 1) // 8) + 1
height_bits_num = height_bytes_num * 8
height_shift = height_bits_num - height
name = f'font_{height}_{ord(characters_range[0])}_{ord(characters_range[1])}'
with open(name + '.c', 'w', encoding='ascii') as f:
    f.write('#include <avr/pgmspace.h>\n')
    f.write('#include "custom_font.h"\n')
    f.write(f'\nconst uint8_t _{name} [] PROGMEM = {{\n')
    for img, width, character in characters:
        # print(width, height)
        f.write(f'// "{character if ord(character) < 128 else ord(character.encode("cp1251"))}"\n')
        for w in range(width):
            value = 0
            for h in range(height):
                value <<= 1
                try:
                    _, _, _, a = img.getpixel((w, h))
                    if a:
                        value |= 1
                except IndexError:
                    print(character)
            value <<= height_shift
            if reverse_bits:
                value = int('{:0{width}b}'.format(value, width=height_bits_num)[::-1], 2)
            if invert_pixels:
                value ^= 0xffffffffffffffff
            # print(bin(value))
            height_bytes = [f'0x{(value >> i * 8) & 0xff:02X}' for i in reversed(range(height_bytes_num))]
            f.write(', '.join(height_bytes) + ',\n')
    f.write(f'}};\n\nstatic const custom_font_char_shift_t _shift_{name} [] PROGMEM = {{\n')
    shift = 0
    for _, width, _ in characters:
        f.write(f'    {{.shift = {shift}, .width = {width}}},\n')
        shift += width * height // 8
    f.write(f'}};\n\ncustom_font_t {name} = {{\n')
    f.write(f'    .char_start = {ord(characters_range[0].encode("cp1251"))},\n')
    f.write(f'    .size = {len(characters)},\n')
    f.write(f'    .height = {height},\n')
    f.write(f'    .char_table = _{name},\n')
    f.write(f'    .shift_table = _shift_{name},\n')
    f.write('};\n')
