#include <ctime>
#include <iostream>
#include <stdio.h>
#include "sendTime.h"

using namespace std;


void sendTime() {

  FILE *file;
  file = fopen("/dev/ttyACM0", "w");  //Opening device file

  time_t rawtime;
  time (&rawtime);

  for (int i = 11; i < 19; i++){
    if (ctime(&rawtime)[i] != ':'){
      //time in format hhmmss
      int x = ctime(&rawtime)[i] - 48;
      cout<< x<< " ";
      fprintf(file, "%d", x);
      fprintf(file, "%c",',');
    }
  }

  fclose(file);
}
