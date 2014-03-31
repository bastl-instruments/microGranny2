#define LOOP_LENGTH_SHIFT 4 //8 for possition based
#define SHIFT_SPEED_SYNC_SHIFT 5
#define SHIFT_SPEED_SHIFT 7

boolean whileShow;
long whileTime;

#define WHILE_DURATION 1400
#define NUMBER_OFFSET 48
#define BIG_LETTER_OFFSET 65
#define SMALL_LETTER_OFFSET 97
#define NUMBER_OF_LETTERS 3

#define PAGE_KNOB_LABEL_OFFSET 4
unsigned char lastMoved;
boolean combo;
boolean shift;

boolean tuned=true;

#define TUNED_BIT 0 //1
#define LEGATO_BIT 1 //0
#define REPEAT_BIT 2 //1
#define SYNC_BIT 3 //1
#define SHIFT_DIR_BIT 4 //0

//1101
//TUNED, LEGATO, REPEAT, SYNC and RANDOM SHIFT
long startGranule;



#define DEFAULT_VELOCITY 127

long longTime;
boolean longPress;
#define LONG_PERIOD 600
#define MOVE_PERIOD 400

//unsigned char updt;
void UI(){

  // if(!rec)  
  // hw.updateDisplay();
  if(rec){
    hw.setColor(RED); 
    hw.updateDisplay();
    hw.updateButtons();
    hw.displayText("slct");
    renderRecordRoutine();

    //   updt++;
    // if(updt>1) updt=0, 
    hw.updateDisplay();
    //       hw.updateKnobs();

    //  hw.updateDisplay();
    // delay(1);
  }
  else{

    hw.updateDisplay();
    hw.updateButtons();

    hw.updateKnobs();
    renderCombo();
    renderHold();

    if(!combo){
      if(hw.justReleased(PAGE)){
        if(++page>=2) page=0;
        hw.freezeAllKnobs();
      } 

      if(hw.justPressed(REC)){
        noSound();
        hold=false;
        dimLeds();
        clearBuffer();
        rec=true;  
        wave.adcInit(RECORD_RATE, MIC_ANALOG_PIN, ADC_REFERENCE);
      }

      renderBigButtons();
    }
    hw.setLed(REC,false);
    hw.setColor(page+2); 
    renderSmallButtons();
    renderTweaking(page);
    renderKnobs();
    renderDisplay();
  } 


}
void dimLeds(){
  for(int i=0;i<NUMBER_OF_SOUNDS;i++) hw.setLed(bigButton[i],false);
  hw.setLed(HOLD,false);
}
void playSound(unsigned char _sound){
  sound=_sound;

  if(_sound<=83 && _sound>=23){
    loadName(activeSound);
    startPlayback(activeSound);
    loadValuesFromMemmory(_sound);
    startEnvelope(midiVelocity,attackInterval);
  }
  else if(_sound<6){

    activeSound=_sound;
    loadName(activeSound);
    startPlayback(activeSound);
    loadValuesFromMemmory(activeSound);
    startEnvelope(DEFAULT_VELOCITY,attackInterval);



  }

}

//long sizeOfFile;

//samples per second*1000 - millisecond - how many samples?
//x= sample Rate*number of samples / 1000
long seekPosition;
void setEnd(unsigned char _sound){
  endIndex=getVar(_sound,END);
  if(sync)  endIndex=pgm_read_word_near(usefulLengths+(endIndex>>6)+1);
  else{
    //ending=true;
    if(endIndex<1022){
      if(endIndex<startIndex) endIndex=startIndex+1;     
      endPosition=endIndex*startGranule;
    }
    else if(shiftSpeed<0 && loopLength!=0) endPosition=file.fileSize()-(sampleRateNow*loopLength)/500,seekPosition=endPosition;
    else endPosition=file.fileSize(),seekPosition=startPosition;
    // else  

  }
}
int pitch;

