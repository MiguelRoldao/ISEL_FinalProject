#include <EEPROM.h>

//*** 0.1 - PIN INITIALIZATION ***//
// Initialize DigitalOuts
const byte chA_pin       =  0;
const byte chB_pin       =  1;
const byte bnk_pin       =  2;
const byte preShpr_pin   =  4;
const byte postShpr_pin  =  7;
const byte boost_pin     =  8;
const byte fxLoop_pin    = 12;
const byte mode_pin      = 13;

// Initialize AnalogOuts
const byte gain_o_pin      =  6;
const byte volume_o_pin    =  5;
const byte presence_o_pin  =  3;
const byte treble_o_pin    =  9;
const byte middle_o_pin    = 10;
const byte bass_o_pin      = 11;

// Initialize AnalogIns
const byte gain_i_pin      = A1;
const byte volume_i_pin    = A2;
const byte presence_i_pin  = A3;
const byte treble_i_pin    = A4;
const byte middle_i_pin    = A5;
const byte bass_i_pin      = A6;
const byte buttons_i_pin   = A7;


//*** 0.2 - VARIABLE INITIALIZATION ***//

struct chMemForm { //Estrutura para ler e escrever na EEPROM
  byte buttonData;
  byte gain_PWM;
  byte volume_PWM;
  byte presence_PWM;
  byte treble_PWM;
  byte middle_PWM;
  byte bass_PWM;
  byte unused;
};

struct buttonVarForm {
  byte channel_var;
  bool bnk_var;
  bool preShpr_var;
  bool postShpr_var;
  bool boost_var;
  bool fxLoop_var;
  bool mode_var; //Recall Preset PWM values
};

// Data aquisition variables
bool recentLoad = 0;
bool equalPotParam = 0;
byte oldReadPotParam[6];
byte newReadPotParam[6];
byte toWritePotParam[6];
byte loadedPotParam[6];
byte buttonFlag = 0; //1 save, 2 mode, 3 ch1, 4 ch2, 5 ch3, 6 bnk, 7 pre, 8 post, 9 boost, 10 fxloop
byte old_buttonFlag = 0;
int old_bttn_value; 
int new_bttn_value;
byte bttn_qntfctn_vals[11];
bool loadedButtonFlags[4];

int extRef;
int temp_extRef;

/*// Present time variables 
int channel_var = 1;
bool bnk_var = LOW;
bool preShpr_var = LOW;
bool postShpr_var = LOW;
bool boost_var = LOW;
bool fxLoop_var = LOW;
bool mode_var = LOW; //Panel or Preset */ //Optou-se pelo "buttonVarForm"

buttonVarForm readButtons; //Variavel guarda a última informação relevante aos botões.

//function headers
void readPots( byte arr[] );
int readButtonsFunc( int b );
void saveCurrentChannel();
void loadFromEEPROM();
int memAddr( int ch, int bnk);
byte buttonFlags2Byte ( buttonVarForm rB );
void buttonByte2Flags (byte var, bool *arr);

void setup() {
  //Changing timer frequency
  TCCR0B = TCCR0B & 0b11111000 | 0b1; //Set pin 5 and  6 PWM to 62.50 kHz
  TCCR1B = TCCR1B & 0b11111000 | 0b1; //Set pin 9 and 10 PWM to 31.25 kHz
  TCCR2B = TCCR2B & 0b11111000 | 0b1; //Set pin 3 and 11 PWM to 31.25 kHz
  
  //analogReference(EXTERNAL);
  
  // DigitalOuts
  pinMode( chA_pin,      OUTPUT );
  pinMode( chB_pin,      OUTPUT );
  pinMode( bnk_pin,      OUTPUT ); 
  pinMode( postShpr_pin, OUTPUT );
  pinMode( preShpr_pin,  OUTPUT );
  pinMode( boost_pin,    OUTPUT );
  pinMode( fxLoop_pin,   OUTPUT );
  pinMode( mode_pin,     OUTPUT );

  readPots(oldReadPotParam);
  readPots(newReadPotParam);

  // Present time variables
  readButtons.channel_var = 1;
  readButtons.bnk_var = LOW;
  readButtons.preShpr_var = LOW;
  readButtons.postShpr_var = LOW;
  readButtons.boost_var = LOW;
  readButtons.fxLoop_var = LOW;
  readButtons.mode_var = LOW; //Panel or Preset
  

  
  bttn_qntfctn_vals[0] = 0;
  bttn_qntfctn_vals[1] = 256/20;
  for(int i = 2; i < 11; ++i ){
    bttn_qntfctn_vals[i] = bttn_qntfctn_vals[1] + (i-1)*256/10;
  }
  
  //For debug ONLY---
  extRef = analogRead(buttons_i_pin);

  old_bttn_value = readButtonsFunc(buttons_i_pin);
  new_bttn_value = readButtonsFunc(buttons_i_pin);

}

