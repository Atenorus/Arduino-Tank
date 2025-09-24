#include <IRremote.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <avr/wdt.h>

// --- Pins ---
const int RECV_PIN = 13;
int LeftMotorA = 11, LeftMotorB = 10, RightMotorA = 5, RightMotorB = 6;
int LeftMotorEnable = 2, RightMotorEnable = 3;
int buzzerPin = 4;
const int trigPin = 7, echoPin = 12;

// --- Variables générales ---
unsigned long lastCode = 0, lastSignalTime = 0, lastIrActivity = 0;
bool lcdIsOn = true;
unsigned long lcdTimeTurnOff = 30000;
const unsigned long timeout = 200;
int speedState = 2, speedValue1 = 150, speedValue2 = 200, speedValue3 = 255;
int lastDistanceCm = 5;
int STOP_DISTANCE_CM = 10;
unsigned long lastPingTime = 0;
const unsigned long pingInterval = 50;
unsigned long lastBeepTime = 0;
int klaxonStyle = 1;
int tankCode = 1111;

// --- Modes ---
int safetyMode = 0; // 0=STOP ≤10cm, 1=BIP, 2=OFF
bool autoSpeedEnabled = false;
bool autoModeEnabled = false;

// --- État du mouvement ---
enum MoveState { STOPPED, FORWARD, BACKWARD, LEFT, RIGHT };
MoveState currentMove = STOPPED;

// --- LCD I2C ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long lastLCDupdate = 0;
const unsigned long lcdInterval = 200;

/////////////////
/// ICONS LCD ///
/////////////////

// cadenas fermer
byte lockClosed[8] = {
  B01110, B10001, B10001, B11111, B11011, B11111, B11111, B00000
};

// cadenas ouvert
byte lockOpen[8] = {
  B01110, B10001, B00001, B11111, B11011, B11111, B11111, B00000
};

// --- EEPROM ---
const int ADDR_SPEED = 0;      // int = 2 octets
const int ADDR_SPEED1 = 2;     // int = 2 octets
const int ADDR_SPEED2 = 4;     // int = 2 octets
const int ADDR_SPEED3 = 6;     // int = 2 octets
const int ADDR_SAFETY = 8;     // int = 2 octets
const int ADDR_AUTO = 10;      // bool = 1 octet
const int ADDR_STOPDIST = 11;  // int = 2 octets
const int ADDR_KLAXON = 13;    // int = 2 octets
const int ADDR_CODE = 15;      // int = 2 octets
const int ADDR_VEILLE = 17;    // int = 2 octets
const int ADDR_LOCK = 19;      // bool = 1 octets


// --- Fonctions moteur ---
void setSpeed(int speed = 2) {
  speedState = constrain(speed, 1, 3);
  int newSpeed = 200;
  switch(speedState){
    case 1: newSpeed = speedValue1; break;
    case 2: newSpeed = speedValue2; break;
    case 3: newSpeed = speedValue3; break;
  }
  analogWrite(LeftMotorEnable, newSpeed);
  analogWrite(RightMotorEnable, newSpeed);
}

void Avant() { digitalWrite(RightMotorA,HIGH); digitalWrite(RightMotorB,LOW);
              digitalWrite(LeftMotorA,HIGH); digitalWrite(LeftMotorB,LOW);}
void Arriere() { digitalWrite(RightMotorA,LOW); digitalWrite(RightMotorB,HIGH);
                 digitalWrite(LeftMotorA,LOW); digitalWrite(LeftMotorB,HIGH);}
void Gauche() { digitalWrite(RightMotorA,LOW); digitalWrite(RightMotorB,HIGH);
                digitalWrite(LeftMotorA,HIGH); digitalWrite(LeftMotorB,LOW);}
void Droite() { digitalWrite(RightMotorA,HIGH); digitalWrite(RightMotorB,LOW);
                digitalWrite(LeftMotorA,LOW); digitalWrite(LeftMotorB,HIGH);}
void Stop() { digitalWrite(RightMotorA,LOW); digitalWrite(RightMotorB,LOW);
              digitalWrite(LeftMotorA,LOW); digitalWrite(LeftMotorB,LOW);}
void Frein() { digitalWrite(RightMotorA,HIGH); digitalWrite(RightMotorB,HIGH);
               digitalWrite(LeftMotorA,HIGH); digitalWrite(LeftMotorB,HIGH);}