void loadValuesFromMemmory(unsigned char _sound){

  unsigned char  notePitch=255;

  long sizeOfFile=file.fileSize();
  if(!tuned && _sound>=23 && _sound <=73) startIndex=_sound-23, _sound=activeSound, startGranule=sizeOfFile/60;
  else if(_sound>=23 && _sound<66) notePitch=_sound-23,_sound=activeSound,startGranule=sizeOfFile/1024, startIndex=getVar(_sound,START);
  else _sound=activeSound,startGranule=sizeOfFile/1024, startIndex=getVar(_sound,START);
  startPosition=startIndex*startGranule;
  setSetting(_sound);
  attackInterval=getVar(_sound,ATTACK);
  releaseInterval=getVar(_sound,RELEASE);
  wave.pause();

  if(notePitch!=255){
    sampleRateNow=(pgm_read_word_near(noteSampleRateTable+notePitch));
  }
  else{
    sampleRateNow=valueToSampleRate(getVar(_sound,RATE));
  }
  wave.setSampleRate(sampleRateNow);//+pitchBendNow);
  crush=getVar(_sound,CRUSH)<<1;
  wave.setCrush(crush);
  if(sync) loopLength=pgm_read_word_near(usefulLengths+(getVar(_sound,LOOP_LENGTH)>>3));
  else loopLength=getVar(_sound,LOOP_LENGTH)<<LOOP_LENGTH_SHIFT;
  //if(sync) shiftSpeed=((long)getVar(_sound,SHIFT_SPEED)-128)<<SHIFT_SPEED_SYNC_SHIFT;
  //else 
  shiftSpeed=((long)getVar(_sound,SHIFT_SPEED)-128)<<SHIFT_SPEED_SHIFT;
  setEnd(_sound);
  wave.seek(seekPosition);
  wave.resume();


}
int valueToSampleRate(int _value){
  pitch=myMap(_value,1023,420);
  if(tuned){
    pitch/=10;
    return (pgm_read_word_near(noteSampleRateTable+pitch));
  }
  else{
    int pitchStep=(pgm_read_word_near(noteSampleRateTable+pitch/10+1) - pgm_read_word_near(noteSampleRateTable+pitch/10))/10;
    return pgm_read_word_near(noteSampleRateTable+pitch/10)+(pitch%10)*pitchStep;
  } 
}
void setSetting(unsigned char _sound){
  setting=getVar(_sound,SETTING);
  repeat=bitRead(setting,REPEAT_BIT);
  tuned=bitRead(setting,TUNED_BIT);
  shiftDir=bitRead(setting,SHIFT_DIR_BIT);
  sync=bitRead(setting,SYNC_BIT);
  legato=bitRead(setting,LEGATO_BIT);
  if(!tuned) legato=false;
  if(slave && sync) sync=true;
  else sync=false;
  // 
}


void startPlayback(unsigned char _sound){

  //if(wave.isPlaying()) stopSound();
  stopSound();
  playBegin(name,_sound),lastPosition=wave.getCurPosition();
  lastMoved=5;
  showSampleName();

}