void loop() {
  
  /******** Data Aquisition ********/

  do {
    buttonFlag = 0;
    readPots(newReadPotParam); //array with new potentiometer values

    temp_extRef = analogRead( buttons_i_pin );
    while( extRef < temp_extRef ){
      extRef = temp_extRef;
      temp_extRef = analogRead( buttons_i_pin );
    }
 
    //  read Buttons
    do {                                                  // Loop assures that the 
      old_bttn_value = new_bttn_value;                    // new_bttn_value is
      delay(640);                                         // accurate to prevent
      new_bttn_value = readButtonsFunc(buttons_i_pin);    // wrong measurementss (eg.
    } while( old_bttn_value != new_bttn_value );          // due to bad switches).

   
    for(int i = 0; i <= 10 && buttonFlag == 0; i++ ){ //quantificar o valor obtido para uma tabela de 10 valores                          
      if( new_bttn_value < bttn_qntfctn_vals[i]){
        buttonFlag = i;
      }
    }

    if (buttonFlag == 0){
      old_buttonFlag = 0;
    }

    equalPotParam = cmpPotParam( oldReadPotParam, newReadPotParam, recentLoad );
  } while( !(old_buttonFlag == 0) );
  /*********************************/
    
  /******** Data Processing ********/
  if( !equalPotParam ) 
    for(int i = 0; i <= 5; i++)
      toWritePotParam[i] = newReadPotParam[i];
  
  if( buttonFlag >= 1 && buttonFlag <= 10 ){
    switch ( buttonFlag ){
      case 1:   //save
        saveCurrentChannelEEPROM();    
        break;
      case 2:   //mode
        readButtons.mode_var = !readButtons.mode_var;
        break;
      case 3:   //ch1
        readButtons.channel_var = 1;
        loadFromEEPROM(); 
        break;
      case 4:   //ch2
        readButtons.channel_var = 2;
        loadFromEEPROM(); 
        break;
      case 5:   //ch3
        readButtons.channel_var = 3;
        loadFromEEPROM(); 
        break;
      case 6:   //bnk
        readButtons.bnk_var = !readButtons.bnk_var;
        loadFromEEPROM(); 
        break;
      case 7:   //pre
        readButtons.preShpr_var = !readButtons.preShpr_var;
        break;
      case 8:   //post
        readButtons.postShpr_var = !readButtons.postShpr_var;
        break;
      case 9:   //boost
        readButtons.boost_var = !readButtons.boost_var;
        break;
      case 10:  //fxloop
        readButtons.fxLoop_var = !readButtons.fxLoop_var;
        break;
    }
  }
  
  /*********************************/
  
  /********** Data Output **********/
  if( buttonFlag >= 1 && buttonFlag <= 10 && buttonFlag != old_buttonFlag || oldReadPotParam != newReadPotParam){
    
    digitalWrite(chA_pin,       readButtons.channel_var == 1 );
    digitalWrite(chB_pin,       readButtons.channel_var == 2 );
    digitalWrite(bnk_pin,       readButtons.bnk_var          );
    digitalWrite(mode_pin,      readButtons.mode_var         );

    digitalWrite(preShpr_pin,   readButtons.preShpr_var      );
    digitalWrite(postShpr_pin,  readButtons.postShpr_var     );
    digitalWrite(boost_pin,     readButtons.boost_var        );
    digitalWrite(fxLoop_pin,    readButtons.fxLoop_var       );

    if( readButtons.mode_var ){

      analogWrite(gain_o_pin,     loadedPotParam[0] );
      analogWrite(volume_o_pin,   loadedPotParam[1] );
      analogWrite(presence_o_pin, loadedPotParam[2] );
      analogWrite(treble_o_pin,   loadedPotParam[3] );
      analogWrite(middle_o_pin,   loadedPotParam[4] );
      analogWrite(bass_o_pin,     loadedPotParam[5] );
      
    }
    else{
      analogWrite(gain_o_pin,     toWritePotParam[0] );
      analogWrite(volume_o_pin,   toWritePotParam[1] );
      analogWrite(presence_o_pin, toWritePotParam[2] );
      analogWrite(treble_o_pin,   toWritePotParam[3] );
      analogWrite(middle_o_pin,   toWritePotParam[4] );
      analogWrite(bass_o_pin,     toWritePotParam[5] );
    }
    
  }
  /*********************************/
  old_buttonFlag = buttonFlag;
  //buttonFlag = 0;
  for( int i = 0; i < 6; i++ ){
    oldReadPotParam[i] = newReadPotParam[i];
  }
}

