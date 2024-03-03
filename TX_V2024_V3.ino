/*  settings_battery - ukazat cas hh:mm:ss od zapnutia - DONE
    settings_io - pridat ukladanie a zmenu modu.
                - zobrazenie modu - DONE
    settings_analogs - vytvorit mode 0 urceny iba pre zobrazenie v nastaveni - DONE
*/ 

// TX1024 - ARDUINO MEGA or MEGA 2560

/* NRF24 LIBS */
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <U8g2lib.h>
#include <EEPROM.h> // Arduino Mega: 4 kB EEPROM storage.

/* DEFINE PINS */
#define in_aux1      22 //UP RIGHT
#define in_aux2      23 
#define in_aux3      24 //UP LEFT
#define in_aux4      25
#define in_aux5      26 //DOWN LEFT
#define in_aux6      27
#define in_battery A10
#define in_a0 A0
#define in_a1 A1
#define in_a2 A2
#define in_a3 A3

#define enc_press 28
#define enc_cw 30
#define enc_ccw 31

// https://github.com/olikraus/u8g2/wiki/u8g2reference
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R1, SCL, SDA, U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R1, U8X8_PIN_NONE);
#define image_width  10
#define image_height 10

//XBMP images
//https://xbm.jazzychad.net/
static unsigned char main_bits[] = {
  0x03,0xff,0x03,0xff,0xb4,0xfc,0x78,0xfc,0x30,0xfc,0x30,0xfc,0x78,0xfc,0xb4,0xfc,0x03,0xff,0x03,0xff};

static unsigned char batt_bits[] = {
  0x78,0xfc,0xce,0xfd,0x02,0xfd,0x02,0xfd,0x02,0xfd,0x02,0xfd,0x02,0xfd,0x02,0xfd,0x02,0xfd,0xfe,0xfd};

static unsigned char chip_bits[] = {
  0xa8,0xfc,0xa8,0xfc,0x03,0xfc,0x78,0xff,0x7b,0xfc,0x78,0xff,0x7b,0xfc,0x00,0xff,0x54,0xfc,0x54,0xfc};

static unsigned char calib_bits[] = {
  0x80,0xfc,0xff,0xff,0xff,0xff,0x80,0xfc,0x84,0xfc,0x04,0xfc,0xff,0xff,0xff,0xff,0x04,0xfc,0x04,0xfc};

static unsigned char signal_bits[] = {
  0xfe,0xfd,0x01,0xfe,0xfc,0xfc,0x02,0xfd,0x78,0xfc,0x84,0xfc,0x30,0xfc,0x78,0xfc,0x78,0xfc,0x30,0xfc};

#define j_width  2
#define j_height 3
static unsigned char j_bits[] = {
  0xff,0xfe,0xff};

static unsigned char j2_bits[] = {
  0xff,0xfd,0xff};

#define direct_width  4
#define direct_height 5
static unsigned char left_bits[] = {
  0xfc,0xf6,0xf3,0xf6,0xfc};

static unsigned char right_bits[] = {
  0xf3,0xf6,0xfc,0xf6,0xf3};

#define up_down_width  5
#define up_down_height 4
static unsigned char up_bits[] = {
  0xe4,0xee,0xfb,0xf1};

static unsigned char down_bits[] = {
  0xf1,0xfb,0xee,0xe4};

#define dr1_width 64
#define dr1_height 79
// LOGO BITS in funtion logo()

#define joy_width  29
#define joy_height 27

//const uint64_t my_radio_pipe = 0xE8E8F0F0E1LL;
const uint64_t my_radio_pipe = 0xA8A8F0F0E1LL;                          //NFR
//const uint64_t my_radio_pipe_in = 0xB8A8F0F0E1LL;

/* NRF24 DEFINE PINS */
RF24 radio(49, 48); //CE, CSN

/* GLOBAL VALUES */
float voltage = 0.0;
int in_menu = 0;
byte radio_MODE = 2;

struct AnalogData {
  int min;
  int mid;
  int max;
  bool reverse;
};

AnalogData a0_data;
AnalogData a1_data;
AnalogData a2_data;
AnalogData a3_data;

struct MyData {
  // unsigned int throttle;
  // unsigned int yaw;
  // unsigned int pitch;
  // unsigned int roll;
  unsigned int TYPR[4];
  // bool AUX1;
  // bool AUX2;
  // bool AUX3;
  // bool AUX4;
  // bool AUX5;
  // bool AUX6;
  byte AUXS[6];
};

MyData data;

void(* resetFunc) (void) = 0;

void setup(){
  u8g2.setBusClock(1000000); //1Mbit/s (clock 1 MHz — Fast Mode Plus)
  u8g2.begin();
  //Define Button(menu) library/hardware interface
  //u8g2.begin(/*Select=*/ enc_press, /*Right/Next=*/ U8X8_PIN_NONE, /*Left/Prev=*/ U8X8_PIN_NONE, /*Up=*/ U8X8_PIN_NONE, /*Down=*/ U8X8_PIN_NONE, /*Home/Cancel=*/ U8X8_PIN_NONE); 
  Serial.begin(115200);

  logo();
  
  pinMode(in_aux1, INPUT_PULLUP);
  pinMode(in_aux2, INPUT_PULLUP);
  pinMode(in_aux3, INPUT_PULLUP);
  pinMode(in_aux4, INPUT_PULLUP);
  pinMode(in_aux5, INPUT_PULLUP);
  pinMode(in_aux6, INPUT_PULLUP);
  
  pinMode(in_a0, INPUT);
  pinMode(in_a1, INPUT);
  pinMode(in_a2, INPUT);
  pinMode(in_a3, INPUT);

  pinMode(enc_press, INPUT);
  pinMode(enc_cw, INPUT);
  pinMode(enc_ccw, INPUT);

  pinMode(in_battery, INPUT);

  data.TYPR[0] = 500;
  data.TYPR[1] = 500;
  data.TYPR[2] = 500;
  data.TYPR[3] = 500;
  data.AUXS[0] = 0;
  data.AUXS[1] = 0;
  data.AUXS[2] = 0;
  data.AUXS[3] = 0;
  data.AUXS[4] = 0;
  data.AUXS[5] = 0;

  a0_data.min = a0_data.mid = a0_data.max = -1;
  a1_data.min = a1_data.mid = a1_data.max = -1;
  a2_data.min = a2_data.mid = a2_data.max = -1;
  a3_data.min = a3_data.mid = a3_data.max = -1;

  a0_data.reverse = true;
  a1_data.reverse = true;
  a2_data.reverse = false;
  a3_data.reverse = false;

  //logo();
  //menu(0);

  /* NRF24 */
  // initialize the transceiver on the SPI bus
  delay(2000);
  if (!radio.begin()) {
    while (1) {radio_error();}  // hold in infinite loop
  }
  radio.begin();
  radio.setChannel(99); //0-124
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS); //RF24_1MBPS, RF24_2MBPS
  radio.openWritingPipe(my_radio_pipe);
  
  first_start();
  
  menu(0);
  Serial.println(radio.getPALevel());
}


