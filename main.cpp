/*
Last Updated: 30 Mar. 2018
By Anton Grimpelhuber (anton.grimpelhuber@gmail.com)
                                                                                        
-----------------------------------------------------------  
Semver (http://semver.org/) VERSION HISTORY (newest on top):  
(date format: yyyymmdd; ex: 20161022 is 22 Oct. 2016)  
------------------------------------------------------------  
 - 20180330 - v1.4 - First port to ESP8266 (tested: wemos D1 mini) by Anton Grimpelhuber  
 - 20161022 - v1.3 - Semver versioning implemented; various code updates, clarifications, & comment additions, and changes to fix incompatibilities so it will now compile with latest versions of gcc compiler; also improved blink indicator routines & added the ability to stop the code-sending sequence once it has begun; by Gabriel Staples (http://www.ElectricRCAircraftGuy.com)  
 - 20101023 - v1.2 - Latest version posted by Ken Shirriff on his website here (http://www.righto.com/2010/11/improved-arduino-tv-b-gone.html) (direct download link here: http://arcfn.com/files/arduino-tv-b-gone-1.2.zip)  
 - 20101018 - v1.2 - Universality for EU (European Union) & NA (North America) added by Mitch Altman; sleep mode added by ka1kjz  
 - 2010____ - v1.2 - code ported to Arduino; by Ken Shirriff  
 - 20090816 - v1.2 - for ATtiny85v, by Mitch Altman & Limor Fried (https://www.adafruit.com/), w/some code by Kevin Timmerman & Damien Good  

TV-B-Gone for Arduino version 1.2, Oct 23 2010
Ported to Arduino by Ken Shirriff
See here: http://www.arcfn.com/2009/12/tv-b-gone-for-arduino.html and here: http://www.righto.com/2010/11/improved-arduino-tv-b-gone.html (newer)

I added universality for EU (European Union) or NA (North America),
and Sleep mode to Ken's Arduino port
 -- Mitch Altman  18-Oct-2010
Thanks to ka1kjz for the code for adding Sleep
 <http://www.ka1kjz.com/561/adding-sleep-to-tv-b-gone-code/>
 
The original code is:
TV-B-Gone Firmware version 1.2
 for use with ATtiny85v and v1.2 hardware
 (c) Mitch Altman + Limor Fried 2009
 Last edits, August 16 2009

With some code from:
Kevin Timmerman & Damien Good 7-Dec-07

------------------------------------------------------------
CIRCUIT:
------------------------------------------------------------
-NB: SEE "main.h" TO VERIFY DEFINED PINS TO USE
The hardware for this project uses a wemos D1 mini ESP8266-based board:
 (internal) IR LED at pin 19
 Connect a push-button = TRIGGERpin for start sending codes
 Connect a push-button = REGIONSWITCH-pin 

More about the wiring is written in the readme.
------------------------------------------------------------
LICENSE:
------------------------------------------------------------
Distributed under Creative Commons 2.5 -- Attribution & Share Alike

*/
#include <Arduino.h>
#include "WORLD_IR_CODES.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "M5StickCPlus2.h"
#include "tvbgone.h"

void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code );
void delay_ten_us(uint16_t us);
uint8_t read_bits(uint8_t count);
uint16_t rawData[300];
uint16_t previousMillis = 0;



#define putstring_nl(s) Serial.println(s)
#define putstring(s) Serial.print(s)
#define putnum_ud(n) Serial.print(n, DEC)
#define putnum_uh(n) Serial.print(n, HEX)

#define MAX_WAIT_TIME 65535 //tens of us (ie: 655.350ms)

IRsend irsend(IRLED);  // Set the GPIO to be used to sending the message.