// --- Capteur ultrason ---
int measureDistanceCm() {
  digitalWrite(trigPin,LOW); delayMicroseconds(2);
  digitalWrite(trigPin,HIGH); delayMicroseconds(10);
  digitalWrite(trigPin,LOW);
  unsigned long dur = pulseIn(echoPin,HIGH,15000UL);
  if(dur==0) return 999;
  return (int)(dur*0.0343/2.0+0.5);
}

// --- Buzzer ---
void beep(int freq,int duration){
  long delayValue = 1000000L/(freq*2);
  long cycles = (long)freq*duration/1000;
  for(long i=0;i<cycles;i++){
    digitalWrite(buzzerPin,HIGH);
    delayMicroseconds(delayValue);
    digitalWrite(buzzerPin,LOW);
    delayMicroseconds(delayValue);
  }
}

void Klaxon() {
  switch(klaxonStyle){
    case 1: beep(1000,50); delay(100); beep(200,50); delay(100); beep(500,50); break;
    case 2: for(int r=0;r<2;r++){for(int f=400;f<=800;f+=10) beep(f,15); for(int f=800;f>=400;f-=10) beep(f,15);} break;
    case 3: beep(200,1000); break;
  }
}

void distanceBeeper() {
  if(lastDistanceCm>30) return;
  int dist = constrain(lastDistanceCm,10,30);
  unsigned long interval = map(dist,10,30,50,500);
  if(millis()-lastBeepTime>=interval){ beep(300,50); lastBeepTime=millis();}
}

void autoAdjustSpeed(){
  if(!autoSpeedEnabled) return;
  if(safetyMode==0 && lastDistanceCm<=STOP_DISTANCE_CM){ Stop(); return; }
  if(lastDistanceCm>STOP_DISTANCE_CM*3) setSpeed(3);
  else if(lastDistanceCm>STOP_DISTANCE_CM*2) setSpeed(2);
  else setSpeed(1);
}

// --- LCD ---
void updateLCD() {
  // ligne 1
  lcd.setCursor(0,0);
  if(safetyMode==0) lcd.print("Radar:ON ");
  else if(safetyMode==1) lcd.print("Radar:BIP");
  else lcd.print("Radar:OFF");

  lcd.setCursor(11,0); lcd.print("Klx:"); lcd.print(klaxonStyle);

  // ligne 2 : vitesse et modes
  lcd.setCursor(0,1); lcd.print("Speed:");
  lcd.print(speedState);

  lcd.setCursor(8,1);
  if(autoModeEnabled) lcd.print("FullAuto");
  else lcd.print(autoSpeedEnabled?"Auto    ":"Manuel  ");
}

/******************************
** Fonction materiel Arduino **
*******************************/
void rebootArduino(){
  wdt_enable(WDTO_15MS);
  while(1){}
}

// --- EEPROM ---
void loadSettings(){
  EEPROM.get(ADDR_SPEED,speedState);
  EEPROM.get(ADDR_SPEED1,speedValue1);
  EEPROM.get(ADDR_SPEED2,speedValue2);
  EEPROM.get(ADDR_SPEED3,speedValue3);
  EEPROM.get(ADDR_SAFETY,safetyMode);
  EEPROM.get(ADDR_AUTO,autoSpeedEnabled);
  EEPROM.get(ADDR_STOPDIST,STOP_DISTANCE_CM);
  EEPROM.get(ADDR_KLAXON,klaxonStyle);
  EEPROM.get(ADDR_VEILLE,lcdTimeTurnOff);
  EEPROM.get(ADDR_CODE,tankCode);

  // Vérifications pour éviter les valeurs corrompues
  if(speedState<1||speedState>3) speedState=2;
  if(speedValue1<150||speedValue1>255) speedValue1=150;
  if(speedValue2<150||speedValue2>255) speedValue2=200;
  if(speedValue3<150||speedValue3>255) speedValue3=255;
  if(safetyMode<0||safetyMode>2) safetyMode=0;
  if(klaxonStyle<1||klaxonStyle>3) klaxonStyle=1;
  if(lcdTimeTurnOff<10000||lcdTimeTurnOff>120000) lcdTimeTurnOff=30000;
  if(tankCode<0000||tankCode>9999) tankCode=1111;

  lcd.clear(); lcd.setCursor(1,0);
  lcd.print("Params Loaded");
  delay(1000); lcd.clear();
}