void first_start(){
  if(EEPROM.read(0) == 200){ //address 0, first start.
    load_data();
  }
  else{
    calibrate();
  }

  if(EEPROM.read(32) == 200){
    load_data_io();
  }
}


void radio_error(){
  clear_area(16);
  u8g2.setFont(u8g2_font_4x6_mf);
  u8g2.setCursor(4, 32);
  u8g2.print("Radio 2.4 GHz");
  u8g2.setCursor(4, 39);
  u8g2.print("Error HW");
  u8g2.setCursor(24, 46);
  u8g2.print("! ! !");
  u8g2.updateDisplay();
  delay(1000);
  clear_area(16);
  u8g2.updateDisplay();
  delay(500);
}


int read_encoder(){
  if(digitalRead(enc_press) == 0)
    return 1;
  else if(digitalRead(enc_cw) == 0)
    return 2;
  else if(digitalRead(enc_ccw) == 0)
    return 3;
  else
    return -1;
}


float read_voltage(){
  float vout = 0.0;
  float vin = 0.0;
  float R1 = 30000.0; //  
  float R2 = 7500.0; // 
  int value = 0;

  value = analogRead(in_battery);
  //Serial.println(value);
  vout = (value * 5.0) / 1024.0; // see text
  vin = vout / (R2/(R1+R2));
  //Serial.println(vin);

  return vin;
}


void load_data(){
  // Load data from EEPROM
  Serial.print("[*] Loading data from EEPROM... ");

  a0_data.min = EEPROM.read(2)-22;
  a0_data.mid = EEPROM.read(3)+384; //512-128
  a0_data.max = 1023-EEPROM.read(4);

  a1_data.min = EEPROM.read(6)-22;
  a1_data.mid = EEPROM.read(7)+384; //512-128
  a1_data.max = 1023-EEPROM.read(8);

  a2_data.min = EEPROM.read(10)-22;
  a2_data.mid = EEPROM.read(11)+384; //512-128
  a2_data.max = 1023-EEPROM.read(12);

  a3_data.min = EEPROM.read(14)-22;
  a3_data.mid = EEPROM.read(15)+384; //512-128
  a3_data.max = 1023-EEPROM.read(16);

  Serial.println(" OK ");
}


void save_data(){
  // Save data to EEPROM
  // EEPROM.write(address, value)
  Serial.print("[*] Saving data to EEPROM... ");

  EEPROM.write(2, a0_data.min+22); //done
  EEPROM.write(3, a0_data.mid-384); //done
  EEPROM.write(4, 1023-a0_data.max); //done

  EEPROM.write(6, a1_data.min+22); //done
  EEPROM.write(7, a1_data.mid-384); //done
  EEPROM.write(8, 1023-a1_data.max); //done

  EEPROM.write(10, a2_data.min+22); //done
  EEPROM.write(11, a2_data.mid-384); //done
  EEPROM.write(12, 1023-a2_data.max); //done

  EEPROM.write(14, a3_data.min+22); //done
  EEPROM.write(15, a3_data.mid-384); //done
  EEPROM.write(16, 1023-a3_data.max); //done

  EEPROM.write(0, 200); //done first start

  int eeprom_cycle = 0;
  EEPROM.get(100, eeprom_cycle); // read
  eeprom_cycle++; // iter
  EEPROM.put(100, eeprom_cycle); // write

  Serial.println(" OK ");
  delay(100);
}


void load_data_io(){
    // Load data from EEPROM
  Serial.print("[*] Loading I/O data from EEPROM... ");

  a0_data.reverse = EEPROM.read(33); //1
  a1_data.reverse = EEPROM.read(34); //0 false is defined as 0 (zero).
  a2_data.reverse = EEPROM.read(35); //0
  a3_data.reverse = EEPROM.read(36); //1

  Serial.println(" OK ");
}


void save_data_io(){
  // Save data to EEPROM
  // EEPROM.write(address, value)

  EEPROM.write(33, a0_data.reverse);
  EEPROM.write(34, a1_data.reverse);
  EEPROM.write(35, a2_data.reverse);
  EEPROM.write(36, a3_data.reverse);

  EEPROM.update(32, 200); //tag

  int eeprom_cycle = 0;
  EEPROM.get(100, eeprom_cycle); // read
  eeprom_cycle++; // iter
  EEPROM.put(100, eeprom_cycle); // write

  delay(100);
}


void print_calib_data(){
  Serial.println(a0_data.min);
  Serial.println(a0_data.mid);
  Serial.println(a0_data.max);
  Serial.println("---------");
}