/*
This project transmits a bunch of TV POWER codes, one right after the other,
 with a pause in between each.  (To have a visible indication that it is
 transmitting, it also pulses a visible LED once each time a POWER code is
 transmitted.)  That is all TV-B-Gone does.  The tricky part of TV-B-Gone
 was collecting all of the POWER codes, and getting rid of the duplicates and
 near-duplicates (because if there is a duplicate, then one POWER code will
 turn a TV off, and the duplicate will turn it on again (which we certainly
 do not want).  I have compiled the most popular codes with the
 duplicates eliminated, both for North America (which is the same as Asia, as
 far as POWER codes are concerned -- even though much of Asia USES PAL video)
 and for Europe (which works for Australia, New Zealand, the Middle East, and
 other parts of the world that use PAL video).

 Before creating a TV-B-Gone Kit, I originally started this project by hacking
 the MiniPOV kit.  This presents a limitation, based on the size of
 the Atmel ATtiny2313 internal flash memory, which is 2KB.  With 2KB we can only
 fit about 7 POWER codes into the firmware's database of POWER codes.  However,
 the more codes the better! Which is why we chose the ATtiny85 for the
 TV-B-Gone Kit.

 This version of the firmware has the most popular 100+ POWER codes for
 North America and 100+ POWER codes for Europe. You can select which region
 to use by soldering a 10K pulldown resistor.
 */


/*
This project is a good example of how to use the AVR chip timers.
 */

extern const IrCode* const NApowerCodes[];
extern const IrCode* const EUpowerCodes[];
extern uint8_t num_NAcodes, num_EUcodes;

/* This is kind of a strange but very useful helper function
 Because we are using compression, we index to the timer table
 not with a full 8-bit byte (which is wasteful) but 2 or 3 bits.
 Once code_ptr is set up to point to the right part of memory,
 this function will let us read 'count' bits at a time which
 it does by reading a byte into 'bits_r' and then buffering it. */

uint8_t bitsleft_r = 0;
uint8_t bits_r=0;
uint8_t code_ptr;
volatile const IrCode * powerCode;

// we cant read more than 8 bits at a time so dont try!
uint8_t read_bits(uint8_t count)
{
  uint8_t i;
  uint8_t tmp=0;

  // we need to read back count bytes
  for (i=0; i<count; i++) {
    // check if the 8-bit buffer we have has run out
    if (bitsleft_r == 0) {
      // in which case we read a new byte in
      bits_r = powerCode->codes[code_ptr++];
      DEBUGP(putstring("\n\rGet byte: ");
      putnum_uh(bits_r);
      );
      // and reset the buffer size (8 bites in a byte)
      bitsleft_r = 8;
    }
    // remove one bit
    bitsleft_r--;
    // and shift it off of the end of 'bits_r'
    tmp |= (((bits_r >> (bitsleft_r)) & 1) << (count-1-i));
  }
  // return the selected bits in the LSB part of tmp
  return tmp;
}


/* Legacy explanation from old Arduino Code for reference
The C compiler creates code that will transfer all constants into RAM when
 the microcontroller resets.  Since this firmware has a table (powerCodes)
 that is too large to transfer into RAM, the C compiler needs to be told to
 keep it in program memory space.  This is accomplished by the macro
 (this is used in the definition for powerCodes).  Since the C compiler assumes
 that constants are in RAM, rather than in program memory, when accessing
 powerCodes, we need to use the pgm_read_word() and pgm_read_byte macros, and
 we need to use powerCodes as an address.  This is done with PGM_P, defined
 below.
 For example, when we start a new powerCode, we first point to it with the
 following statement:
 PGM_P thecode_p = pgm_read_word(powerCodes+i);
 The next read from the powerCode is a byte that indicates the carrier
 frequency, read as follows:
 const uint8_t freq = pgm_read_byte(code_ptr++);
 After that is a byte that tells us how many 'onTime/offTime' pairs we have:
 const uint8_t numpairs = pgm_read_byte(code_ptr++);
 The next byte tells us the compression method. Since we are going to use a
 timing table to keep track of how to pulse the LED, and the tables are
 pretty short (usually only 4-8 entries), we can index into the table with only
 2 to 4 bits. Once we know the bit-packing-size we can decode the pairs
 const uint8_t bitcompression = pgm_read_byte(code_ptr++);
 Subsequent reads from the powerCode are n bits (same as the packing size)
 that index into another table in ROM that actually stores the on/off times
 const PGM_P time_ptr = (PGM_P)pgm_read_word(code_ptr);
 */

