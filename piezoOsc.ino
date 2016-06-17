#include <OSCMessage.h>

#define BAUD_RATE 115200
#define NUM_PADS 4
#define ADDRESS_BUFFER_LENGTH 20

//Teensy and Leonardo variants have special USB serial
#if defined(CORE_TEENSY)|| defined(__AVR_ATmega32U4__)
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial(Serial);
#else
// any hardware serial port
#include <SLIPEncodedSerial.h>
SLIPEncodedSerial SLIPSerial(Serial);
#endif

String baseAddress = "/drum/";
OSCMessage msg;
char charBuffer[ADDRESS_BUFFER_LENGTH];

void outputBang(int value, int padIndex) {
  String fullAddress = baseAddress + padIndex + "/bang";
  sendInt(value, fullAddress);
}

void outputVolume(int value, int padIndex) {
  String fullAddress = baseAddress + padIndex + "/volume";
  sendInt(value, fullAddress);
}

void sendInt(int value, String address) {
  if (value < 10)
    return;

  address.toCharArray(charBuffer, ADDRESS_BUFFER_LENGTH);

  msg.setAddress(charBuffer);
  msg.add((int32_t)value);

  SLIPSerial.beginPacket();
  msg.send(SLIPSerial); // send the bytes to the SLIP stream
  SLIPSerial.endPacket(); // mark the end of the OSC Packet
  msg.empty(); // free space occupied by message
}

class Envelope {
  private:
    boolean falling = false;

  protected:
    int oldValue = 1024;

  public:
    virtual boolean isLocalMaximum(int newValue) {
      boolean ret = false;

      ret = !this->falling && (newValue < this->oldValue);
      this->falling = newValue < oldValue;
      this->oldValue = newValue;

      return ret;
    }
};

class BangEnvelope : public Envelope {
  private:
    boolean aimed = true;
    int aimThreshold = 2;

  public:
    virtual boolean isLocalMaximum(int newValue) {
      boolean localMax = Envelope::isLocalMaximum(newValue);

      if(this->aimed && localMax) {
        this->aimed = false;
        return true;
      }

      if (newValue < aimThreshold)
        this->aimed = true;

      if (this->aimed)
        digitalWrite(13, HIGH);
      else
        digitalWrite(13, LOW);

      return false;
    }
};

class DrumPad {
private:
  int analogPort;
  int oldValue;
  Envelope volumeEnv;
  BangEnvelope bangEnv;

public:
  DrumPad(int analogPort) {
    this->analogPort = analogPort;
  }

  void process() {
    int currentValue = analogRead(this->analogPort);

    if (volumeEnv.isLocalMaximum(currentValue)) {
      if (bangEnv.isLocalMaximum(currentValue))
        outputBang(this->oldValue, this->analogPort);
      outputVolume(this->oldValue, this->analogPort);
      this->oldValue = currentValue;
    }
  }
};

DrumPad* drumPads[NUM_PADS];

void setup() {
  // put your setup code here, to run once:
  SLIPSerial.begin(BAUD_RATE);

  for (int i = 0; i < NUM_PADS; i++)
    drumPads[i] = new DrumPad(i);

  pinMode(13, OUTPUT);
}

void loop() {
  for (int i = 0; i < NUM_PADS; i++)
    drumPads[i]->process();
}