//////////// EXTRA Functions //////////////
void readPots(byte arr[] ){
  arr[0] = analogRead(gain_i_pin)/4;
  arr[1] = analogRead(volume_i_pin)/4;
  arr[2] = analogRead(presence_i_pin)/4;
  arr[3] = analogRead(treble_i_pin)/4;
  arr[4] = analogRead(middle_i_pin)/4;
  arr[5] = analogRead(bass_i_pin)/4;
}

int readButtonsFunc(int b){
  return ((analogRead(b)/4) * 256)/(extRef/4);
}


void saveCurrentChannelEEPROM(){
  int addr = memAddr( readButtons.channel_var, readButtons.bnk_var );
  
  byte buttonByte = buttonFlags2Byte ( readButtons );
  chMemForm toSave = {
    buttonByte,
    newReadPotParam[0],
    newReadPotParam[1],
    newReadPotParam[2],
    newReadPotParam[3],
    newReadPotParam[4],
    newReadPotParam[5],
    0,
  };
  EEPROM.put( addr, toSave);

  for(int i = 1; i <= 5; i++){
    digitalWrite( mode_pin, !readButtons.mode_var );
    delay( 3200 );
    digitalWrite( mode_pin, readButtons.mode_var  );
    delay( 3200 );
  }
}

void loadFromEEPROM(){
  chMemForm lD;
  EEPROM.get( memAddr(readButtons.channel_var,readButtons.bnk_var) , lD);
  
  buttonByte2Flags ( lD.buttonData, loadedButtonFlags );
  
  readButtons.preShpr_var =   loadedButtonFlags[0] ;
  readButtons.postShpr_var =  loadedButtonFlags[1] ;
  readButtons.boost_var =     loadedButtonFlags[2] ;
  readButtons.fxLoop_var =    loadedButtonFlags[3] ;
  
  loadedPotParam[0] = lD.gain_PWM     ;
  loadedPotParam[1] = lD.volume_PWM   ;
  loadedPotParam[2] = lD.presence_PWM ;
  loadedPotParam[3] = lD.treble_PWM   ;
  loadedPotParam[4] = lD.middle_PWM   ;
  loadedPotParam[5] = lD.bass_PWM     ;

  for( int i = 0; i <= 5; i++ ){
    toWritePotParam[i] = loadedPotParam[i];
  }

  recentLoad = HIGH;
}


int memAddr( int ch, int bnk){
  unsigned addr = (ch-1) * 0x10 ;
  if( bnk )
    addr += 0x8;
  return addr;
}


byte buttonFlags2Byte ( buttonVarForm rB ){   // byte is {0 0 0 0 pre post boost fx}
  byte b = 0;
  if( rB.fxLoop_var )
    b += B0001;
  if( rB.boost_var )
    b += B0010;
  if( rB.postShpr_var )
    b += B0100;
  if( rB.preShpr_var )
    b += B1000;
  return b;
}


void buttonByte2Flags (byte var, bool arr[4]){  //flags array is {pre, post, boost, fx}
  arr[3] = var%2;       
  arr[2] = (var/2)%2;
  arr[1] = (var/4)%2;
  arr[0] = (var/8)%2;

}

bool cmpPotParam( byte *a, byte *b, bool flag){
  for( int i = 0; i <= 5 ; i++ ) {
    if( a[i] != b[i] && (!flag || a[i]-b[i] >= 4 || a[i]-b[i] <= -4 ) ){
      flag = LOW;
      return false;
    }
  }
  return true;
}