void calibrate(){
  menu(2);
  clear_area(16);
  
  u8g2.setFont(u8g2_font_4x6_mf);
  u8g2.setCursor(4, 22);
  u8g2.print("Calibrating...");
  u8g2.setCursor(4, 29);
  u8g2.print("Move joysticks");
  
  unsigned long timer_c = 0;
  bool done_calib = true;
  byte done_calib_continue = 0;
  
  //unsigned long previousMillis = millis();
  //while (millis() - previousMillis < 5000) {
  while(done_calib){
    int val0 = analogRead(in_a0);
    if(val0 > a0_data.max)
      a0_data.max = val0;
    else if(val0 < a0_data.min || a0_data.min == -1)
      a0_data.min = val0;

    int val1 = analogRead(in_a1);
    if(val1 > a1_data.max)
      a1_data.max = val1;
    else if(val1 < a1_data.min || a1_data.min == -1)
      a1_data.min = val1;

    int val2 = analogRead(in_a2);
    if(val2 > a2_data.max)
      a2_data.max = val2;
    else if(val2 < a2_data.min || a2_data.min == -1)
      a2_data.min = val2;

    int val3 = analogRead(in_a3);
    if(val3 > a3_data.max)
      a3_data.max = val3;
    else if(val3 < a3_data.min || a3_data.min == -1)
      a3_data.min = val3;

    if(millis()-timer_c > 0){

      byte y = 7;

      u8g2.setDrawColor(0);
      u8g2.drawBox(0, 32+y-1, 64, 13);
      u8g2.drawBox(0, 52+y-1, 64, 13);
      u8g2.drawBox(0, 72+y-1, 64, 13);
      u8g2.drawBox(0, 92+y-1, 64, 13);
      u8g2.setDrawColor(1);
    
      u8g2.setCursor(5, 40+y);
      u8g2.print(a0_data.min);
      u8g2.setCursor(46, 40+y);
      u8g2.print(a0_data.max);
      u8g2.drawFrame(0,32+y,64,11);
      u8g2.drawVLine(32, 32+y, 11);

      u8g2.setCursor(5, 60+y);
      u8g2.print(a1_data.min);
      u8g2.setCursor(46, 60+y);
      u8g2.print(a1_data.max);
      u8g2.drawFrame(0,52+y,64,11);
      u8g2.drawVLine(32, 52+y, 11);

      u8g2.setCursor(5, 80+y);
      u8g2.print(a2_data.min);
      u8g2.setCursor(46, 80+y);
      u8g2.print(a2_data.max);
      u8g2.drawFrame(0,72+y,64,11);
      u8g2.drawVLine(32, 72+y, 11);

      u8g2.setCursor(5, 100+y);
      u8g2.print(a3_data.min);
      u8g2.setCursor(46, 100+y);
      u8g2.print(a3_data.max);
      u8g2.drawFrame(0,92+y,64,11);
      u8g2.drawVLine(32, 92+y, 11);

      u8g2.setDrawColor(2);
      int x = 32;
      x = map(val0, 0, 1023, 0, 64);
      if(x > 32)
        u8g2.drawBox(33,32+y+1,x-32,9);
      else if(x < 32)
        u8g2.drawBox(x,32+y+1,32-x,9);

      x = map(val1, 0, 1023, 0, 64);
      if(x > 32)
        u8g2.drawBox(33,52+y+1,x-32,9);
      else if(x < 32)
        u8g2.drawBox(x,52+y+1,32-x,9);

      x = map(val2, 0, 1023, 0, 64);
      if(x > 32)
        u8g2.drawBox(33,72+y+1,x-32,9);
      else if(x < 32)
        u8g2.drawBox(x,72+y+1,32-x,9);

      x = map(val3, 0, 1023, 0, 64);
      if(x > 32)
        u8g2.drawBox(33,92+y+1,x-32,9);
      else if(x < 32)
        u8g2.drawBox(x,92+y+1,32-x,9);

      u8g2.setDrawColor(1);

      //int ret_encoder = u8g2.getMenuEvent(); //Returns: 0, if no button was pressed or a key pressed event.
      if(read_encoder() == 1){
        Serial.println("ENTER");
        done_calib_continue++;
        done_calib = false;

        if(a0_data.min > 100 || a1_data.min > 100 || a2_data.min > 100 || a3_data.min > 100){
          done_calib = true;
          clear_area(16);
          u8g2.setCursor(4, 22);
          u8g2.print("Calibrate fail");
          u8g2.setCursor(4, 29);
          u8g2.print("Min is > 100");
          u8g2.setCursor(4, 36);
          u8g2.print("Continue ?");
        }
        else if(a0_data.max < 924 || a1_data.max < 924 || a2_data.max < 924 || a3_data.max < 924) {
          done_calib = true;
          clear_area(16);
          u8g2.setCursor(4, 22);
          u8g2.print("Calibrate fail");
          u8g2.setCursor(4, 29);
          u8g2.print("Max is < 924");
          u8g2.setCursor(4, 36);
          u8g2.print("Continue ?");
        }

        if (done_calib_continue > 1) {
          done_calib = false;
          break;
        }

        Serial.println(done_calib_continue);
        u8g2.updateDisplay();
        delay(1000);
      }

      u8g2.drawButtonUTF8(32, 122, U8G2_BTN_HCENTER|U8G2_BTN_BW1, 30,  2,  2, "Done" );
      u8g2.setFontMode(0);

      u8g2.updateDisplay();
      
      timer_c = millis();
    }

  }

  clear_area(16);
  u8g2.setCursor(4, 22);
  u8g2.print("Centering...");
  u8g2.setCursor(4, 29);
  u8g2.print("Don't move the");
  u8g2.setCursor(4, 36);
  u8g2.print("joysticks !");
  u8g2.updateDisplay();
  delay(2000);
  
  bool done_center = true;
  int read_mid_dev[] = {0,0,0,0};
  int read_mid[] = {0,0,0,0};

  read_mid[0] = analogRead(in_a0);
  read_mid[1] = analogRead(in_a1);
  read_mid[2] = analogRead(in_a2);
  read_mid[3] = analogRead(in_a3);
  delay(100);

  while(done_center){
    
    clear_area(40);
    u8g2.drawStr(46, 43, "dev:");

    int val0 = analogRead(in_a0);
    if(abs(read_mid[0] - val0) > read_mid_dev[0])
      read_mid_dev[0] = abs(read_mid[0] - val0);

    read_mid[0] = val0;
    int x = map(val0, 0, 1023, 0, 64);
    u8g2.drawFrame(29,48,6,11);
    u8g2.drawVLine(x,47, 13);
    u8g2.setCursor(50, 55);
    u8g2.print(read_mid_dev[0]);

    int val1 = analogRead(in_a1);
    if(abs(read_mid[1] - val1) > read_mid_dev[1])
      read_mid_dev[1] = abs(read_mid[1] - val1);
      
    read_mid[1] = val1;
    x = map(val1, 0, 1023, 0, 64);
    u8g2.drawFrame(29,48+15,6,11);
    u8g2.drawVLine(x,47+15, 13);
    u8g2.setCursor(50, 55+15);
    u8g2.print(read_mid_dev[1]);

    int val2 = analogRead(in_a2);
    if(abs(read_mid[2] - val2) > read_mid_dev[2])
      read_mid_dev[2] = abs(read_mid[2] - val2);
      
    read_mid[2] = val2;
    x = map(val2, 0, 1023, 0, 64);
    u8g2.drawFrame(29,48+15*2,6,11);
    u8g2.drawVLine(x,47+15*2, 13);
    u8g2.setCursor(50, 55+15*2);
    u8g2.print(read_mid_dev[2]);

    int val3 = analogRead(in_a3);
    if(abs(read_mid[3] - val3) > read_mid_dev[3])
      read_mid_dev[3] = abs(read_mid[3] - val3);
      
    read_mid[3] = val3;
    x = map(val3, 0, 1023, 0, 64);
    u8g2.drawFrame(29,48+15*3,6,11);
    u8g2.drawVLine(x,47+15*3, 13);
    u8g2.setCursor(50, 55+15*3);
    u8g2.print(read_mid_dev[3]);

    a0_data.mid = val0;
    a1_data.mid = val1;
    a2_data.mid = val2;
    a3_data.mid = val3;

    if(read_encoder() == 1){
      Serial.println("ENTER");
      done_center = false;

      if(read_mid_dev[0] > 10 || read_mid_dev[1] > 10 || read_mid_dev[2] > 10 || read_mid_dev[3] > 10){
        done_center = true;
        clear_area(16);
        u8g2.setCursor(4, 22);
        u8g2.print("Deviation fail");
        u8g2.setCursor(4, 29);
        u8g2.print("Dev is > 10");
        u8g2.setCursor(4, 36);
        u8g2.print("Continue ?");
      }
    }

    u8g2.drawButtonUTF8(32, 122, U8G2_BTN_HCENTER|U8G2_BTN_BW1, 30,  2,  2, "Done" );
    u8g2.setFontMode(0);

    u8g2.updateDisplay();
  }
  
  delay(1000);
  clear_area(16);

  save_data();
  //print_data_from_eeprom
  //print_calib_data();
  
}


