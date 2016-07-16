#include <Arduino.h>
#include <OSCMessage.h>

/**
 * Baudrate for serial communication. Don't use to low values as it will uneccessaryly block the program.
 */
#define BAUD_RATE 115200
/**
 * The number of piezos attached to the Arduino. It is assumed the 1st piezo is attached to A0, the 2nd to A1 and so on.
 */
#define NUM_PADS 5


//Teensy and Leonardo variants have special USB serial
#if defined(CORE_TEENSY)|| defined(__AVR_ATmega32U4__)
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial(Serial);
#else
// any hardware serial port
#include <SLIPEncodedSerial.h>
SLIPEncodedSerial SLIPSerial(Serial);
#endif

/**
 * Helper class fo OSC communication.
 */
class DrumpadOscHelper {
private:
    String baseAddress = "/drum/";
    OSCMessage msg;
    #define ADDRESS_BUFFER_LENGTH 20
    char charBuffer[ADDRESS_BUFFER_LENGTH];

    /**
     * Internal helper to send an integer value to a specific address.
     * @param value
     * @param address
     */
    void sendInt(int value, String address) {
        if (value < 10)
            return;

        address.toCharArray(this->charBuffer, ADDRESS_BUFFER_LENGTH);

        this->msg.setAddress(this->charBuffer);
        this->msg.add((int32_t) value);

        SLIPSerial.beginPacket();
        this->msg.send(SLIPSerial); // send the bytes to the SLIP stream
        SLIPSerial.endPacket(); // mark the end of the OSC Packet
        this->msg.empty(); // free space occupied by message
    }

public:
    /**
     * Output a "bang" OSC message for the given drumpad and amplitude.
     * @param value the amplitude of the bang.
     * @param padIndex the number of the pad.
     */
    void outputBang(int value, int padIndex) {
        String fullAddress = baseAddress + padIndex + "/bang";
        sendInt(value, fullAddress);
    }

    /**
     * Output a "volume" OSC message for the given drumpad and amplitude.
     * @param value the amplitude of the bang.
     * @param padIndex the number of the pad.
     */
    void outputVolume(int value, int padIndex) {
        String fullAddress = baseAddress + padIndex + "/volume";
        sendInt(value, fullAddress);
    }

};

/**
 * Helper class for maxima detection.
 */
class Envelope {
private:
    boolean falling = false;

protected:
    int oldValue = 1024;

public:
    /**
     * This function tells you whether the previously given value was a local maxima.
     * @param newValue the current value measured on the analog port.
     * @return {@code true} if the previously given value was a local maxima; {@code false} else.
     */
    boolean isLocalMaximum(int newValue) {
        boolean ret;

        ret = !this->falling && (newValue < this->oldValue);
        this->falling = newValue < oldValue;
        this->oldValue = newValue;

        return ret;
    }
};

/**
 * This class represents one drumpad connected to the Arduino.
 */
class DrumPad {
private:
    uint8_t analogPort;

    int oldValue;
    unsigned long lastBangTime = 0;
    int lastBangAmp = 0;

    Envelope volumeEnv;
    Envelope bangEnv;
    DrumpadOscHelper oscHelper;

    /**
     * Helper function for reverberate suppression.
     * @param thisAmp the current amplitude measured at the analog port.
     * @return {@code true} if the current amplitude is NOT caused by reverberate; {@code false} else.
     */
    boolean isNotReverberate(int thisAmp) {
        unsigned long timeDiff = millis() - lastBangTime;

        float thisAmpFloat = thisAmp;
        float threshold = (float)lastBangAmp * 50 / timeDiff;

        return thisAmpFloat > threshold;
    }

public:
    /**
      * Constructor
      * @param analogPort the analog port of your Arduino which the piezo sensor is attached to.
      */
    DrumPad(uint8_t analogPort) {
        this->analogPort = analogPort;
    }

    /**
     * Check if the drumpad was hit (Call this in your loop() function).
     * If a hit on the drumpad is detected the "bang" message is sent over OSC for the current drumpad. If not, but the
     * signal still contains a local maxima on the current value, the "volume" message is sent.
     */
    void process() {
        int currentValue = analogRead(this->analogPort);

        if (volumeEnv.isLocalMaximum(currentValue)) {
            if (bangEnv.isLocalMaximum(currentValue))
            if (this->isNotReverberate(this->oldValue)) {
                this->oscHelper.outputBang(this->oldValue, this->analogPort);
                this->lastBangTime = millis();
                this->lastBangAmp = this->oldValue;
            }

            this->oscHelper.outputVolume(this->oldValue, this->analogPort);
            this->oldValue = currentValue;
        }
    }
};

DrumPad* drumPads[NUM_PADS];

void setup() {
    // Setup SLIP
    SLIPSerial.begin(BAUD_RATE);

    // Setup the individual drumpads
    for (uint8_t i = 0; i < NUM_PADS; i++)
        drumPads[i] = new DrumPad(i);
}

void loop() {
    // Process all drumpads
    for (int i = 0; i < NUM_PADS; i++)
        drumPads[i]->process();
}
