#include <OSCMessage.h>

#define OSC_ADDRESS "/drum/0/"
#define BAUD_RATE 115200

//Teensy and Leonardo variants have special USB serial
#if defined(CORE_TEENSY)|| defined(__AVR_ATmega32U4__)
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial(Serial);
#else
// any hardware serial port
#include <SLIPEncodedSerial.h>
SLIPEncodedSerial SLIPSerial(Serial);
#endif

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

class BangEnvelope : Envelope {
  private:
    boolean aimed = true;
    int aimThreshold = 2;

  public:
    virtual boolean isLocalMaximum(int newValue) {
      if(this->aimed && Envelope::isLocalMaximum(newValue)) {
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

int oldValue;
Envelope volumeEnv;
BangEnvelope bangEnv;

OSCMessage msg;

void setup() {
  // put your setup code here, to run once:
  SLIPSerial.begin(BAUD_RATE);

  pinMode(13, OUTPUT);
}

void loop() {
  int currentValue = analogRead(A0);

  if (volumeEnv.isLocalMaximum(currentValue)) {
    if (bangEnv.isLocalMaximum(currentValue)) {
      outputBang(oldValue);
    }
    outputVolume(oldValue);
    oldValue = currentValue;
  }

}

void outputBang(int value) {
  sendInt(value, "/drum/0/bang");
}

void outputVolume(int value) {
  sendInt(value, "/drum/0/volume");
}

void sendInt(int value, char* address) {
  if (value < 10)
    return;

  msg.setAddress(address);
  msg.add((int32_t)value);

  SLIPSerial.beginPacket();
  msg.send(SLIPSerial); // send the bytes to the SLIP stream
  SLIPSerial.endPacket(); // mark the end of the OSC Packet
  msg.empty(); // free space occupied by message
}

