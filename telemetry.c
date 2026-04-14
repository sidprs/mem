#include <stdio.h>
#include <stdlib.h>

/*
  Telemetry for Tiny byte stream from TinyUart
  Packet Format 8 bytes per Packet
    little endian / big endian are ways to store structs



*/

/*

  difference between static and const
  having a way to store the difference between TLA power percent difference
  linear interpolation



*/

static int convertTLAPower(float* tla) {
  float power = 0.0f;

  return power;
}

int main(int argc, char* argv[]) {
  // the requirement is for the throttle lever angle to convert it to power
  // if its between 2 values then linear interpolate
  // if the difference between current and final percent is greater than 100%
  // then do not do the calculation in main it was a volatile tla and it was
  // being scaned using scanf in the main loop in a while loop this value was
  // being passed to convertTLAPower and then return the power based on 5
  // requirements
  //
  return 0;
}