int mapJoystickValues255(int val, int lower, int middle, int upper, bool reverse){

  val = constrain(val, lower, upper);
  if ( val < middle )
    val = map(val, lower, middle, 0, 128);
  else
    val = map(val, middle, upper, 128, 255);
  return ( reverse ? 251 - val : val );
}


long maplong(long x, long in_min, long in_max, long out_min, long out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


int mapJoystickValues(int val, int lower, int middle, int upper, bool reverse){

  val = constrain(val, lower, upper);
  if ( val < middle )
    val = maplong(val, lower, middle, 0, 500);
  else
    val = maplong(val, middle, upper, 500, 1000);
  return ( reverse ? 1000 - val : val );
}


void print_data(){

  Serial.print("Throttle:"); Serial.print(data.TYPR[0]);  Serial.print("\t");
  Serial.print("Yaw:");      Serial.print(data.TYPR[1]);  Serial.print("\t");
  Serial.print("Pitch:");    Serial.print(data.TYPR[2]);  Serial.print("\t");
  Serial.print("Roll:");     Serial.print(data.TYPR[3]);  Serial.print("\t");
  Serial.print("Aux1:");     Serial.print(data.AUXS[0]);      Serial.print("\t");
  Serial.print("Aux2:");     Serial.print(data.AUXS[1]);      Serial.print("\t");
  Serial.print("Size:");     Serial.print(sizeof(MyData)); Serial.print("\n");
}


void logo(){

  static char dr1_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x01, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xF8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x03, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xF8, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xF0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x07, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xF8, 0x03, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0xF8, 0x01, 0x00, 0x00, 
  0xF8, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0xF8, 0x01, 0x00, 0x00, 
  0x38, 0x00, 0x00, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 
  0xF0, 0x03, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 
  0x1C, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0x03, 0xC0, 0xFF, 0x00, 0x00, 0x00, 
  0xF0, 0x1F, 0x00, 0xE0, 0xFF, 0x00, 0x00, 0x00, 0xFC, 0x3F, 0x00, 0xF0, 
  0xFF, 0x80, 0x03, 0x00, 0xFE, 0x3F, 0x00, 0xF0, 0xFF, 0x00, 0x0E, 0x00, 
  0xFF, 0x3F, 0x00, 0xF8, 0xFF, 0x00, 0x38, 0x00, 0xDF, 0x1F, 0x00, 0xFC, 
  0xFF, 0x00, 0xE0, 0xC0, 0xC7, 0x1F, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0xE7, 
  0xC1, 0x3F, 0x80, 0xFF, 0xFF, 0x07, 0x00, 0xFC, 0xC0, 0x3F, 0xE0, 0xFF, 
  0xFF, 0x07, 0x00, 0xF0, 0x80, 0x7F, 0xE0, 0xFF, 0xFF, 0x07, 0x00, 0xF8, 
  0x00, 0xFF, 0xF0, 0xFF, 0xFF, 0x03, 0x00, 0xFC, 0x00, 0xFC, 0xF9, 0xFF, 
  0xFF, 0x01, 0x00, 0xFC, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFC, 
  0x00, 0xE0, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0xFC, 0x00, 0xC0, 0xFF, 0xFF, 
  0x0F, 0x00, 0x00, 0xFC, 0x00, 0x80, 0xFF, 0xFF, 0x0F, 0x00, 0xF0, 0xFF, 
  0x00, 0xC0, 0xFF, 0xFF, 0x0F, 0xC0, 0xFF, 0xFF, 0x00, 0xF0, 0xE3, 0xFF, 
  0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0xF8, 0xFF, 0xFF, 0x87, 0xFF, 0xFF, 0xFF, 
  0x00, 0xF8, 0xFF, 0x7F, 0xFF, 0xFF, 0x3F, 0xFE, 0x00, 0xFC, 0xFF, 0xFF, 
  0xFF, 0xFF, 0x00, 0x3B, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x07, 0x80, 0x00, 
  0x00, 0xFF, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 0xFF, 
  0x0F, 0x00, 0x40, 0x00, 0xE0, 0xFF, 0xF7, 0xFF, 0x07, 0x00, 0x60, 0x00, 
  0xDF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x78, 0x00, 0xDF, 0xFF, 0xFF, 0xFF, 
  0x01, 0x00, 0x3C, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x3C, 0x00, 
  0xFF, 0x9F, 0xFF, 0xFF, 0x00, 0x00, 0x1E, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x00, 0x00, 0x0F, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0x01, 0x80, 0x07, 0x00, 
  0xF0, 0xFF, 0xFF, 0xFF, 0x03, 0xC0, 0x03, 0x00, 0xF8, 0xFF, 0xFF, 0xF9, 
  0x07, 0xC0, 0x03, 0x00, 0xF8, 0xFF, 0xFF, 0xF0, 0x0F, 0xEE, 0x01, 0x00, 
  0xF0, 0xFF, 0x3F, 0xC0, 0x1F, 0xFE, 0x00, 0x00, 0xC0, 0xFF, 0x1F, 0xFC, 
  0xFF, 0x3F, 0x00, 0x00, 0xC0, 0xB7, 0xDF, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 
  0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0x01, 0x00, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 
  0x00, 0xFF, 0x87, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0xF0, 0x83, 0x07, 
  0xF0, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0x07, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xC0, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0x3F, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xC0, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xF7, 0x03, 
  0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xC7, 0x07, 0x00, 0x00, 0x00, 0x00, 
  0x80, 0xFF, 0x87, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x07, 0x1E, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x07, 0x3C, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xE0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };


  //u8g2.clearBuffer();
  clear_area(16);
  u8g2.setFontMode(0);
  u8g2.drawXBM( 0, 30, dr1_width, dr1_height, dr1_bits);
  u8g2.setFont(u8g2_font_4x6_mf);
  u8g2.setCursor(44, 22+7*3);
  u8g2.print("v3");
  u8g2.updateDisplay();
  Serial.println("[+] Print LOGO");
  delay(100);
}


