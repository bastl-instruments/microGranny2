/**
 * 
 * _ __ ___  / ___|___ \  / _ \ 
 * | '_ ` _ \| |  _  __) || | | |
 * | | | | | | |_| |/ __/ | |_| |
 * |_| |_| |_|\____|_____(_)___/ 
 * 
 * 
 * microGranny 2.0
 * by Vaclav Pelousek      http://www.pelousek.net/
 * for Bastl Instruments   http://www.bastl-instruments.com/
 * 
 * based on WaveRP library Adafruit Wave Shield - Copyright (C) 2009 by William Greiman
 * -library heavily hacked - BIG THX https://code.google.com/p/waverp/
 * 
 * needs SD Fat library too https://code.google.com/p/sdfatlib/
 *
 *
 *
 * -thanks for understanding basics thru Mozzi library http://sensorium.github.io/Mozzi/
 * -written in Arduino + using SDFat library
 *
 * 
 * beta testing and help: Ondrej Merta, Ryba, HRTL
 *
 * monophonic granular sampler
 * 
 * Features:
 * -6 big play buttons, 6 function buttons, 4 knobs, rgb led, 4 digit 7 segment display, bunch of leds
 * -6 samples with full adjustment in 1 preset
 * -12 presets
 * -wav sample playback from microSD card
 * -record wav via line or onboard microphone
 * -hold button
 * -sample rate (tuned or free run)
 * -crush
 * -start, end possition 
 * -looping mode
 * -instant loop
 * -granular loop lenght
 * -shift speed (possitive or negative)
 * -enevlope (attack, release)
 * -MIDI Input + MIDI thru
 * -responsive to note, cc and clock (synchronize loop and grains)
 * -randomizer
 * -copy paste
 * -input & output volume knob
 * -power switch
 * -wooden enclosure
 * 
 * 
 * 
 */
#include <SdFat.h>
#include <WaveRP.h>
#include <SdFatUtil.h>
#include <ctype.h>
//#include <mozzi_analog.cpp>
#include <mg2HW.h>
//#include <MIDI.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>



///#include <envelope.h>


#define RECORD_RATE 22050 // try pot !!

#define MAX_FILE_SIZE 104857600UL  // 100 MB
//#define MAX_FILE_SIZE 1048576000UL // 1 GB
#define MIC_ANALOG_PIN 0 // Analog pin connected to mic preamp
// Voltage Reference Selections for ADC
//#define ADC_REFERENCE ADC_REF_AREF  // use voltage on AREF pin
#define ADC_REFERENCE ADC_REF_AVCC  // use 5V VCC

//#define DISPLAY_RECORD_LEVEL 1

#define MIDI_BAUD 31250
//------------------------------------------------------------------------------
// global variables
Sd2Card card;           // SD/SDHC card with support for version 2.00 features
SdVolume vol;           // FAT16 or FAT32 volume
SdFile root;            // volume's root directory
SdFile file;            // current file
WaveRP wave;            // wave file recorder/player
mg2HW hw;
//------------------------------------------------------------------------------



long seekTo;
unsigned char crush;
unsigned char volume;
long lastPosition;
unsigned char currentPreset=0;
unsigned char currentBank=0;
#define E_BANK 1001
#define E_PRESET 1002

void setup(void) {

  hw.initialize();
  initSdCardAndReport();
  //Serial.begin(9600);
  
  
 if(!EEPROM.read(1000)) playBegin("ZZ.WAV",7);
 else EEPROM.write(1000,0),currentPreset=EEPROM.read(E_PRESET),currentBank=EEPROM.read(E_BANK);
  initMidi();
   //Serial.begin(MIDI_BAUD);

  initMem(); 
 //  clearMemmory();
  restoreAnalogRead();
 // hw.freezeAllKnobs();
}

void restoreAnalogRead()
{
  ADCSRA=135; // default ARDUINO B10000111
}


void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
asm volatile ("  jmp 0");  
} 

void loop() {
readMidi();//,hw.displayText("midi");
 // readMidi();
  UI();
 // readMidi();
 // readMidi();
  updateSound();
 // readMidi();
 // readMidi();
}