void renderTweaking(unsigned char _page){

  if(wave.isPlaying()){
    unsigned char _sound=activeSound;
    switch(_page){
    case 0:
      {
        if(!hw.knobFreezed(0)){
          /*
          if(tuned) sampleRateNow=(pgm_read_word_near(noteSampleRateTable+myMap(getVar(_sound,RATE),1023,43)));
           else sampleRateNow=(myMap(getVar(_sound,RATE),1023,29900)+100);
           */
          sampleRateNow=valueToSampleRate(getVar(_sound,RATE));
          /*
          pitch=myMap(getVar(_sound,RATE),1023,420);
           if(tuned) pitch/=10,sampleRateNow=(pgm_read_word_near(noteSampleRateTable+pitch));
           else{
           int pitchStep=(pgm_read_word_near(noteSampleRateTable+pitch/10+1) - pgm_read_word_near(noteSampleRateTable+pitch/10))/10;
           sampleRateNow= pgm_read_word_near(noteSampleRateTable+pitch/10)+(pitch%10)*pitchStep;
           }
           */


          if(sound<7 || !tuned) wave.setSampleRate(sampleRateNow);//+pitchBendNow);
        }
        if(!hw.knobFreezed(1)){
          wave.setCrush(getVar(_sound,CRUSH)<<1);
        }

        if(!hw.knobFreezed(2)) attackInterval=getVar(_sound,ATTACK);
        if(!hw.knobFreezed(3)) releaseInterval=getVar(_sound,RELEASE);

      }
      break;
    case 1:
      if(sync) loopLength=pgm_read_word_near(usefulLengths+(getVar(_sound,LOOP_LENGTH)>>3));
      else loopLength=getVar(_sound,LOOP_LENGTH)<<LOOP_LENGTH_SHIFT;
      if(!hw.knobFreezed(1)) {
        //if(sync) shiftSpeed=((long)getVar(_sound,SHIFT_SPEED)-128)<<SHIFT_SPEED_SYNC_SHIFT;
        //else 
        shiftSpeed=((long)getVar(_sound,SHIFT_SPEED)-128)<<SHIFT_SPEED_SHIFT;
      }
      if(!hw.knobFreezed(2)){ //splice here??
        startIndex=getVar(_sound,START);
        startPosition=startIndex*startGranule;
      }
      if(!hw.knobFreezed(3)) setEnd(_sound);


      break;

    }
  }


}
void showValue(int _value){
  hw.displayNumber(abs(_value));

  if(!(_value/100)) hw.lightNumber(VOID,1);
  if(!((_value%100)/10)){
    if(_value<100) hw.lightNumber(VOID,2);
    if(_value<0){ 
      hw.lightNumber(MINUS,2);
    }
  }
  else  if(_value<0) {
    if(_value<=-100) hw.lightNumber(MINUS,0);
    else if(_value<=-10) hw.lightNumber(MINUS,1);
    else hw.lightNumber(MINUS,2);
  }
  if(_value>=1000) hw.lightNumber(1,0),hw.lightNumber(0,1);

}
void renderCombo(){

  for(int i=0;i<NUMBER_OF_BIG_BUTTONS;i++){
    if(hw.buttonState(PAGE) && hw.justPressed(bigButton[i])) combo=true,loadPreset(currentBank,i);
  }
  if(hw.buttonState(PAGE) && hw.justPressed(UP)) combo=true,loadPreset(currentBank+1,currentPreset);//,showForWhile("pr  "),hw.setDot(1,true),hw.displayChar(presetName[2],3),hw.displayChar(presetName[1],2),clearIndexes();
  if(hw.buttonState(PAGE) && hw.justPressed(DOWN)) combo=true,loadPreset(currentBank-1,currentPreset);//,showForWhile("pr  "),hw.setDot(1,true),hw.displayChar(presetName[2],3),hw.displayChar(presetName[1],2),clearIndexes();
  if(hw.buttonState(PAGE) && hw.justPressed(REC)){
    combo=true;
    if(storePreset(currentBank,currentPreset)) showForWhile("save"); 
    else showForWhile("eror");
  }
  shift=hw.buttonState(FN);
  if(shift && !hw.buttonState(PAGE)){
    if(hw.justPressed(UP)) combo=true, copy(activeSound);//,copyPreset();
    if(hw.justPressed(DOWN)) combo=true, paste(activeSound);//,pastePreset();
    if(hw.justPressed(REC)){
      combo=true;
      if(storePreset(currentBank,currentPreset)) showForWhile("save"); 
      else showForWhile("eror");
    }
    for(int i=0;i<5;i++){
      if(hw.justPressed(bigButton[i])) {
        combo=true;
        setting=getVar(activeSound,SETTING);
        bitWrite(setting,i,!bitRead(setting,i));
        setVar(activeSound,SETTING,setting); 
        setSetting(activeSound);
      }
    }

    if(hw.justPressed(HOLD)) {
      instantLoop++;
      if(instantLoop>2) instantLoop=0;
      if(instantLoop==1){
        instantStart=wave.getCurPosition();
        if(sync) instantClockCounter=clockCounter;
      }
      if(instantLoop==2){ 
        if(sync) instantClockCounter=clockCounter-instantClockCounter, instantClockCounter=snapToUseful(instantClockCounter);
        else instantEnd=wave.getCurPosition();
        wave.pause();
        wave.seek(instantStart);
        wave.resume();
        lastPosition=instantStart;
      }
    }
    if(hw.justPressed(bigButton[5])) combo=true, demo();

  }


  if(combo){
    //turn off combo when all buttons are released 
    unsigned char _count=0; 
    for(int i=0;i<NUMBER_OF_BUTTONS;i++)  _count+=hw.buttonState(i);
    if(_count==0) combo=false;
  }  
}