void clear_area(int horizont){
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, horizont, 64, 128-horizont);
  u8g2.setDrawColor(1);
}


void menu(byte menu){

  byte offset = 3;
  //u8g2.clearBuffer();
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 64, 16);
  u8g2.setDrawColor(1);

  // https://github.com/olikraus/u8g2/wiki/fntlist8
  // 8:
  //u8g2.setFont(u8g2_font_ncenB08_tf); //11737 but transparent shit
  // u8g2.setFont(u8g2_font_t0_11_mf); //11687
  //u8g2.setFont(u8g2_font_profont12_mf); //11629
  //u8g2.setFont(u8g2_font_spleen6x12_mf); // 11648
  // 6:
  //u8g2_font_spleen5x8_mf
  //u8g2_font_profont10_mf

  u8g2.setFontMode(0);
  u8g2.drawXBM( offset, 1, image_width, image_height, main_bits);
  u8g2.drawXBM( 12+offset, 1, image_width, image_height, chip_bits);
  u8g2.drawXBM( (12*2+offset), 1, image_width, image_height, calib_bits);
  u8g2.drawXBM( 12*3+offset, 1, image_width, image_height, signal_bits);
  u8g2.drawXBM( 12*4+offset, 1, image_width, image_height, batt_bits);
  int p = map(round(read_voltage()*100), round(3.4*2*100), round(4.2*2*100), 0, 5);
    if(p > 5)
      p = p;
  u8g2.drawBox(12*4+6,4+(5-p),4,p); //battery filler

  u8g2.setDrawColor(2);
  u8g2.drawBox(12*menu+2,0,12,12);
  u8g2.setDrawColor(1);

  u8g2.updateDisplay();
  delay(50);
}


void menu_choose(byte choose){
  Serial.println(choose);
  Serial.println("----");
  delay(100);
  if(choose == 0)
    logo();
  if(choose == 1)
    settings_io();
  else if(choose == 2)
    settings_analog();
  else if(choose == 3)
    settings_2_4_ghz();
  else if(choose == 4)
    settings_battery();
}