#define BUTTON_PRESSED LOW 
#define BUTTON_RELEASED HIGH

uint16_t ontime, offtime;
uint8_t i,num_codes;
uint8_t region = NA;

void setup()   
{
  //damit der Stick auf Batteriebetrieb an bleibt
  pinMode(4, OUTPUT); 
  digitalWrite(4, HIGH);
  //Display des Stick CPlus2 initialisieren
  auto cfg = M5.config();
  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(1);
  StickCP2.Display.setSwapBytes(true); //ohne werden die jpg immer inverted angezeigt
  StickCP2.Display.setTextColor(GREEN);
  StickCP2.Display.setTextDatum(middle_center);
  StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
  StickCP2.Display.setTextSize(2);
  analogReadResolution(12);
  StickCP2.Display.clear();
  StickCP2.Display.pushImage(0,0,145,135,tvbgone);
   StickCP2.Display.setTextSize(2);
  StickCP2.Display.setCursor(150, 35);
  if (region ==0) 
    StickCP2.Display.printf("EU");
  else
    StickCP2.Display.printf("NA");
   StickCP2.Display.setTextSize(1);
  StickCP2.Display.setCursor(155, 115); StickCP2.Display.printf("%d %%", StickCP2.Power.getBatteryLevel());

  Serial.begin(115200);

  irsend.begin();

  pinMode(REGIONSWITCH, INPUT_PULLUP);
  pinMode(TRIGGER, INPUT_PULLUP);

  delay_ten_us(5000); //50ms (5000x10 us) delay: let everything settle for a bit


}

void sendAllCodes() 
{
  bool endingEarly = false; //will be set to true if the user presses the button during code-sending 
      
  // determine region from REGIONSWITCH: 1 = NA, 0 = EU (defined in main.h)
  if (region == NA) {
    num_codes = num_NAcodes;
  }
  else {
    num_codes = num_EUcodes;
  }

  // for every POWER code in our collection
  for (i=0 ; i<num_codes; i++) 
  {

    // print out the code # we are about to transmit
    DEBUGP(putstring("\n\r\n\rCode #: ");
    putnum_ud(i));

    // point to next POWER code, from the right database
    if (region == NA) {
      powerCode = NApowerCodes[i];
    }
    else {
      powerCode = EUpowerCodes[i];
    }
    
    // Read the carrier frequency from the first byte of code structure
    const uint8_t freq = powerCode->timer_val;
    // set OCR for Timer1 to output this POWER code's carrier frequency

    // Print out the frequency of the carrier and the PWM settings
    DEBUGP(putstring("\n\rFrequency: ");
    putnum_ud(freq);
    );
    
    DEBUGP(uint16_t x = (freq+1) * 2;
    putstring("\n\rFreq: ");
    putnum_ud(F_CPU/x);
    );

    // Get the number of pairs, the second byte from the code struct
    const uint8_t numpairs = powerCode->numpairs;
    DEBUGP(putstring("\n\rOn/off pairs: ");
    putnum_ud(numpairs));

    // Get the number of bits we use to index into the timer table
    // This is the third byte of the structure
    const uint8_t bitcompression = powerCode->bitcompression;
    DEBUGP(putstring("\n\rCompression: ");
    putnum_ud(bitcompression);
    putstring("\n\r"));

    // For EACH pair in this code....
    code_ptr = 0;
    for (uint8_t k=0; k<numpairs; k++) {
      uint16_t ti;

      // Read the next 'n' bits as indicated by the compression variable
      // The multiply by 4 because there are 2 timing numbers per pair
      // and each timing number is one word long, so 4 bytes total!
      ti = (read_bits(bitcompression)) * 2;

      // read the onTime and offTime from the program memory
      ontime = powerCode->times[ti];  // read word 1 - ontime
      offtime = powerCode->times[ti+1];  // read word 2 - offtime

      DEBUGP(putstring("\n\rti = ");
      putnum_ud(ti>>1);
      putstring("\tPair = ");
      putnum_ud(ontime));
      DEBUGP(putstring("\t");
      putnum_ud(offtime));      

      rawData[k*2] = ontime * 10;
      rawData[(k*2)+1] = offtime * 10;
      yield();
    }

    // Send Code with library
    irsend.sendRaw(rawData, (numpairs*2) , freq);
    Serial.print("\n");
    yield();
    //Flush remaining bits, so that next code starts
    //with a fresh set of 8 bits.
    bitsleft_r=0;
    
    // delay 235 milliseconds before transmitting next POWER code
    delay_ten_us(23500);

    // if user is pushing (holding down) TRIGGER button, stop transmission early 
    if (digitalRead(TRIGGER) == BUTTON_PRESSED) 
    {
      while (digitalRead(TRIGGER) == BUTTON_PRESSED){
        yield();
      }
      endingEarly = true;
      delay_ten_us(50000); //pause for 0,5 sec to give the user time to release the button so that the code sequence won't immediately start again.
      break; //exit the POWER code "for" loop
    }
    
  } //end of POWER code for loop


} //end of sendAllCodes

