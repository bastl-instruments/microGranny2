char name[7]="30.WAV";
#define SLOW_INTERVAL 100
#define FAST_INTERVAL 50
//#include <wiring.c>
boolean rec, recording;
int blinkCounter;
boolean blinkState;
unsigned char recSound;
boolean thru;
unsigned char indexedHash=0;
boolean indexed(unsigned char _sound){
  return bitRead(indexedHash,_sound);
}
void indexed(unsigned char _sound,boolean _state){
  bitWrite(indexedHash,_sound,_state);
}
long index[NUMBER_OF_SOUNDS];
void clearIndexes(){
  indexedHash=0;
}
void initSdCardAndReport(){
  if (!card.init()) error("card");
  if (!vol.init(&card));// error("vol ");
  if (!root.openRoot(&vol));// error("root");
  /*
  uint8_t bpc = vol.blocksPerCluster();
   //  PgmPrint("BlocksPerCluster: ");
   
   //Serial.println(bpc, DEC);
   
   uint8_t align = vol.dataStartBlock() & 0X3F;
   // PgmPrint("Data alignment: ");
   //Serial.println(align, DEC);
   
   //  PgmPrint("sdCard size: ");
   //Serial.print(card.cardSize()/2000UL);
   
   
   if (align || bpc < 64) {
   //   PgmPrintln("\nFor best results use a 2 GB or larger card.");
   //// PgmPrintln("Format the card with 64 blocksPerCluster and alignment = 0.");
   // PgmPrintln("If possible use SDFormater from www.sdcard.org/consumers/formatter/");
   }
   if (!card.eraseSingleBlockEnable()) {
   // PgmPrintln("\nCard is not erase capable and can't be used for recording!");
   }
   */
}

uint8_t playBegin(char* name,unsigned char _sound) {

  if(indexed(_sound)){
    if (!file.open(&root, index[_sound], O_READ)) {
      return false;
    }
  }
  else{
    //name
    if (!file.open(&root,name, O_READ)) { 
      return false;
    }
    else{
      index[_sound]=root.curPosition()/32-1;
      indexed(_sound,true); 
    }



  }


  /*
  if (!file.open(&root, name, O_READ)) { 
   // index[_sound
   // if (!file.open(&root, index, O_READ))
   //  PgmPrint("Can't open: ");
   // Serial.println(name);
   return false;
   }
   */

  if (!wave.play(&file)) {
    //  PgmPrint("Can't play: ");
    // Serial.println(name);
    file.close();
    return false;
  }
  return true;
}




void error(char* str) {
  //  hw.initialize();
  hw.displayText(str);
  while(1){
    hw.updateDisplay(); 
  }
}



// record a track
unsigned char updt;

void trackRecord(unsigned char _sound,unsigned char _preset) {
  stopSound();
  //  Serial.flush();
  // Serial.end();

  //name="30.WAV";
  // noInterrupts();

  name[0]=currentBank + 48;
  _preset=currentPreset*6+_sound;
  if(_preset<10) name[1]=_preset+48;
  else name[1]=_preset+55;


  if (file.open(&root,name, O_READ)) {
    file.close();
    if (SdFile::remove(&root, name)) {
      //  hw.displayText("redy");
    } 
    else return;

    // return;

  }
  if (!file.createContiguous(&root, name, MAX_FILE_SIZE)) {
    //  hw.displayText("eror");
    return;
  }
  wave.adcInit(RECORD_RATE, MIC_ANALOG_PIN, ADC_REFERENCE);
  hw.displayText("redy");
  hw.setLed(bigButton[_sound],true);


  while(1){
    updt++;
    if(updt>3) updt=0, hw.updateDisplay(); 
    hw.updateButtons();
    hw.updateDisplay();

    // hw.updateMatrix(); //hack
    blinkLed(REC,FAST_INTERVAL);
    //   for(int i=0;i<NUMBER_OF_BIG_BUTTONS;i++) {
    //   if(hw.justPressed(bigButton[i])) _sound=i, hw.setLed(bigButton[i],true);
    // trackRecord(recSound-1,currentPreset)
    //}
    if(hw.justPressed(HOLD)) thru=!thru,wave.setAudioThru(thru);
    hw.setLed(HOLD,thru);
    displayVolume();
    if(hw.justPressed(REC)){
      break;
    }
    if(hw.justPressed(PAGE)){
      recording=false,recSound=0,file.remove(),file.close(),rec=false,restoreAnalogRead();
      break;
    }
  }
  if(!rec){
    chacha();
    return;
  }
  else recording=false,recSound=0,rec=false;

  if(!wave.record(&file, RECORD_RATE, MIC_ANALOG_PIN, ADC_REFERENCE)) {
    hw.displayText("eror");
    file.remove();
    return;
  } 
  hw.dimForRecord(bigButton[_sound]);

  pinMode(6,INPUT_PULLUP);

  while (wave.isRecording()) { //udělat něco jako delay
    if(!digitalRead(6)) wave.stop(); //hw.justPressed(REC)) 
  }
  wave.stop();


  wave.trim(&file);
  file.close();
  clearSound(_sound);
  setVar(_sound,SAMPLE_NAME_1,name[0]);
  setVar(_sound,SAMPLE_NAME_2,name[1]);
  //  recorded=true;
  // setVar(_sound,MODE,1);
  indexed(_sound,false);

  chacha();
  // restoreAnalogRead();
  //  noDots();
  //  Serial.begin(MIDI_BAUD);
  // Serial.flush();
  //recorded=255;
}



void chacha(){
  storePreset(currentBank,currentPreset);
  EEPROM.write(1001,currentBank);
  EEPROM.write(1002,currentPreset);
  EEPROM.write(1000,1),software_Reset(); 
}




