#pragma once

#ifdef UNIT_TEST
  #include <stdint.h>
  #include <string.h>
#else
  #include <Arduino.h>
#endif
#include <Mesh.h>

struct ChannelDetails {
  mesh::GroupChannel channel;
  char name[32];
};
