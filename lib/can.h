#include <stdint.h>

typedef struct {
  uint32_t ID;
  uint8_t Length;
  uint8_t Data[8];
} tCanFrame;