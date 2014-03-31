// op qr st   uv wx yz
unsigned char incomingByte,note;
boolean ignore, comandOff,slave,cc; //,midiNoteOn  noteOn,
unsigned char state=0;
unsigned char channel=0;
int clockCounter;
#define MIDI_CHANNEL 1023
#define POLYPHONY NUMBER_OF_VOICES
unsigned char notesInBuffer=ZERO;
boolean thereIsNoteToPlay;
unsigned char sound, activeSound;

#define BUFFER_SIZE 16
unsigned char midiBuffer[BUFFER_SIZE];
#define EMPTY 255
unsigned char midiVelocity;
unsigned char fromBuffer;
unsigned char instantLoop;
boolean sustain;
boolean legato;
unsigned char channelSide=0;
unsigned char sideNote=1;
unsigned char sideDecay;
//int pitchBendNow;   
int sampleRateNow;
unsigned char setting;
int attackInterval, releaseInterval;
//long legatoPosition;
PROGMEM prog_uint16_t noteSampleRateTable[49]={/*0-C*/
  2772,2929,3103,3281,3500,3679,3910,4146,4392,4660,4924,5231,5528,5863,6221,6579,6960,7355,7784,8278,8786,9333,9847,10420,11023,11662,12402,13119,13898,14706,15606,16491,17550,18555,19677,20857,22050,23420,24807,26197,27815,29480,29480,29480,29480,29480,29480,29480,/*48-C*/29480};


void shiftBufferLeft(unsigned char from){
  for(int i=from;i<notesInBuffer;i++){
    midiBuffer[i]=midiBuffer[i+1]; 
  }
}

void shiftBufferRight(){
  for(int i=notesInBuffer;i>ZERO;i--){
    midiBuffer[i]=midiBuffer[i-1]; 
  }
}

boolean isThereNoteToPlay(){
  return thereIsNoteToPlay;

}
unsigned char noteToPlay(){
  thereIsNoteToPlay=false;
  return midiBuffer[fromBuffer];
}


void putNoteIn(unsigned char note){
  if(note<6) hw.freezeAllKnobs();
  if(notesInBuffer==BUFFER_SIZE-1) removeNote(midiBuffer[BUFFER_SIZE-1]);
  removeNote(note); // check if the note is already in the buffer if yes remove it
  if(notesInBuffer<BUFFER_SIZE){ //put the note to the first position
    if(notesInBuffer>ZERO){ 
      shiftBufferRight();
    }
    midiBuffer[ZERO]=note; // put the last note to the first place 
    notesInBuffer++;
    thereIsNoteToPlay=true;
    fromBuffer=ZERO;
  }


  if(thereIsNoteToPlay) {
    if(legato && note>=23 && note<66 && notesInBuffer>1) sound=note,sampleRateNow=(pgm_read_word_near(noteSampleRateTable+sound-23)),wave.setSampleRate(sampleRateNow);
    else playSound(midiBuffer[ZERO]);
  }
  //  hw.freezAllKnobs();
}

void clearBuffer(){
  for(int i=ZERO;i<BUFFER_SIZE;i++) midiBuffer[i]=EMPTY;
  notesInBuffer=0;
}
boolean removeNote(unsigned char note){
  if(notesInBuffer>ZERO){ 
    unsigned char takenOut;
    boolean takeOut=ZERO;

    for(int i=ZERO;i<notesInBuffer;i++){
      if(midiBuffer[i]==note) takeOut=true, takenOut=i;
    } 

    if(takeOut){
      shiftBufferLeft(takenOut);
      notesInBuffer--;
      for(int i=notesInBuffer;i<BUFFER_SIZE;i++) midiBuffer[i]=EMPTY;
      return true;
    }
    else return false;

  }
}
unsigned char putNoteOut(unsigned char note){
  if(note<6) hw.freezeAllKnobs();

  if(removeNote(note)){

    if(notesInBuffer>0){
      if(midiBuffer[ZERO]!=sound) {
        if(legato && midiBuffer[ZERO]>=23 && midiBuffer[ZERO]<66) sound=midiBuffer[ZERO],sampleRateNow=(pgm_read_word_near(noteSampleRateTable+sound-23)),wave.setSampleRate(sampleRateNow);
        else playSound(midiBuffer[ZERO]),instantLoop=0;
      } //legatoPosition=wave.getCurPosition(),
    }
    else if(!sustain) stopEnvelope(),instantLoop=0;
    return midiBuffer[ZERO];

  }




}