void resetSettings() {
  speedState = 2;
  speedValue1 = 150;
  speedValue2 = 200;
  speedValue3 = 255;
  safetyMode = 0;
  autoSpeedEnabled = false;
  STOP_DISTANCE_CM = 10;
  klaxonStyle = 1;
  lcdTimeTurnOff = 30000;
  tankCode = 1111;

  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Params Reset");
  delay(1000); lcd.clear();
}

void saveSettings(){
  EEPROM.put(ADDR_SPEED, speedState);
  EEPROM.put(ADDR_SPEED1,speedValue1);
  EEPROM.put(ADDR_SPEED2,speedValue2);
  EEPROM.put(ADDR_SPEED3,speedValue3);
  EEPROM.put(ADDR_SAFETY, safetyMode);
  EEPROM.put(ADDR_AUTO, autoSpeedEnabled);
  EEPROM.put(ADDR_STOPDIST, STOP_DISTANCE_CM);
  EEPROM.put(ADDR_KLAXON, klaxonStyle);
  EEPROM.put(ADDR_VEILLE, lcdTimeTurnOff);
  delay(1000);
  lcd.clear(); lcd.setCursor(2,0);
  lcd.print("Params Saved");
  delay(1000); lcd.clear();
}



// --- Menu ---
int getIrCode(const char* lcdLabel) {
  int digits[4] = {0,0,0,0};
  int pos = 0;
  unsigned long lastCode = 0;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(lcdLabel);

  bool validated = false;

  while (!validated) {
    if (IrReceiver.decode()) {
      unsigned long irCode = IrReceiver.decodedIRData.decodedRawData;
      if (irCode == 0xFFFFFFFF) irCode = lastCode;
      else lastCode = irCode;

      int digit = -1;

      switch (irCode) {
        case 0xE916FF00: digit = 1; break;
        case 0xE619FF00: digit = 2; break;
        case 0xF20DFF00: digit = 3; break;
        case 0xF30CFF00: digit = 4; break;
        case 0xE718FF00: digit = 5; break;
        case 0xA15EFF00: digit = 6; break;
        case 0xF708FF00: digit = 7; break;
        case 0xE31CFF00: digit = 8; break;
        case 0xA55AFF00: digit = 9; break;
        case 0xAD52FF00: digit = 0; break;
        case 0xBB44FF00: // Effacer
          if (pos > 0) {
            pos--;
            digits[pos] = 0;
            lcd.setCursor(pos,1);
            lcd.print(" ");
            lcd.setCursor(pos,1);
          }
          break;
        case 0xBF40FF00: // Valider
          if (pos > 0) validated = true; // valide même si moins de 4 chiffres
          break;
      }

      if (digit != -1 && pos < 4) {
        digits[pos] = digit;
        lcd.setCursor(pos,1);
        lcd.print("*");
        pos++;
      }

      IrReceiver.resume();
    }
    delay(50);
  }

  // convertir les chiffres saisis en int
  int enteredCode = 0;
  for (int i = 0; i < pos; i++) {
    enteredCode = enteredCode*10 + digits[i];
  }
  return enteredCode;
}

void initLockChars() {
  lcd.createChar(0, lockClosed); // 🔒
  lcd.createChar(1, lockOpen);   // 🔓
}

void maxTentative(int timeIsLocked = 10){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Max tentatives!");
  EEPROM.put(ADDR_LOCK, true);
  
  while(timeIsLocked > 0){
    lcd.setCursor(0, 1);
    lcd.write(byte(0));  // affiche 🔒 (cadenas fermé)

    lcd.setCursor(2, 1);
    lcd.print("Bloque: ");
    lcd.print(timeIsLocked);
    lcd.print("s ");

    delay(1000);
    timeIsLocked--;
  }

  // petite animation de déblocage 🔓
  EEPROM.put(ADDR_LOCK, false);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write(byte(1)); // 🔓
  lcd.setCursor(2,0);
  lcd.print("Debloque !");
  delay(1500);

  lcd.clear();
}

int checkCode(const char* lcdLabel, int secretCode, int maxAttempts=3) {
  while (true) {
    int attempt = 0;

    while (attempt < maxAttempts) {
      int entered = getIrCode(lcdLabel);

      if (entered == secretCode) return 1;
      else {
        attempt++;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Mauvais code!");
        lcd.setCursor(0,1);
        lcd.print("Essai ");
        lcd.print(attempt);
        lcd.print("/");
        lcd.print(maxAttempts);
        delay(2000);
      }
    }

    // trop d'essais
    maxTentative(30);
  }
}



