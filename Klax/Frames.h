#ifndef FRAMES_H
#define FRAMES_H

// Frame pour l'état OK
const uint32_t frameOK[] = {
  0x00000000,
  0x00000000,
  0x00000000
};

// Frame pour l'état perdu
const uint32_t frameLost[] = {
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF
};

// Frame pour l'état échec
const uint32_t frameFail[] = {
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA
};

#endif