void initMidi(){
  clearBuffer();
  readMidiChannel();
  Serial.begin(MIDI_BAUD);

}
#define SIDE_CHANNEL 1022
#define SIDE_NOTE 1021
#define SIDE_DECAY 1020

unsigned char controler, CCvalue;
void readMidiChannel(){

  channelSide=EEPROM.read(SIDE_CHANNEL);

  sideNote=EEPROM.read(SIDE_NOTE);
  sideDecay=EEPROM.read(SIDE_DECAY);
  channel=EEPROM.read(MIDI_CHANNEL);
  if(channel>16) EEPROM.write(MIDI_CHANNEL,0), channel=0;

  showValue(channel+1);
  hw.displayChar('C',0);
  hw.displayChar('H',1); 

  hw.update();
  for(int i=0;i<6;i++){
    if(hw.buttonState(bigButton[i])){
      if(hw.buttonState(UP) && hw.buttonState(DOWN)) sideDecay=i, EEPROM.write(SIDE_DECAY,sideDecay), showValue(sideDecay), hw.displayChar('S',0), hw.displayChar('D',1);
      else if(hw.buttonState(UP)) channelSide=i+6*hw.buttonState(FN), EEPROM.write(SIDE_CHANNEL,channelSide), showValue(channelSide+1), hw.displayChar('S',0), hw.displayChar('C',1); 
      else if(hw.buttonState(DOWN)) sideNote=i+60*hw.buttonState(FN), EEPROM.write(SIDE_NOTE,sideNote), showValue(sideNote), hw.displayChar('S',0), hw.displayChar('N',1); 
      else channel=i+6*hw.buttonState(FN),EEPROM.write(MIDI_CHANNEL,channel);
    }
  }

  if(wave.isPlaying()){
    while(!wave.isPaused()) hw.update();
  }
  stopSound();

}

long lastClockPosition, clockLength;
//unsigned char pByte1,pByte2;
//boolean pb;
boolean side;
int bytesAvailable;
void readMidi(){


  //channel=map(analogRead(4),0,1024,0,16);
  while(Serial.available() > 0){
    bytesAvailable=Serial.available();
    if (bytesAvailable <= 0) return;
    if(bytesAvailable>=64) Serial.flush(); // If the buffer is full -> Don't Panic! Call the Vogons to destroy it.
    else {

      // read the incoming byte:

      unsigned char incomingByte = Serial.read();
      Serial.write(incomingByte); // thru

      switch (state){      
      case 0:
        if( handleRealTime(incomingByte));

        else if(incomingByte < 128) { // if running status byte
          if(!ignore){ //
            if(cc) controler=incomingByte;
            // else if(pb) pByte1=incomingByte;
            else note=incomingByte;
            state=2;
          }
        }

        if (incomingByte== (144 | channel)){  // look for as status-byte, our channel, note on
          state=1;
          ignore=false;
          cc=false;
          comandOff=false;
          side=false;
          // pb=false;
        }

        else if (incomingByte== (0x80 | channel)){ // look for as status-byte, our channel, note off
          state=1;
          ignore=false;
          comandOff=true;
          cc=false;
          side=false;
          //  pb=false;

        }
        else if (incomingByte== (0xB0 | channel)){ // look for as status-byte, our channel, controlchange
          state=1;
          ignore=false;
          cc=true;
          comandOff=false;
          side=false;
          // comandOff=true;

        }
        /*
        else if (incomingByte== (0xE0 | channel)){ //pitchBend
         state=1;
         ignore=false;
         cc=false;
         pb=true;
         }
         */

        else if (incomingByte== (144 | channelSide)){  // look for as status-byte, our channel, note on
          state=1;
          ignore=false;
          cc=false;
          side=true;
          // pb=false;
        }

        else if(incomingByte>127){
          ignore=true; 
        }



        break;

      case 1:
        // get the note to play or stop
        if(incomingByte < 128) {
          if(cc) controler=incomingByte;
          // else if(pb) pByte1=incomingByte;
          else note=incomingByte;
          state=2;
        }
        else{
          if(!handleRealTime(incomingByte)) state = 0; 
          // reset state machine as this should be a note number
        }
        break;

      case 2: // get the velocity / cc value

        if(incomingByte < 128) {
          if(cc){
            CCvalue=incomingByte;
            proceedCC(controler,CCvalue);
          }
          //else if(pb) pByte2=incomingByte, proceedPB(pByte1,pByte2);
          else if(side) proceedSideChain(note),side=false;

          else{
            if(incomingByte!=0){ 
              if(comandOff){
                putNoteOut(note);
                comandOff=false;

              }
              else{
                midiVelocity=incomingByte;

                putNoteIn(note);
                // hw.freezeAllKnobs();

                //  midiNoteOn=true;
              }
            }
            else{ 
              putNoteOut(note);
              comandOff=false;

              // midiNoteOn=false;
            }
          }

          state = 0; 

        }
        else if(!handleRealTime(incomingByte)) state = 0; 

        // state=0;

      } 
      //    Serial.write(incomingByte);
    }
  }
}

