#ifndef VWTP20_H
#define VWTP20_H

class VWTP20 {
public:
  VWTP20();
  ~VWTP20();
  void Connect();

private:
  void setup();
};

void readPacket();

#endif