void insert_joy_io(){

  static unsigned char joy_bits[] = {
    0x00,0x40,0x00,0xe0,0x00,0xe0,0x00,0xe0,0x00,0xf0,0x01,0xe0,
    0x00,0xf8,0x03,0xe0,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0xe0,
    0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0xe0,
    0x00,0x00,0x00,0xe0,0x10,0x00,0x00,0xe1,0x18,0xe0,0x00,0xe3,
    0x1c,0xf0,0x01,0xe7,0x1e,0xf0,0x01,0xef,0x1c,0xf0,0x01,0xe7,
    0x18,0xe0,0x00,0xe3,0x10,0x00,0x00,0xe1,0x00,0x00,0x00,0xe0,
    0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0xe0,
    0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0xe0,0x00,0xf8,0x03,0xe0,
    0x00,0xf0,0x01,0xe0,0x00,0xe0,0x00,0xe0,0x00,0x40,0x00,0xe0
  };

  String MODE = "TYPR";
  
  if(radio_MODE == 1)
    MODE = "PYTR";
  else if(radio_MODE == 2)
    MODE = "TYPR";
  else if(radio_MODE == 3)
    MODE = "PRTY";
  else if(radio_MODE == 4)
    MODE = "TRPY";

  u8g2.drawXBM(2, 62, joy_width, joy_height, joy_bits);
  u8g2.setCursor(1+joy_width/2, 72);
  u8g2.print(MODE[0]);
  u8g2.setCursor(1+joy_width/2, 84);
  u8g2.print(MODE[0]);
  u8g2.setCursor(9, 78);
  u8g2.print(MODE[1]);
  u8g2.setCursor(21, 78);
  u8g2.print(MODE[1]);

  u8g2.drawXBM(2+joy_width+3, 62, joy_width, joy_height, joy_bits);
  u8g2.setCursor(4+joy_width+joy_width/2, 72);
  u8g2.print(MODE[2]);
  u8g2.setCursor(4+joy_width+joy_width/2, 84);
  u8g2.print(MODE[2]);
  u8g2.setCursor(41, 78);
  u8g2.print(MODE[3]);
  u8g2.setCursor(53, 78);
  u8g2.print(MODE[3]);
}


void settings_io(){

  delay(200);
  clear_area(16);
  bool button_mode = false;
  bool in = true;
  int pointer = 0;
  u8g2.setFont(u8g2_font_4x6_mf);

  u8g2.setCursor(5, 22+7*0);
  u8g2.print("Mode:        ");
  u8g2.print(radio_MODE);

  u8g2.setCursor(5, 22+7*1);
  u8g2.print("Reverse A0:  ");
  u8g2.drawXBM(50, 22+7*1-direct_height+1, direct_width, direct_height, left_bits);
  if(a0_data.reverse)
    u8g2.drawXBM(50, 22+7*1-direct_height+1, direct_width, direct_height, right_bits);
  u8g2.print(a0_data.reverse);

  u8g2.setCursor(5, 22+7*2);
  u8g2.print("Reverse A1:  ");
  u8g2.drawXBM(50, 22+7*2-up_down_height, up_down_width, up_down_height, down_bits);
  if(a1_data.reverse)
    u8g2.drawXBM(50, 22+7*2-up_down_height, up_down_width, up_down_height, up_bits);
  u8g2.print(a1_data.reverse);

  u8g2.setCursor(5, 22+7*3);
  u8g2.print("Reverse A2:  ");
  u8g2.drawXBM(50, 22+7*3-direct_height+1, direct_width, direct_height, right_bits);
  if(a2_data.reverse)
    u8g2.drawXBM(50, 22+7*3-direct_height+1, direct_width, direct_height, left_bits);
  u8g2.print(a2_data.reverse);

  u8g2.setCursor(5, 22+7*4);
  u8g2.print("Reverse A3:  ");
  u8g2.drawXBM(50, 22+7*4-up_down_height, up_down_width, up_down_height, up_bits);
  if(a3_data.reverse)
    u8g2.drawXBM(50, 22+7*4-up_down_height, up_down_width, up_down_height, down_bits);
  u8g2.print(a3_data.reverse);

  insert_joy_io();

  int eeprom_cycle = 0;
  EEPROM.get(100, eeprom_cycle); // read
  u8g2.setCursor(5, 22+7*12);
  u8g2.print("EEPROM CYCLES");
  u8g2.setCursor(5, 22+7*13);
  u8g2.print("(");
  u8g2.print(eeprom_cycle);
  u8g2.print("/100000)  ");

  u8g2.updateDisplay();

  while(in){

    int enc = read_encoder();
    if(enc > 1){
      if(enc == 3)
        pointer--;
      else if(enc == 2)
        pointer++;
    
      if(pointer > 5)
        pointer = 5;
      else if(pointer < 0)
        pointer = 0;

      Serial.println(pointer);
      //delay(40);

      u8g2.setDrawColor(0);
      u8g2.drawBox(0, 16, 4, 128-16);
      u8g2.setDrawColor(1);

      if(pointer > 0){
          u8g2.drawCircle(1, 22+7*(pointer-1)-3, 2, U8G2_DRAW_ALL);
      }

      if(pointer == 0){
        menu(1);
      }
      else {
        menu(6);
      }

    }
    else if (enc == 1){
      Serial.println("enter");
      delay(200);
      switch(pointer){
        case 0:
          in = false;
          break;
        case 1:
          radio_MODE++;
          if(radio_MODE > 4){
            radio_MODE = 1;
          }
          else if(radio_MODE < 1){
            radio_MODE = 4;
          }
          break;
        case 2:
          a0_data.reverse = !a0_data.reverse;
          save_data_io();
          break;
        case 3:
          a1_data.reverse = !a1_data.reverse;
          save_data_io();
          break;
        case 4:
          a2_data.reverse = !a2_data.reverse;
          save_data_io();
          break;
        case 5:
          a3_data.reverse = !a3_data.reverse;
          save_data_io();
          break;
      }

      u8g2.setCursor(5, 22+7*0);
      u8g2.print("Mode:        ");
      u8g2.print(radio_MODE);

      u8g2.setCursor(5, 22+7*1);
      u8g2.print("Reverse A0:  ");
      u8g2.drawXBM(50, 22+7*1-direct_height+1, direct_width, direct_height, left_bits);
      if(a0_data.reverse)
        u8g2.drawXBM(50, 22+7*1-direct_height+1, direct_width, direct_height, right_bits);
      u8g2.print(a0_data.reverse);

      u8g2.setCursor(5, 22+7*2);
      u8g2.print("Reverse A1:  ");
      u8g2.drawXBM(50, 22+7*2-up_down_height, up_down_width, up_down_height, down_bits);
      if(a1_data.reverse)
        u8g2.drawXBM(50, 22+7*2-up_down_height, up_down_width, up_down_height, up_bits);
      u8g2.print(a1_data.reverse);

      u8g2.setCursor(5, 22+7*3);
      u8g2.print("Reverse A2:  ");
      u8g2.drawXBM(50, 22+7*3-direct_height+1, direct_width, direct_height, right_bits);
      if(a2_data.reverse)
        u8g2.drawXBM(50, 22+7*3-direct_height+1, direct_width, direct_height, left_bits);
      u8g2.print(a2_data.reverse);

      u8g2.setCursor(5, 22+7*4);
      u8g2.print("Reverse A3:  ");
      u8g2.drawXBM(50, 22+7*4-up_down_height, up_down_width, up_down_height, up_bits);
      if(a3_data.reverse)
        u8g2.drawXBM(50, 22+7*4-up_down_height, up_down_width, up_down_height, down_bits);
      u8g2.print(a3_data.reverse);

      insert_joy_io();
      
      EEPROM.get(100, eeprom_cycle); // read
      u8g2.setCursor(5, 22+7*13);
      u8g2.print("(");
      u8g2.print(eeprom_cycle);
      u8g2.print("/100000)  ");

      u8g2.updateDisplay();
    }

  }

  clear_area(16);
  delay(100);
}