void proceedSideChain(unsigned char _note){
  if(_note==sideNote){
    if(sideDecay==0) startEnvelope(midiVelocity,attackInterval);
    else startEnvelope(midiVelocity,sideDecay<<2);
  }
}
boolean handleRealTime(unsigned char _incomingByte){
  if(_incomingByte==0xF8){ //clock
    clockCounter++;
    // sync shiftSpeed - unnecessary 
    // clockLength=abs(wave.getCurPosition()-lastClockPosition);
    // sync shiftSpeed - unnecessary 
    slave=true;
    return true;
  }
  else if(_incomingByte==0xFA){ //start
    clockCounter=0;
    slave=true;
    return true;

  }
  else if(_incomingByte==0xFC){ //stop
    clockCounter=0;
    slave=true;
    return true;
  } 
  else return false;
}

#define SUSTAIN_PEDAL_BYTE 64
#define PRESET_BY_CC_BYTE 0
#define RANDOMIZE_BYTE 127

#define CONTROL_CHANGE_BITS 7
#define CONTROL_CHANGE_OFFSET 102
#define CONTROL_CHANGE_OFFSET_2 110

void proceedCC(unsigned char _number,unsigned char _value){

  if(_number==1) setVar(activeSound,LOOP_LENGTH,scale(_value,CONTROL_CHANGE_BITS,variableDepth[LOOP_LENGTH])), hw.unfreezeAllKnobs(),renderTweaking(1),hw.freezeAllKnobs(); //modwheel
  if(_number==SUSTAIN_PEDAL_BYTE){ 
    sustain=_value>>6;
    if(!sustain && notesInBuffer==0) stopEnvelope(),instantLoop=0;  
  }

  if(_number==PRESET_BY_CC_BYTE) loadPreset(currentBank,myMap(_value,128,NUMBER_OF_PRESETS));

  // else if(_number==RANDOMIZE_BYTE) randomize(activeSound);

  else if(_number>=CONTROL_CHANGE_OFFSET && _number<CONTROL_CHANGE_OFFSET_2){
    _number=_number-CONTROL_CHANGE_OFFSET;
    setVar(activeSound,_number,scale(_value,CONTROL_CHANGE_BITS,variableDepth[_number]));  
    hw.unfreezeAllKnobs();
    renderTweaking(_number/VARIABLES_PER_PAGE);
    hw.freezeAllKnobs();
  }

}


/*
void proceedPB(unsigned char _byte1,unsigned char _byte2){
 int pitchBendNow=word(_byte1,_byte2)-8192; 
 wave.setSampleRate(sampleRateNow+pitchBendNow);
 }
 */

