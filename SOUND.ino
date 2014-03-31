long loopLength;
long  shiftSpeed;
long startPosition;
long endPosition;
int startIndex,endIndex;
boolean repeat;
boolean shiftDir,sync;
boolean ending=true;
//unsigned char row;
boolean hold;
unsigned char page;

long instantStart,instantEnd;

unsigned char envelopePhase;
long envelopeTime;
int envelopeNow;


long startTime;
unsigned char lfoAmt;

PROGMEM prog_uint16_t usefulLengths[17]={
  0,1,2,3,6,12,24/*doba*/,48/*2d*/,72/*3d*/,96/*4d*/,144/*6d*/,192/*8d*/,288/*12d*/,384/*16d*/,576/*24d*/,768/*32d*/,24000};


void updateSound(){
  // wave.getData();
  if(shiftDir && rand(2)==0) shiftSpeed=-shiftSpeed;
  if(notesInBuffer>0 || envelopePhase==2 || sustain){

    if(!wave.isPaused()){ //isPlaying
      renderEnvelope();
      renderLooping();
      renderGranular();

    }
    else {
      // setSetting(midiBuffer[ZERO]);
      setSetting(activeSound);
      if(repeat){
        loadValuesFromMemmory(sound);
      } 
      else stopSound();
    }
  }

  if(notesInBuffer>0){
    setSetting(activeSound);
    if(!wave.isPlaying() || wave.isPaused()){
      if(repeat) playSound(midiBuffer[ZERO]); 
    }
  }

}

int instantClockCounter;
void renderLooping(){
long _pos=wave.getCurPosition();
  if(instantLoop==2){
    if(sync){
      if((clockCounter % instantClockCounter)==0){
        wave.pause();
        wave.seek(instantStart);
        wave.resume();
        lastPosition=instantStart;
      }
    }
    else{
      if(shiftSpeed<0 && loopLength!=0){ 
        if(_pos<=instantEnd){
          wave.pause();
          wave.seek(instantStart);
          wave.resume();
          lastPosition=instantStart;
        }
      }
      else{
        if(_pos>=instantEnd){
          wave.pause();
          wave.seek(instantStart);
          wave.resume();
          lastPosition=instantStart;
        }
      }
    }
  }
  else{


    if(shiftSpeed<0 && loopLength!=0){ 
      if(sync){
        if(endIndex!=1000){
          if((clockCounter % endIndex)==0){
            if(repeat) wave.pause(),loadValuesFromMemmory(sound);
            else stopSound();
          }
        }
      }
      else{
        if(_pos<= startPosition ){
          if(repeat) wave.pause(),loadValuesFromMemmory(sound), wave.pause(), lastPosition=endPosition,wave.seek(lastPosition),wave.resume();
          else stopSound();
        }
      }
/*
      if(_pos>= lastPosition ){
        lastPosition+=shiftSpeed;

        wave.pause();
        wave.seek(lastPosition);
        wave.resume();
      }
*/

    }
    else{
      if(_pos<=startPosition) loadValuesFromMemmory(sound);
      if(sync){
        if(endIndex!=1000){
          if((clockCounter % endIndex)==0){
            if(repeat) wave.pause(),loadValuesFromMemmory(sound);
            else stopSound();
          }
        }
      }
      else{
        if(_pos>=endPosition){
          if(repeat) wave.pause(),loadValuesFromMemmory(sound);
          else stopSound();
        }
      }
    }
  }
  //
  /*
    else{
   
   if(wave.getCurPosition()>=endPosition){
   if(repeat) wave.pause(),loadValuesFromMemmory(sound);
   else stopSound();
   }
   
   if(wave.isPaused()){
   if(repeat) loadValuesFromMemmory(sound);
   else stopSound();
   }
   }
   */
  //  }
}