void loop() 
{
  //Super "ghetto" (but decent enough for this application) button debouncing:
  //-if the user pushes the Trigger button, then wait a while to let the button stop bouncing, then start transmission of all POWER codes
  if (digitalRead(TRIGGER) == BUTTON_PRESSED) 
  {
    delay_ten_us(40000);
    while (digitalRead(TRIGGER) == BUTTON_PRESSED){
      delay_ten_us(500);
      yield();
    }
    sendAllCodes();
  }
  yield();

  if (digitalRead(REGIONSWITCH) == BUTTON_PRESSED) 
  {
    delay_ten_us(40000);
    while (digitalRead(REGIONSWITCH) == BUTTON_PRESSED){
      delay_ten_us(500);
      yield();
    }
    // change region
    if (region == EU) {
      region = NA;
      StickCP2.Display.fillRect(145,0,95,135,BLACK);
      StickCP2.Display.setTextSize(2);
      StickCP2.Display.setCursor(150, 35);StickCP2.Display.printf("NA");
      StickCP2.Display.setTextSize(1);
      StickCP2.Display.setCursor(155, 115); StickCP2.Display.printf("%d %%", StickCP2.Power.getBatteryLevel());

  }
    else {
      region = EU;
      StickCP2.Display.fillRect(145,0,95,135,BLACK);
      StickCP2.Display.setTextSize(2);
      StickCP2.Display.setCursor(150, 35);StickCP2.Display.printf("EU");
      StickCP2.Display.setTextSize(1);
      StickCP2.Display.setCursor(155, 115); StickCP2.Display.printf("%d %%", StickCP2.Power.getBatteryLevel());
    }
    
  }
  yield();





  uint16_t currentMillis = millis();
  if (currentMillis - previousMillis >= 60000) { //batt percentage update only once a minute = 60.000 milisec
    
    // save the last time batt-percentage was updated
    previousMillis = currentMillis;
    
    StickCP2.Display.fillRect(145,90,95,45,BLACK);
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.setCursor(155, 115); StickCP2.Display.printf("%d %%", StickCP2.Power.getBatteryLevel());
  }







}//loop


/****************************** LED AND DELAY FUNCTIONS ********/


// This function delays the specified number of 10 microseconds
// it is 'hardcoded' and is calibrated by adjusting DELAY_CNT
// in main.h Unless you are changing the crystal from 8MHz, dont
// mess with this.
//-due to uint16_t datatype, max delay is 65535 tens of microseconds, or 655350 us, or 655.350 ms. 
//-NB: DELAY_CNT has been increased in main.h from 11 to 25 (last I checked) in order to allow this function
// to work properly with 16MHz Arduinos now (instead of 8MHz).
void delay_ten_us(uint16_t us) {
  uint8_t timer;
  while (us != 0) {
    // for 8MHz we want to delay 80 cycles per 10 microseconds
    // this code is tweaked to give about that amount.
    for (timer=0; timer <= DELAY_CNT; timer++) {
      NOP;
      NOP;
    }
    NOP;
    us--;
  }
}