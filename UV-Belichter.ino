/*  UV_TIMER    21.08.2016    */
//#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
Servo myservo;
#define I2CADR 0x20 //0x20 Pollin, 0x27 alle anderen Displays
//                    I2C-ADR,EN,RW,RS,Bit#0 D4,Bit#1 D5,Bit#2 D6,Bit#3 D7 
LiquidCrystal_I2C lcd( I2CADR, 2, 1, 0,       4,       5,       6,       7);//,3, POSITIVE);
#define DI1   3 //PD3 //Drehimpulsgeber1
#define DI2   4 //PD4 //Drehimpulsgeber2
#define TA    2 //PD2 //Taster Drehimpulsgeber
#define SERVO 5 //PD5 //Servo für Aetzbad bewegen
#define LED   6 //PD6 //grüne LED am Scanner
#define TA_SC 7 //PD7 //Grüne Taste am Scanner
#define PIEP  A2 //PC2 //Ausgang für Piepser
#define UV    A3 //PC3 //UV Ein-/Ausschalten
unsigned long tempo [] {0,0}, t_blinken=0, t_sek=0, t_push1=0, t_push2=0, t_dsp_akt=0, ti_alarm1, ti_alarm2, t_servo=0, t1_servo=0;
int sek=270, returnwert_old=0, tempopos=0, ta_flag=0, ta_flag_old=0, servo_pos=0;
boolean di_flanke=false, ta_flanke=false, alarm_flag=false, alarm_flag2=false, servo_dir=false, servo_flag=false;
/*** SETUP      ***************************************************************/
void setup() {
  Serial.begin(115200);
  pinMode (UV,   OUTPUT);
  pinMode (PIEP, OUTPUT);
  pinMode (SERVO,OUTPUT);
  pinMode (LED,  OUTPUT);
  pinMode (DI1,  INPUT);
  pinMode (DI2,  INPUT);
  pinMode (TA,   INPUT);
  pinMode (TA_SC,INPUT);
  digitalWrite(UV,LOW);
  digitalWrite(PIEP,LOW);
  digitalWrite(LED,HIGH);
  lcd.begin(8,2);
  lcd.home();
  lcd.print("UV LICHT");
  lcd.setCursor(0,1);
  lcd.print("21.08.16");
  myservo.attach(SERVO);
  delay(2000);
  Wire.begin(); //für Temperatursensor LM75A nötig
}
/*** LOOP       ***************************************************************/
void loop() {
  anzeige();
  zeiteinstellung();
  startknopf();
  betrieb();
  alarm(); 
  servo();
}
/*** anzeige()  ***************************************************************/
void anzeige() {
  int n;
  if(display_akt()==true) { //display nur alle 200ms aktualisieren
    lcd.setCursor(0,0);
    if((millis()-tempo[tempopos])>60000&&(ta_flag==0||ta_flag==3)) { //60 Sekunden keine Eingabe
      lcd.print(lm75a(),1);
      lcd.print(" Cel");
    }
    else {
      if((sek>0&&ta_flag==0) ||ta_flag==1) {
        servo_flag=false;
        lcd.print("Count UV");
      }
      if((sek==0&&ta_flag <4)||ta_flag==3) {
        servo_flag=true;
        lcd.print("StopUhr ");
      }
      if(sek<0 ||ta_flag==2) {
        servo_flag=true;
        lcd.print("nur CNT ");
      }
      if(ta_flag>4) {
        servo_flag=false;
        lcd.print("Fertig  ");
      }
    }
    lcd.setCursor(0,1);
    if(sek< 600&&sek>=0) lcd.print("   0");
    if(sek>=600)         lcd.print("   ");
    if(sek<0&&sek>-600)  lcd.print("  -0");
    if(sek<=-600)        lcd.print("  -");
    if(sek>=0) n=sek;
    else n=sek*(-1);
    lcd.print(n/60);
    lcd.print(":");
    if(n%60<10) lcd.print("0");
    lcd.print(n%60);
  }
}
/*** zeiteinstellung  *********************************************************/
void zeiteinstellung() {
  int wert=10;
  if(sek<30&&sek>-30) wert=1;
  switch(drehimpuls()) {  //0=kein Impuls, 1=langsam CW, 2=schnell CW, 3=langsam CCW, 4=schnell CCW
    case 1: { sek+=wert; break; }
    case 2: { sek+=(3*wert); break; }
    case 3: { sek-=wert; break; }
    case 4: { sek-=3*wert; break; }
    default: {}
  }
}
/*** STARTKNOPF  **************************************************************/
void startknopf(){
  if(push()==true) {
    if(ta_flag==0) {
      if(sek>0) {
        digitalWrite(UV,HIGH);
        ta_flag=1;
      }
      if(sek <0)ta_flag=2;
      if(sek==0)ta_flag=3;
    }
    if(ta_flag >4) {
      ta_flag=0;
      alarm_flag=false;
    }
  }
}
/*** BETRIEB   (jetzt läuft die Zeit)  ****************************************/
void betrieb() {
  if(ta_flag==1) {
    if((millis()-t_blinken)>500) {
      digitalWrite(LED, !digitalRead(LED));
      t_blinken=millis();
    }
    if(sekunde()==true) sek--;
    if(sek==0) {
      alarm_flag=true;
      digitalWrite(UV,LOW);
      digitalWrite(LED,HIGH);
      ta_flag=5;
    }
  }
  if(ta_flag==2) {
    if(sekunde()==true) sek++;
    if(sek==0) {
      alarm_flag=true;
      ta_flag=6;
    }
  }
  if(ta_flag==3) {
    if(sekunde()==true) sek++;
    if(push()==true) ta_flag=7;
  }
}
/*** PUSH   Knopf am Drehimpulsgeber gedrückt?   ******************************/
boolean push() {
  if(digitalRead(TA)==HIGH) {
    if(ta_flanke=true) {
      ta_flanke=false;
      t_push2=millis()-t_push1;
    }
  }
  else {
    if(ta_flanke==false) {
      ta_flanke=true;
      t_push1=millis();
      return(true);
    }
  }
  return(false);
}
/*** SEKUNDE   wenn eine Sekunde rum ist true, sonst false ********************/
boolean sekunde() {
  if(millis()-t_sek>1000) {
    t_sek=millis();
    return(true);
  }
  else return(false);
}
/*** DISPLAY-aktualisierung   ****************************************************/
boolean display_akt() {
  if(millis()-t_dsp_akt>200) {
    t_dsp_akt=millis();
    return(true);
  }
  else return(false);
}
/*** ALARM   Piepser auslösen**************************************************/
void alarm () {
  if(alarm_flag==true){
    if((millis()-ti_alarm2)>2000) { //alle 2Sekunden für 2 Sekunden Piepsen
      ti_alarm2=millis();
      alarm_flag2=!alarm_flag2;
    }
    if((micros()-ti_alarm1)>1000) { //1KHz Ton erzeugen (Mikros=einmillionstel Sekunde)
        ti_alarm1=micros();
        if(alarm_flag2==true) digitalWrite(PIEP, !digitalRead(PIEP));
    }
  }
  else digitalWrite(PIEP, LOW); //definierten Pegel im Aus-Zustand herstellen
}
/*** DREHIMPULS  **************************************************************/
int drehimpuls() {
  int tempoflag, returnwert, cw; 
  if(digitalRead(DI1)==HIGH) di_flanke=false;
  else {
    if(di_flanke==false) {
      cw=digitalRead(DI2);
      di_flanke=true;
      tempo[tempopos]=millis();
      if((tempo[tempopos]-tempo[(tempopos+1)%2]) > 60)  tempoflag=0;
      else tempoflag=1;
      tempopos++;
      if(tempopos>1) tempopos=0;
      if(cw==HIGH) returnwert=1+tempoflag; //welche Drehrichtung
      else returnwert=3+tempoflag;
      if((returnwert==2&&returnwert_old>3)||(returnwert==4&&returnwert_old<3)) returnwert=0; // ungültige Kombination
      returnwert_old=returnwert;
      return(returnwert);
    }
  }
  return(0);
}
//***Temeratursensor lm75a abfragen************************************
float lm75a() {
  Wire.requestFrom(0x48,2);
  while(Wire.available()) {
    int8_t msb = Wire.read();
    int8_t lsb = Wire.read();
    lsb = (lsb & 0x80 ) >> 7; //strip one bit of the lsb, now lsb = 0 or 1
    float f=msb + 0.5 * lsb;
    return(f);// add to to form an float
  }
}
void servo(){
  if(servo_flag==true) {
    if((millis()-t_servo)>200) {
      t_servo=millis();
      if(servo_pos<= 175&&servo_dir==true ) servo_pos+=5;
      if(servo_pos>=180 &&(millis()-t1_servo>12000)) {
        t1_servo=millis();
        servo_dir=false;
      }
      if(servo_pos>=   5&&servo_dir==false) servo_pos-=5;
      if(servo_pos<=0 && (millis()-t1_servo>12000)) {
        t1_servo=millis();
        servo_dir=true;
      }
      Serial.println(servo_pos);
      myservo.write(servo_pos);
    }
  }
  /* val = analogRead(potpin);            // reads the value of the potentiometer (value between 0 and 1023) 
  val = map(val, 0, 1023, 0, 180);     // scale it to use it with the servo (value between 0 and 180) 
  myservo.write(val);                  // sets the servo position according to the scaled value 
  delay(15);                           // waits for the servo to get there */
} 