void settings_analog(){

  delay(200);
  bool button_calib = false;
  bool in = true;
  u8g2.setFont(u8g2_font_4x6_mf);

  while(in){
    
    control(0);

    clear_area(16);

    u8g2.drawFrame(7, 25, 32, 32);
    int x = map(data.TYPR[1], 0, 1023, -16, +16);
    u8g2.drawVLine((7+16)+x, 25, 32+5);
    int y = map(data.TYPR[0], 0, 1023, -16, +16);
    u8g2.drawHLine(1, (25+16)+y, 32+7-1);
    u8g2.drawXBM(0, (25+16)+y-1, j_width, j_height, j_bits);
    u8g2.drawHLine(1, 25+16*2+4, 7+16+x-1);
    u8g2.drawXBM(0, 25+16*2+4-1, j_width, j_height, j_bits);


    u8g2.drawFrame(25, 70, 32, 32);
    x = map(data.TYPR[3], 0, 1023, -16, +16);
    u8g2.drawVLine((25+16)+x, 70-5, 32+5);
    y = map(data.TYPR[2], 0, 1023, -16, +16);
    u8g2.drawHLine(25, (70+16)+y, 32+7-1);
    u8g2.drawXBM(62, (70+16)+y-1, j_width, j_height, j2_bits);
    u8g2.drawHLine(25+16+x, 70-5, 32+7-x-1);
    u8g2.drawXBM(62, 70-5-1, j_width, j_height, j2_bits);


    int enc = read_encoder();
    if(enc != -1){
      if(enc == 1 && button_calib)
        calibrate();
      else if(enc == 1 && !button_calib)
        in = false;
      else if(enc > 1){
        if(button_calib)
          button_calib = false;
        else
          button_calib = true;
      }
    }

    if(button_calib){
      u8g2.drawButtonUTF8(32, 122, U8G2_BTN_INV|U8G2_BTN_HCENTER|U8G2_BTN_BW1, 30,  2,  2, "Calibrate" );
      u8g2.setFontMode(0);
      menu(6);
    }
    else{
      u8g2.drawButtonUTF8(32, 122, U8G2_BTN_HCENTER|U8G2_BTN_BW1, 30,  2,  2, "Calibrate" );
      u8g2.setFontMode(0);
      menu(2);
    }
    
    
  }

  clear_area(16);
  delay(100);
  
}


void settings_2_4_ghz(){

  bool radio_connect = radio.isChipConnected();
  clear_area(16);
  u8g2.setFont(u8g2_font_4x6_mf);
  u8g2.setCursor(4, 22+(7*0));
  u8g2.print("HW 2.4GHz:");
  if(radio_connect)
    u8g2.print(" OK");
  else
    u8g2.print("ERROR");

  u8g2.setCursor(4, 22+(7*1));
  u8g2.print("Channel:   ");
  u8g2.print(radio.getChannel());

  u8g2.setCursor(4, 22+(7*2));
  u8g2.print("Valid:     ");
  u8g2.print(radio.isValid());

  u8g2.setCursor(4, 22+(7*3));
  u8g2.print("Radio pipe: ");
  u8g2.setCursor(4, 22+(7*4));
  u8g2.print(" 0xA8A8F0F0E1LL");

  u8g2.updateDisplay();
  delay(400);

  while(read_encoder() == -1){
    control(radio_MODE); // read data from analogs

    u8g2.setDrawColor(0);
    u8g2.drawBox(40, 58, 21, 57);
    u8g2.setDrawColor(1);

    u8g2.setCursor(4, 22+(7*6));
    u8g2.print("TYPR[0]:  "); u8g2.print(data.TYPR[0]);
    u8g2.setCursor(4, 22+(7*7));
    u8g2.print("TYPR[1]:  ");      u8g2.print(data.TYPR[1]);
    u8g2.setCursor(4, 22+(7*8));
    u8g2.print("TYPR[2]:  ");    u8g2.print(data.TYPR[2]);
    u8g2.setCursor(4, 22+(7*9));
    u8g2.print("TYPR[3]:  ");     u8g2.print(data.TYPR[3]);
    u8g2.setCursor(4, 22+(7*10));
    u8g2.print("Aux[1-6]:");
    u8g2.setCursor(4, 22+(7*11));
    u8g2.print(" ["); 
    u8g2.print(data.AUXS[0]);
    u8g2.print(data.AUXS[1]);
    u8g2.print(data.AUXS[2]);
    u8g2.print(data.AUXS[3]);
    u8g2.print(data.AUXS[4]);
    u8g2.print(data.AUXS[5]);
    u8g2.print("]");

    u8g2.setCursor(4, 22+(7*13));
    u8g2.print("Size:      ");
    u8g2.print(sizeof(MyData));
    u8g2.print("b");
    u8g2.updateDisplay();
  }

  clear_area(16);
  delay(100);
}


