#include <stdio.h>

int main (void)
{

  // get live data
  //printf("%c%c%c%c%c",0xff,0xff,0x27,3,42);

  // read system parameter
  //printf("%c%c%c%c%c",0xff,0xff,0x30,3,0x33);

  // read sensor id
  //printf("%c%c%c%c%c",0xff,0xff,0x3a,3,0x3d);

  // read sensor id new
  //printf("%c%c%c%c%c",0xff,0xff,0x3c,3,0x3f);

  // read rain data
  printf("%c%c%c%c%c",0xff,0xff,0x57,3,0x5a);


  return 0;
}