void setStopDistance(){
  lcd.clear(); lcd.setCursor(0,0); lcd.print("Distance Arret:");
  bool done=false; int lastDisplayed=-1;
  while(!done){
    if(IrReceiver.decode()){
      unsigned long code=IrReceiver.decodedIRData.decodedRawData;
      if(code==0xFFFFFFFF) code=lastCode; else lastCode=code;
      switch(code){
        case 0xB946FF00: STOP_DISTANCE_CM++; break;
        case 0xEA15FF00: STOP_DISTANCE_CM--; break;
        case 0xBF40FF00: done=true; break;}
      STOP_DISTANCE_CM=constrain(STOP_DISTANCE_CM,5,50);
      if(STOP_DISTANCE_CM!=lastDisplayed){ lcd.setCursor(7,1); lcd.print("    "); lcd.setCursor(7,1); lcd.print(STOP_DISTANCE_CM); lastDisplayed=STOP_DISTANCE_CM; }
      IrReceiver.resume();
    }
    delay(50);
  }
  lcd.clear();
}

void setKlaxonStyle(){
  lcd.clear(); lcd.setCursor(0,0); lcd.print("Klaxon Style:");
  bool done=false; int lastDisplayed=-1;
  while(!done){
    if(IrReceiver.decode()){
      unsigned long code=IrReceiver.decodedIRData.decodedRawData;
      if(code==0xFFFFFFFF) code=lastCode; else lastCode=code;
      switch(code){ 
        case 0xE916FF00: klaxonStyle=1; break;
        case 0xE619FF00: klaxonStyle=2; break;
        case 0xF20DFF00: klaxonStyle=3; break;
        case 0xBF40FF00: done=true; break;}
      if(klaxonStyle!=lastDisplayed){ lcd.setCursor(7,1); lcd.print("  "); lcd.setCursor(7,1); lcd.print(klaxonStyle); lastDisplayed=klaxonStyle; }
      IrReceiver.resume();
    }
    delay(50);
  }
  lcd.clear();
}

void resetLoadSettings(){
  lcd.clear(); 
  lcd.setCursor(5,0); lcd.print("1.Reset"); 
  lcd.setCursor(0,1); lcd.print("2.Load"); 
  lcd.setCursor(10,1); lcd.print("3.Exit");

  bool done=false;
  while(!done){
    if(IrReceiver.decode()){
      unsigned long code=IrReceiver.decodedIRData.decodedRawData;
      if(code==0xFFFFFFFF) code=lastCode; else lastCode=code;
      switch(code){
        case 0xE916FF00: done=true; resetSettings(); break;  // Reset
        case 0xE619FF00: done=true; loadSettings(); break;   // Load
        case 0xF20DFF00: done=true; lcd.clear(); return;     // Exit
      }
      IrReceiver.resume();
    }
  }
  lcd.clear();
}

void setLcdTimeOff(){
  lcd.clear(); lcd.setCursor(0,0); lcd.print("Temps Veille :");
  bool done=false; int lastDisplayed=-1;
  while(!done){
    if(IrReceiver.decode()){
      unsigned long code=IrReceiver.decodedIRData.decodedRawData;
      if(code==0xFFFFFFFF) code=lastCode; else lastCode=code;
      switch(code){
        case 0xB946FF00: lcdTimeTurnOff = lcdTimeTurnOff + 1000; break;
        case 0xEA15FF00: lcdTimeTurnOff = lcdTimeTurnOff - 1000; break;
        case 0xBF40FF00: done=true; break;}
      lcdTimeTurnOff = constrain(lcdTimeTurnOff, 10000UL, 120000UL);
      if(lcdTimeTurnOff!=lastDisplayed){ lcd.setCursor(7,1); lcd.print("    "); lcd.setCursor(7,1); lcd.print(lcdTimeTurnOff/1000); lastDisplayed=lcdTimeTurnOff; }
      IrReceiver.resume();
    }
    delay(50);
  }
  lcd.clear();
}