int snapToUseful(int _number){
  unsigned char smallerThan;
  for(int i=0;i<16;i++){
    if(_number < pgm_read_word_near(usefulLengths+i)){
      smallerThan=i; 
      break;
    }
  }

  int difference=pgm_read_word_near(usefulLengths+smallerThan)-pgm_read_word_near(usefulLengths+smallerThan-1);
  unsigned char _clock;
  if(_number>= ((difference/2)+pgm_read_word_near(usefulLengths+smallerThan-1))) _clock=smallerThan;
  else _clock=smallerThan-1;
  _number=pgm_read_word_near(usefulLengths+_clock);
  return _number;
}
void renderSmallButtons(){

  if(!combo){

    if(hw.justPressed(UP)){
      stopSound();
      listNameUp();
      setVar(activeSound,SAMPLE_NAME_1,name[0]);
      setVar(activeSound,SAMPLE_NAME_2,name[1]);
      sound=activeSound;
      if(notesInBuffer>0) playSound(sound);
      else showSampleName(),whileShow=true,whileTime=millis();
      longTime=millis();
      longPress=false;
    } 

    if(hw.justPressed(DOWN)){
      stopSound();
      listNameDown();
      setVar(activeSound,SAMPLE_NAME_1,name[0]);
      setVar(activeSound,SAMPLE_NAME_2,name[1]);
      sound=activeSound;
      if(notesInBuffer>0) playSound(sound);
      else showSampleName(),whileShow=true,whileTime=millis();
      longTime=millis();
      longPress=false;
    } 

    if(hw.buttonState(DOWN) || hw.buttonState(UP)){
      if(!longPress){
        if((millis()-longTime) > LONG_PERIOD) longPress=true, longTime=millis();
      }
      else{
        if((millis()-longTime) > MOVE_PERIOD){ 
          longTime=millis();
          if(hw.buttonState(DOWN)){
            downWithFirstLetter();
            setVar(activeSound,SAMPLE_NAME_1,name[0]);
            hw.displayChar(name[0],2);
            whileShow=true,whileTime=millis();


          }
          else if(hw.buttonState(UP)){
            upWithFirstLetter();
            setVar(activeSound,SAMPLE_NAME_1,name[0]);
            hw.displayChar(name[0],2);
            whileShow=true,whileTime=millis();


          } 
        }
      }
    }
    if(hw.justReleased(UP) || hw.justReleased(DOWN)){
      if(longPress){  
        sound=activeSound;
        indexed(activeSound,false);
        if(notesInBuffer>0) playSound(sound);
        else showSampleName(),whileShow=true,whileTime=millis();
      }
    }
  }

}

unsigned char interfaceSound;
void renderBigButtons(){

  for(int i=0;i<NUMBER_OF_BIG_BUTTONS;i++) {
    if(hold){
      if(hw.justPressed(bigButton[i])) putNoteOut(sound), sound=i, activeSound=sound,putNoteIn(sound), interfaceSound=sound;//,playSound(sound);
    }
    else{
      if(hw.justPressed(bigButton[i])) sound=i,activeSound=sound,putNoteIn(sound),interfaceSound=sound;//,playSound(sound);
      if(hw.justReleased(bigButton[i])) sound=putNoteOut(i);// stopEnvelope();
    }     

    if(activeSound==i && notesInBuffer>0) hw.setLed(bigButton[i],true);
    else hw.setLed(bigButton[i],false);
  }
}