void settings_battery(){
  clear_area(16);
  delay(200);
  

  const byte img_battery_w = 22;
  const byte img_battery_h = 44;
  static unsigned char image_battery[] = {
    0xc0,0xff,0xc0,0xc0,0xff,0xc0,0xfe,0xff,0xdf,0xff,0xff,0xff,
    0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,
    0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,
    0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,
    0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,
    0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,
    0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,
    0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,
    0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,
    0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,0x03,0x00,0xf0,
    0x03,0x00,0xf0,0x03,0x00,0xf0,0xff,0xff,0xff,0xfe,0xff,0xdf
  };

  int x = 22;
  int y = 50;

  int percent = 0; //8.4V = 100%
  float volt = 0.0;
  bool button_reset = false;
  bool in = true;

  byte i = 0;

  while(in){

    volt = volt + read_voltage();
    i++;
    if(i > 9){
      clear_area(16);
      u8g2.drawXBM(x, y, img_battery_w, img_battery_h, image_battery);

      u8g2.setFont(u8g2_font_4x6_mf);
      u8g2.setCursor(5, 22+7*0);
      u8g2.print("Run:  ");

      unsigned long timer_r = millis();
      unsigned long seconds = timer_r/1000;
      unsigned long minutes = seconds/60;
      unsigned long hours = minutes/60;
      seconds %= 60;
      minutes %= 60;
      hours %= 24;

      if(hours < 10)
        u8g2.print("0");
      u8g2.print(hours);
      u8g2.print(":");
      if(minutes < 10)
        u8g2.print("0");
      u8g2.print(minutes);
      u8g2.print(":");
      if(seconds < 10)
        u8g2.print("0");
      u8g2.print(seconds);

      volt = volt/10;
      u8g2.setFont(u8g2_font_5x8_mf);
      u8g2.setCursor(x, y-5);
      u8g2.print(volt);
      u8g2.print("V");

      percent = map(round(volt*100), round(3.4*2*100), round(4.2*2*100), 0, 100);
      if(percent > 100)
        percent = 100;

      u8g2.setCursor(x+2, y+img_battery_h+10);
      u8g2.print(percent);
      u8g2.print("%");

      int fill = map(percent, 0, 100, 0, img_battery_h-8);
      u8g2.drawBox(x+3,y+5+(img_battery_h-8-fill),16,fill); //battery filler

      i = 0;
      volt = 0.0;
    }
    else{
      clear_area(115);
    }

    int enc = read_encoder();
    if(enc != -1){
      if(enc == 1 && button_reset)
        resetFunc();
      else if(enc == 1 && !button_reset)
        in = false;
      else if(enc > 1){
        if(button_reset)
          button_reset = false;
        else
          button_reset = true;
      }
    }

    if(button_reset){
      u8g2.setFont(u8g2_font_4x6_mf);
      u8g2.drawButtonUTF8(32, 122, U8G2_BTN_INV|U8G2_BTN_HCENTER|U8G2_BTN_BW1, 30,  2,  2, "Reboot SW" );
      u8g2.setFontMode(0);
      menu(6);
    }
    else{
      u8g2.setFont(u8g2_font_4x6_mf);
      u8g2.drawButtonUTF8(32, 122, U8G2_BTN_HCENTER|U8G2_BTN_BW1, 30,  2,  2, "Reboot SW" );
      u8g2.setFontMode(0);
      menu(4);
    }
  }

  clear_area(16);
  delay(100);
}


void control(byte mode){
  if(mode >= 2){
    data.TYPR[3] = mapJoystickValues(analogRead(in_a0), a0_data.min+1, a0_data.mid, a0_data.max-1, a0_data.reverse);
    data.TYPR[2] = mapJoystickValues(analogRead(in_a1), a1_data.min+1, a1_data.mid, a1_data.max-1, a1_data.reverse);
    data.TYPR[0] = mapJoystickValues(analogRead(in_a2), a2_data.min+1, a2_data.mid, a2_data.max-1, a2_data.reverse);
    data.TYPR[1] = mapJoystickValues(analogRead(in_a3), a3_data.min+1, a3_data.mid, a3_data.max-1, a3_data.reverse);
  }
  else {
    data.TYPR[1] = mapJoystickValues(analogRead(in_a0), a0_data.min+1, a0_data.mid, a0_data.max-1, a0_data.reverse);
    data.TYPR[0] = mapJoystickValues(analogRead(in_a1), a1_data.min+1, a1_data.mid, a1_data.max-1, !a1_data.reverse); // ! = REVERS
    data.TYPR[3] = mapJoystickValues(analogRead(in_a2), a2_data.min+1, a2_data.mid, a2_data.max-1, a2_data.reverse);
    data.TYPR[2] = mapJoystickValues(analogRead(in_a3), a3_data.min+1, a3_data.mid, a3_data.max-1, !a3_data.reverse); // ! = REVERS
  }

  data.AUXS[5] = !digitalRead(in_aux1); // ! = REVERS
  data.AUXS[4] = !digitalRead(in_aux2); // ! = REVERS
  data.AUXS[3] = !digitalRead(in_aux3); // ! = REVERS
  data.AUXS[2] = !digitalRead(in_aux4); // ! = REVERS
  data.AUXS[1] = !digitalRead(in_aux5); // ! = REVERS
  data.AUXS[0] = !digitalRead(in_aux6); // ! = REVERS
}


void loop(){

  control(radio_MODE); // read data from analogs
  
  radio.write(&data, sizeof(MyData));

  int enc = read_encoder();
  if(enc > 1){
    if(enc == 3)
      in_menu--;
    else if(enc == 2)
      in_menu++;
    
    if(in_menu > 4)
      in_menu = 0;
    else if(in_menu < 0)
      in_menu = 4;

    menu(in_menu);
  }
  else if (enc == 1){
    menu_choose(in_menu);
  }
}