void setVitesses() {
  bool done = false;
  int vitesseIndex = 1;   // par défaut on commence sur V1

  while (!done) {
    if (IrReceiver.decode()) {
      unsigned long code = IrReceiver.decodedIRData.decodedRawData;
      if (code == 0xFFFFFFFF) code = lastCode;
      else lastCode = code;

      switch (code) {
        // Sélection de la vitesse active
        case 0xE916FF00: vitesseIndex = 1; break; // touche "1"
        case 0xE619FF00: vitesseIndex = 2; break; // touche "2"
        case 0xF20DFF00: vitesseIndex = 3; break; // touche "3"

        // Ajustements
        case 0xB946FF00:
          if (vitesseIndex == 1) speedValue1 += 5;
          if (vitesseIndex == 2) speedValue2 += 5;
          if (vitesseIndex == 3) speedValue3 += 5;
          break;

        case 0xEA15FF00:
          if (vitesseIndex == 1) speedValue1 -= 5;
          if (vitesseIndex == 2) speedValue2 -= 5;
          if (vitesseIndex == 3) speedValue3 -= 5;
          break;

        // Quitter le menu
        case 0xBF40FF00:
          done = true;
          break;
      }

      // Contraintes
      speedValue1 = constrain(speedValue1, 150, speedValue2);
      speedValue2 = constrain(speedValue2, speedValue1, speedValue3);
      speedValue3 = constrain(speedValue3, speedValue2, 255);

      // Rafraichisement de l'affichage
      lcd.clear();
      // Ligne 1 : V1 et V2
      lcd.setCursor(0, 0);
      lcd.print((vitesseIndex == 1) ? ">" : " ");
      lcd.print("V1:"); lcd.print(speedValue1);
      lcd.print("  ");
      lcd.print((vitesseIndex == 2) ? ">" : " ");
      lcd.print("V2:"); lcd.print(speedValue2);

      // Ligne 2 : V3
      lcd.setCursor(4, 1);
      lcd.print((vitesseIndex == 3) ? ">" : " ");
      lcd.print("V3:"); lcd.print(speedValue3);

      IrReceiver.resume();
    }
    delay(50);
  }

  lcd.clear();
}

void menuParams(){
  int menuIndex=0; const int menuItems=6; bool inMenu=true; int lastMenuIndex=-1;
  while(inMenu){
    if(menuIndex!=lastMenuIndex){
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Menu Params:"); lcd.setCursor(0,1);
      switch(menuIndex){
        case 0: lcd.print("->Stop Distance"); break;
        case 1: lcd.print("->Klaxon"); break;
        case 2: lcd.print("->Temps veille"); break;
        case 3: lcd.print("->Vitesses"); break;
        case 4: lcd.print("->Changer Code"); break;
        case 5: lcd.print("->Reset/Load"); break;}
      lastMenuIndex=menuIndex;
    }
    if(IrReceiver.decode()){
      unsigned long code=IrReceiver.decodedIRData.decodedRawData;
      if(code==0xFFFFFFFF) code=lastCode; else lastCode=code;
      switch(code){
        case 0xB946FF00: menuIndex=(menuIndex-1+menuItems)%menuItems; break;
        case 0xEA15FF00: menuIndex=(menuIndex+1)%menuItems; break;
        case 0xBC43FF00:
          switch(menuIndex){ 
          case 0: setStopDistance(); break;
          case 1: setKlaxonStyle(); break;
          case 2: setLcdTimeOff(); break;
          case 3: setVitesses(); break;
          case 4: checkCode("Entrer code:", tankCode, 2);
            lcd.clear();
            tankCode = getIrCode("Changer code:"); 
            EEPROM.put(ADDR_CODE,tankCode);
            lcd.clear(); lcd.setCursor(0,0); lcd.print("Code sauvegarde!");
            delay(1500); lcd.clear(); break; 
          case 5: resetLoadSettings(); break;}
          lastMenuIndex=-1; break;
        case 0xBB44FF00: inMenu=false; lcd.clear(); break;
      }
      IrReceiver.resume();
    }
    delay(100);
  }
}

// --- Pilote Auto ---
void piloteAuto(){
  autoSpeedEnabled=true;
  int distanceFront=measureDistanceCm();
  if(distanceFront>STOP_DISTANCE_CM) Avant();
  else{
    Stop(); delay(100);
    Gauche(); delay(500);
    if(measureDistanceCm()>STOP_DISTANCE_CM) Avant(); 
    else{ 
      Droite();
      delay(1000); Stop();
    }
  }
}