void renderRecordRoutine(){
  if(recSound==0){
    if(hw.justPressed(REC) || hw.justPressed(PAGE)) rec=false,restoreAnalogRead(),combo=true;
    if(hw.justPressed(HOLD)) thru=!thru,wave.setAudioThru(thru);
    hw.setLed(HOLD,thru);
    for(int i=0;i<NUMBER_OF_BIG_BUTTONS;i++) {
      if(hw.justPressed(bigButton[i])) recSound=i,trackRecord(recSound,currentPreset);
    }
    displayVolume();
  }
}

void displayVolume(){

  unsigned char _range= wave.adcGetRange();
  for(int i=0;i<4;i++){
    if(_range>=((i+1)*31)) hw.setDot(i,true);
    else hw.setDot(i,false);
  }
  wave.adcClearRange();
}
void renderHold(){
  if(!shift){
    if(hw.justPressed(HOLD)){
      if(hold){
        hold=false;
        unsigned char keepPlaying=255;
        for(int i=0;i<NUMBER_OF_BIG_BUTTONS;i++){
          if(hw.buttonState(bigButton[i])){
            if(i==midiBuffer[ZERO]) keepPlaying=i;
          }
        }
        if(keepPlaying!=255) sound=keepPlaying;//, clearBuffer(1);
        else clearBuffer(),sound=0, stopEnvelope(),instantLoop=0;
      }
      else hold=true;
    }
  }
  hw.setLed(HOLD,hold); 
}


void renderKnobs(){
  unsigned char _sound;
  //if(notesInBuffer>0) _sound=activeSound;
  //else 
  _sound=interfaceSound;

  for(int i=0;i<NUMBER_OF_KNOBS;i++){

    unsigned char _variable=i+VARIABLES_PER_PAGE*page;


    if(hw.knobFreezed(i)) {
      hw.setLed(knobLed[i],false);

      if(inBetween( scale(hw.knobValue(i),KNOB_BITS,variableDepth[_variable]), scale(hw.lastKnobValue(i),KNOB_BITS,variableDepth[_variable]),getVar(_sound,_variable) ) ) hw.unfreezeKnob(i);//,showForWhile(knobLabel(page,i)),lastMoved==i; //external unfreez

    }

    else{    
      hw.setLed(knobLed[i],true);   
      int _value=scale(hw.knobValue(i),KNOB_BITS,variableDepth[_variable]);
      int was=getVar(_sound,_variable);
      setVar(_sound,_variable,_value); 

      if(variableDepth[_variable]>7){
        if((was>>2)!=(_value>>2)) {
          lastMoved=i;
          whileShow=true;
          whileTime=millis();
          if(_variable==START) loadValuesFromMemmory(sound); //snap to start
        }

      }
      else if((was>>1)!=(_value>>1)) {
        lastMoved=i;
        whileShow=true;
        whileTime=millis();
      }

      /*
        if(hw.knobMoved(i) || was!=_value) {
       lastMoved=i;
       whileShow=true;
       whileTime=millis();
       }
       */
      //  if(was!=_value)

      boolean showSlash=false;
      if(lastMoved==i){
        if(_variable==RATE){
          if(tuned){
            _value=pitch-36;
            //  _value=myMap(_value,1024,43);
            // _value-=36;
          }
          else _value=pitch-360;//_value-=860;

        }
        else if(_variable==LOOP_LENGTH && sync){
          _value=pgm_read_word_near(usefulLengths+(_value>>3));
          if(_value<24 && _value>0) _value= 24/_value,showSlash=true;
          else _value/=24; 
        }
        else if(_variable==SHIFT_SPEED){
          _value-=128;
        }
        else if(_variable==END){
          if(sync){
            _value=pgm_read_word_near(usefulLengths+(_value>>6)+1);
            if(_value<24 && _value>0) _value= 24/_value,showSlash=true;
            else _value/=24; 
          }
          else _value=endIndex; 
        }
        hw.displayChar(pgm_read_word_near(labels + i + page*PAGE_KNOB_LABEL_OFFSET),0);
        showValue(_value);
        if(showSlash) hw.lightNumber(SLASH,1);

      }
    }

  }
  // }
  /*
  else{
   lastMoved=5;
   
   }
   */

  if(notesInBuffer==0) for(int i=0;i<NUMBER_OF_KNOBS;i++) hw.setLed(knobLed[i],false);  
}