unsigned char velocity;
int attackInt;
void startEnvelope(unsigned char _velocity,int _attack){
  envelopeTime=millis();
  envelopeNow=37;
  envelopePhase=0;
  wave.setVolume(envelopeNow);
  velocity=31-(_velocity>>2);
  attackInt=_attack;
}

void stopEnvelope(){
  envelopeTime=millis();
  //  envelopeNow=0;

  if(envelopePhase!=2) wave.setVolume(envelopeNow);
  envelopePhase=2;
}

void renderEnvelope(){
  switch(envelopePhase){
  case 0:
    if(attackInt==0) envelopePhase=1, envelopeNow=0,wave.setVolume(envelopeNow);
    else if(millis()-envelopeTime>=attackInt){
      envelopeTime=millis();
      if(attackInt<30) envelopeNow-=3;
      else envelopeNow--;
      if(envelopeNow<=velocity) envelopeNow=0, envelopePhase=1;
      wave.setVolume(envelopeNow);
    }

    break;
  case 1:
    envelopeNow=velocity;
    wave.setVolume(velocity);
    break;
  case 2:
    if(wave.isPlaying()){
      if(releaseInterval==0) envelopePhase=3, envelopeNow=36,wave.setVolume(envelopeNow),stopSound();
      if(millis()-envelopeTime>=releaseInterval){
        envelopeTime=millis();
        if(releaseInterval<30) envelopeNow+=3;
        else envelopeNow++;

        if(envelopeNow>=36) envelopePhase=3, stopSound();
        wave.setVolume(envelopeNow);
      }
    }
    else{
      envelopeNow=36;
      envelopePhase=3, stopSound();
      wave.setVolume(envelopeNow);
    }

    break;
  case 3:
    stopSound();
    break;
  }
}
long lastLL;

static unsigned long x=132456789, y=362436069, z=521288629;

unsigned long xorshift96()
{ //period 2^96-1
  // static unsigned long x=123456789, y=362436069, z=521288629;
  unsigned long t;

  x ^= x << 16;
  x ^= x >> 5;
  x ^= x << 1;

  t = x;
  x = y;
  y = z;
  z = t ^ x ^ y;

  return z;
}

int rand(int maxval)
{
  return (int) (((xorshift96() & 0xFFFF) * maxval)>>16);
}
/*
int rand( int minval,  int maxval)
 {
 return (int) ((((xorshift96() & 0xFFFF) * (maxval-minval))>>16) + minval);
 }
 */
long granularTime;
void renderGranular(){



  if(loopLength!=0){
    if(lastLL==0) lastPosition=wave.getCurPosition();
    long pos=wave.getCurPosition();

    if(sync){

      if((clockCounter%loopLength)==0){
        // sync shiftSpeed - unnecessary 
        int _shiftSpeed=shiftSpeed;
        /*
        int _shiftSpeed=(int)getVar(sound,SHIFT_SPEED)-63; 
         if(_shiftSpeed > 0) _shiftSpeed=pgm_read_word_near(usefulLengths+(abs(_shiftSpeed)>>3)) * clockLength;
         else _shiftSpeed=(int)-pgm_read_word_near(usefulLengths+(abs(_shiftSpeed)>>3)) * clockLength;
         // sync shiftSpeed - unnecessary 
         */
        /*
        if(shiftDir){
         if(rand(2)==0) lastPosition+=_shiftSpeed;
         else lastPosition-=_shiftSpeed;
         }
         else 
         */
        lastPosition+=_shiftSpeed; //shift speed clock ???

        wave.pause();
        wave.seek(lastPosition);
        wave.resume();
      }

    }

    else{
      if(millis()-granularTime>=loopLength){
        granularTime=millis(); 

        //if(pos-lastPosition>=loopLength){
        lastPosition+=shiftSpeed;
        if(lastPosition<0){
          if(shiftSpeed<0) lastPosition=endPosition;
          else lastPosition=0;
        }
        wave.pause();
        wave.seek(lastPosition);
        wave.resume();
      }
    } 


  }
  lastLL=loopLength;

}