// --- Setup ---
void setup(){
  Serial.begin(9600);
  IrReceiver.begin(RECV_PIN,DISABLE_LED_FEEDBACK);
  loadSettings();

  pinMode(RightMotorA,OUTPUT); pinMode(RightMotorB,OUTPUT);
  pinMode(LeftMotorA,OUTPUT); pinMode(LeftMotorB,OUTPUT);
  pinMode(LeftMotorEnable,OUTPUT); pinMode(RightMotorEnable,OUTPUT);
  pinMode(trigPin,OUTPUT); pinMode(echoPin,INPUT);
  pinMode(buzzerPin,OUTPUT); digitalWrite(buzzerPin,LOW);

  Stop(); setSpeed(speedState);

  lcd.init();
  lcd.backlight();
  lcdIsOn = true;
  lastIrActivity = millis();
  initLockChars(); // charger les icônes 🔒 🔓 au démarrage
  lcd.setCursor(1,0); lcd.print("Conqueror Tank");
  lcd.setCursor(3,1); lcd.print("By Souleil");
  delay(1000); lcd.clear();

  bool lock;
  EEPROM.get(ADDR_LOCK, lock);
  if(lock==true) maxTentative(30);  // si lock==true → 30, sinon 0

  checkCode("Entrer code :", tankCode, 3);
  lcd.clear();
}

// --- Loop ---
void loop() {
  unsigned long now = millis();

  // --- Mesure périodique de la distance ---
  if(now - lastPingTime >= pingInterval){ 
    lastPingTime = now; 
    lastDistanceCm = measureDistanceCm();
  }

  // --- Radar BIP ---
  if(safetyMode == 1) distanceBeeper();

  // --- Extinction LCD après inactivité ---
  if (lcdIsOn && (now - lastIrActivity > lcdTimeTurnOff)) {
    lcd.noBacklight();
    lcdIsOn = false;
  }

  // --- Lecture télécommande IR ---
  if(IrReceiver.decode()){
    unsigned long code = IrReceiver.decodedIRData.decodedRawData;
    if(code == 0xFFFFFFFF) code = lastCode; 
    else lastCode = code;

    lastSignalTime = now;   // gestion STOP
    lastIrActivity = now;   // gestion LCD
    if (!lcdIsOn) {
      lcd.backlight();
      lcdIsOn = true;
      updateLCD(); // mise à jour directe
    }

    // --- Commandes IR ---
    switch(code){
      case 0xB946FF00: currentMove = FORWARD; break;
      case 0xEA15FF00: currentMove = BACKWARD; break;
      case 0xBB44FF00: currentMove = LEFT; break;
      case 0xBC43FF00: currentMove = RIGHT; break;
      case 0xE718FF00: Frein(); break;
      case 0xBF40FF00: Klaxon(); break;
      case 0xE916FF00: setSpeed(1); autoSpeedEnabled = false; break;
      case 0xE619FF00: setSpeed(2); autoSpeedEnabled = false; break;
      case 0xF20DFF00: setSpeed(3); autoSpeedEnabled = false; break;
      case 0xF30CFF00: safetyMode = (safetyMode + 1) % 3; break;
      case 0xA15EFF00: autoSpeedEnabled = !autoSpeedEnabled; break;
      case 0xF708FF00: autoModeEnabled = !autoModeEnabled; autoSpeedEnabled = autoModeEnabled; break;
      case 0xAD52FF00: menuParams(); break;
      case 0xB54AFF00: saveSettings(); break;
      case 0xBD42FF00: rebootArduino(); break;
    }
    IrReceiver.resume();
  }

  // --- Stop si aucune commande IR depuis timeout ---
  if(millis() - lastSignalTime > timeout) currentMove = STOPPED;

  // --- Mode Auto ou Manuel ---
  if(autoModeEnabled) piloteAuto();
  else {
    switch(currentMove){
      case FORWARD: if(safetyMode == 0 && lastDistanceCm <= STOP_DISTANCE_CM) Stop(); else Avant(); break;
      case BACKWARD: Arriere(); break;
      case LEFT: Gauche(); break;
      case RIGHT: Droite(); break;
      case STOPPED: Stop(); break;
    }
  }

  // --- Ajustement automatique de la vitesse ---
  autoAdjustSpeed();

  // --- Mise à jour LCD seulement si allumé ---
  if(lcdIsOn && (now - lastLCDupdate >= lcdInterval)){
    updateLCD(); 
    lastLCDupdate = now;
  }
}