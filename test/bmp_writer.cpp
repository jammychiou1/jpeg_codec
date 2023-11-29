#include <string>

#include "bmp_writer.h"
int main() {
  bmp_writer wr;
  wr.new_file("test.bmp", 10, 15);
}
