#pragma once

#include <Arduino.h>
#include <Mesh.h>

struct ChannelDetails {
  mesh::GroupChannel channel;
  char name[32];
  uint8_t options; // low 2 bits: alert policy override; remaining bits reserved
};