void renderDisplay(){


  if(shift){
    //  hw.displayText("set "); 
    // hw.displayChar(activeSound+49,3);
    for(int i=0;i<5;i++)  hw.setLed(bigButton[i],bitRead(setting,i));
    /*
    hw.setLed(bigButton[0],repeat);
     hw.setLed(bigButton[1],tuned);
     hw.setLed(bigButton[2],shiftDir);
     hw.setLed(bigButton[3],bitRead(setting,SYNC_BIT));
     hw.setLed(bigButton[4],legato);
     */
    hw.setLed(bigButton[5],false);

  }

  if(instantLoop==2){
    hw.displayText("loop"); 
  }//else if(

  else if(whileShow){
    if(millis()-whileTime>WHILE_DURATION) whileShow=false,showSampleName(),noDots(),lastMoved=5;
  }
  else if(!wave.isPlaying() && !rec) {
    for(int i=0;i<4;i++) hw.lightNumber(LINES,i);
    noDots();
  } 
  else{
    showSampleName(); 
    noDots();
  }
  //showValue(bytesAvailable);
}



void showForWhile(char *show){
  whileShow=true;
  whileTime=millis();
  hw.displayText(show); 
}


void blinkLed(unsigned char _LED,int interval){
  if(++blinkCounter >= interval) blinkCounter=0, blinkState=!blinkState;
  hw.setLed(_LED,blinkState);
  //
}


void randomize(unsigned char _sound){
  for(int i=0;i<10;i++) setVar(_sound,i,rand(maxVal(i))); 
  setVar(_sound,CRUSH,rand(20));
}

unsigned char copyMemory[NUMBER_OF_BYTES];
void copy(unsigned char _sound){
  showForWhile("copy");
  for(int i=0;i<NUMBER_OF_BYTES;i++) copyMemory[i]=variable[_sound][i];
}

void paste(unsigned char _sound){
  showForWhile("pste");
  for(int i=0;i<NUMBER_OF_BYTES;i++) variable[_sound][i]=copyMemory[i];
  indexed(_sound,false);
  hw.freezeAllKnobs();
}

void noSound(){
  envelopePhase=3;
  stopSound();
}
void stopSound(){
  wave.stop(),file.close();
}

void loadName(unsigned char _sound){
  //for(int i=0;i<2;i++)  name[i]=getVar(_sound,SAMPLE_NAME_1+i);
  name[0]=getVar(_sound,SAMPLE_NAME_1);
  name[1]=getVar(_sound,SAMPLE_NAME_2);
}

void showSampleName(){
  hw.displayChar('S',0);
  hw.displayChar(' ',1);
  hw.displayChar(name[0],2);
  hw.displayChar(name[1],3);
}


void noDots(){
  for(int i=0;i<4;i++) hw.setDot(i,false);
}

/*
char* knobLabel(unsigned char _page,unsigned char _knob){
 
 char label[NUMBER_OF_DIGITS];
 for(int i=0;i<NUMBER_OF_DIGITS;i++) label[i]=pgm_read_word_near(labels + i + _knob + _page*PAGE_KNOB_LABEL_OFFSET);
 return label;
 
 }
 */

void demo(){

  currentBank=9;
  currentPreset=5;
  loadPreset(currentBank,currentPreset);
  for(int i=0;i<30;i++){
    unsigned char _note=rand(6);
    putNoteIn(_note);
    while(wave.isPlaying()){
      hw.displayText("demo");
      hw.update();
      updateSound();
      if(hw.buttonState(bigButton[5])) break;  
    }
    putNoteOut(_note);
  } 
  clearBuffer();

}










