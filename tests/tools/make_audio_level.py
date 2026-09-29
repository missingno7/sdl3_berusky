#!/usr/bin/env python3
"""
Writes tests/levels/audio_events.lv3 - a level for the audio regression test
(tests/scripts/20_audio.txt): one corridor where every step to the right
does one thing that has a sound in the DOS game.

    python tests/tools/make_audio_level.py

Row 10, from x = 1 (bug 1 walks to the right):
   2 no floor           steps_background
   3 floor 0            steps_mud
   4 floor 10           steps_marble
   5-8 keys             pickup (keys 1-4)
   9 pickax, 10 stone 0 pickup, iron_stone
  11 pickax, 12 stone 1 pickup, blue_stone
  13 color key, 14 classic color door   pickup, unlock + door_open
  15 color key, 16 cyber color door     pickup, unlock
  17 classic color passage              door_close (in), door_close (out)
  19 cyber color passage                blue_flash (out)
  21 classic one-pass door              door_close (out)
  23 cyber one-pass door                blue_flash (out)
  25 explosive, 26 box                  explosion
  27 box                                push (twice)
Then down: (28,11) the fifth key - exit_open, (28,12) the exit - level_done.
Bug 2 sits at (1,13) for Tab.
"""

import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), 'levels', 'audio_events.lv3')

W, H = 32, 21
NO_FLOOR = 0xffff
NO_PLAYER = 0xffff

P_BOX, P_TNT, P_WALL, P_EXIT, P_STONE, P_KEY, P_MATTOCK = 6, 7, 8, 9, 10, 11, 12
P_KEY1 = 13
P_DOOR1_H_Z = 23
P_ID_DOOR1_H_Z = 43
P_DV_H_O = 58
CLASSIC, CYBER = 0, 1

floor = [[(NO_FLOOR, 0) for x in range(W)] for y in range(H)]
items = [[(0, 0) for x in range(W)] for y in range(H)]
players = [[NO_PLAYER for x in range(W)] for y in range(H)]

# the corridor floor: marble, the steps station has its own
for x in range(1, 30):
    floor[10][x] = (0, 10)
floor[10][2] = (NO_FLOOR, 0)
floor[10][3] = (0, 0)
for y in (11, 12):
    floor[y][28] = (0, 10)
floor[13][1] = (0, 10)

row = {
    5: (P_KEY, 0), 6: (P_KEY, 0), 7: (P_KEY, 0), 8: (P_KEY, 0),
    9: (P_MATTOCK, 0), 10: (P_STONE, 0),
    11: (P_MATTOCK, 0), 12: (P_STONE, 1),
    13: (P_KEY1, 0), 14: (P_DOOR1_H_Z, CLASSIC),
    15: (P_KEY1, 0), 16: (P_DOOR1_H_Z, CYBER),
    17: (P_ID_DOOR1_H_Z, CLASSIC),
    19: (P_ID_DOOR1_H_Z, CYBER),
    21: (P_DV_H_O, CLASSIC),
    23: (P_DV_H_O, CYBER),
    25: (P_TNT, 0), 26: (P_BOX, 0),
    27: (P_BOX, 0),
}
for x, it in row.items():
    items[10][x] = it
items[11][28] = (P_KEY, 0)
items[12][28] = (P_EXIT, 0)

# walls around the corridor
for x in range(0, 31):
    for y in (9, 11):
        if not (y == 11 and x == 28):
            items[y][x] = (P_WALL, 0)
items[10][0] = (P_WALL, 0)
items[10][30] = (P_WALL, 0)

players[10][1] = 0
players[13][1] = 1

data = bytearray()
data += b'Level 3 (C) Anakreon 1999'.ljust(30, b'\0')
data += bytes([0, 0])            # background, music (the DOS game never read it)
data += bytes(5)                 # players' rotation
data += bytes(100)               # reserved
for y in range(H):
    for x in range(W):
        f = floor[y][x]
        data += struct.pack('<10H', f[0], f[1], 0, 0, 0, 0, 0, 0, 0, 0)
for y in range(H):
    for x in range(W):
        i = items[y][x]
        data += struct.pack('<10H', i[0], i[1], 0, 0, 0, 0, 0, 0, 0, 0)
for y in range(H):
    for x in range(W):
        data += struct.pack('<H', players[y][x])

assert len(data) == 28361, len(data)
with open(OUT, 'wb') as f:
    f.write(data)
print('wrote', OUT)
