/*  VNH2SP30 pin definitions
 xxx[0] controls '1' outputs
 xxx[1] controls '2' outputs */
int pwmpin[4] = { 5, 6, 9, 10 };    // PWM input
//int inApin[4] = { 4, 7, 8, 11 };    // NEW INA: Clockwise input
int inApin[4] = { 7, 4, 11, 8 };    // OLD INA: Clockwise input 

int pwmValues[4] = { 0, 0, 0, 0 };  // Stocke les valeurs PWM
int previousPwmValues[4] = {0, 0, 0, 0};

String inputString = "";
bool stringComplete = false;

const String DEVICE_ID = "Klax_5"; // Changez cette valeur pour chaque Arduino

#include "WiFiS3.h"
//#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include "Frames.h"
#include "WiFiCredentials.h"
#include <WiFiUdp.h>
#include <OSCMessage.h>

// Serveur UDP pour OSC
WiFiUDP Udp;
const int localPort = 8000;  // Port local pour recevoir les messages OSC
const int remotePort = 8001;  // Port local pour recevoir les messages OSC

// Initialisation de la matrice LED
ArduinoLEDMatrix LEDMatrix;

byte frame[8][12] = {0}; // Structure 8x12 pour représenter la matrice LED

int status = WL_IDLE_STATUS;
WiFiServer server(80);

// Variables globales pour le clignotement de la LED
unsigned long previousMillis = 0; // Temps de la dernière mise à jour de la LED
bool ledState = LOW;              // État actuel de la LED

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;  // Attendre la connexion au port série
  }

  Serial.println("KLAX Starting...");
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);  // Configurer la LED intégrée comme sortie
  //analogReadResolution(12);

  // Initialiser les broches numériques comme sorties
  for (int i = 0; i < 4; i++) {
    pinMode(inApin[i], OUTPUT);
    pinMode(pwmpin[i], OUTPUT);
  }

  // Initialiser les moteurs à l'arrêt
  for (int i = 0; i < 4; i++) {
    digitalWrite(inApin[i], LOW);
  }

  // Vérifier le module WiFi
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication avec le module WiFi échouée !");
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Veuillez mettre à jour le firmware");
  }

  // Connexion au WiFi
  connectToWiFi();

  Serial.print("Identifiant unique : ");
  Serial.println(DEVICE_ID);

  Udp.begin(localPort);
  
  LEDMatrix.begin();

  Serial.println("KLAX READY!");
  Serial.println();
}

void test() {
  int MIN = 50;
  int MAX = 220;
  int i = MIN;
  while (i < MAX) {
    motorGo(0, i);
    delay(20);
    i = i + 1;
  }
  while (i >= MIN) {
    motorGo(0, i);
    delay(20);
    i = i - 1;
  }

  motorOff(0);

  delay(200);
  motorGo(1, 100);
  delay(300);
  motorOff(1);

  delay(200);
  motorGo(2, 100);
  delay(300);
  motorOff(2);

  delay(300);

  i = 0;
  while (i < 100) {
    motorGo(3, i);
    motorGo(2, i);
    delay(10);
    i = i + 1;
  }
  while (i >= 0) {
    motorGo(3, i);
    motorGo(2, i);

    delay(10);
    i = i - 1;
  }
  motorOff(3);
  motorOff(2);

  //motorGo(0, 1023);
  delay(5000);
}

void loop() {
  static unsigned long lastWifiCheckMillis = 0;  // Temps de la dernière vérification WiFi
  unsigned long currentMillis = millis();

  // Vérifier si le WiFi est toujours connecté
  bool isConnected = (WiFi.status() == WL_CONNECTED || WL_SCAN_COMPLETED);
  /*Serial.print("Wifi ");
  Serial.print(isConnected);
  Serial.print(" status ");
  Serial.println(WiFi.status());*/

  // Gérer le clignotement de la LED
  handleBlinkLED(isConnected);

  // Vérifier la connexion WiFi toutes les 30 secondes
  if (currentMillis - lastWifiCheckMillis >= 30000) {
    lastWifiCheckMillis = currentMillis;  // Mettre à jour le temps de la dernière vérification

    if (!isConnected) {
      Serial.println("Connexion WiFi perdue. Tentative de reconnexion...");
      connectToWiFi();
    }
  }

  receiveOSC();

  // Mettre à jour l'affichage des moteurs sur la matrice LED
  updateMotorDisplay();

}

void connectToWiFi() {
  Serial.println("Scan des réseaux WiFi...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("Aucun réseau trouvé !");
    while (true) {
      handleBlinkLED(false);  // Clignotement rapide (200 ms)
    }
  } else {
    Serial.println("Réseaux détectés :");
    for (int j = 0; j < n; j++) {
      Serial.print("   ");
      Serial.println(WiFi.SSID(j));
    }
  }

  // Essayer de se connecter à un réseau connu
  bool connected = false;
  for (int i = 0; i < networkCount; i++) {
    for (int j = 0; j < n; j++) {
      String test_ssid = WiFi.SSID(j);

      // Comparaison directe avec ==
      if (test_ssid == networks[i][0]) {
        Serial.print("Tentative de connexion au réseau : ");
        Serial.println(networks[i][0]);
        
        status = WiFi.begin(networks[i][0].c_str(), networks[i][1].c_str());

        // Attendre la connexion avec clignotement rapide
        int attempts = 10;
        while (status != WL_CONNECTED && attempts > 0) {
          handleBlinkLED(false);  // Clignotement rapide (200 ms)
          status = WiFi.status();
          attempts--;
        }

        if (status == WL_CONNECTED) {
          connected = true;
          break;
        }
      }
    }
    if (connected) break;
  }

  if (connected) {
    //Serial.println("Connecté au réseau !");
    printCurrentNet();
  } else {
    Serial.println("Impossible de se connecter à un réseau connu.");
  }
}

void receiveOSC() {
  int packetSize = Udp.parsePacket();  // Vérifier si un paquet UDP est disponible
  if (packetSize > 0) {
    OSCMessage msg;  // Créer un objet OSCMessage
    while (packetSize--) {
      msg.fill(Udp.read());  // Remplir le message avec les données reçues
    }

    if (!msg.hasError()) {
      // Traiter le message OSC
      processOSC(msg);
    } else {
      Serial.println("Erreur dans le message OSC");
    }
  }
}

/*
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}*/

void processOSC(OSCMessage &msg) {
  if (msg.fullMatch("/motorAll")) {  // Si le message correspond à "/motorAll"
    for (int motor = 0; motor < 4; motor++) {
      int pwm = msg.getInt(motor);  // Récupérer la valeur PWM pour chaque moteur
      if (pwm >= 0 && pwm <= 255) {
        pwmValues[motor] = pwm;

        if (pwm == 0) {
          motorOff(motor);  // Éteindre le moteur si PWM = 0
        } else {
          motorGo(motor, pwm);  // Appliquer la valeur PWM au moteur
        }
      }
    }
    updateMotorDisplay();  // Mettre à jour l'affichage des moteurs
  /*} else if (msg.fullMatch("/motor")) {  // Si le message correspond à "/motor"
    int motor = msg.getInt(0);    // Récupérer le premier argument (numéro du moteur)
    int pwm = msg.getInt(1);      // Récupérer le deuxième argument (valeur PWM)

    if (motor > 0 && motor <= 4 && pwm >= 0 && pwm <= 255) {
      pwmValues[motor - 1] = pwm;

      if (pwm == 0) {
        motorOff(motor - 1);
      } else {
        motorGo(motor - 1, pwm);
      }

      updateMotorDisplay();  // Mettre à jour l'affichage des moteurs
    }*/
  } else if (msg.fullMatch("/getIP")) { // Si le message correspond à "/getIP"
    sendIPAddress(msg); // Appeler la fonction pour envoyer l'adresse IP
  } else {
    Serial.println("Message OSC inconnu");
  }
}

void sendIPAddress(OSCMessage &msg) {
  IPAddress ip = WiFi.localIP(); // Obtenir l'adresse IP locale
  char ipString[16];
  snprintf(ipString, sizeof(ipString), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  // Envoyer la réponse OSC
  OSCMessage response("/ip");
  // Serial.print("RemoteIP: ");
  // Serial.print(Udp.remoteIP());
  // Serial.print(" | Port: ");
  // Serial.print(remotePort);
  // Serial.print(" | ID: ");
  // Serial.print(DEVICE_ID);
  // Serial.print(" | IP: ");
  // Serial.println(ipString);
  response.add(DEVICE_ID.c_str()); // Ajouter l'identifiant unique
  response.add(ipString); // Ajouter l'adresse IP comme argument
  Udp.beginPacket(Udp.remoteIP(), remotePort);
  response.send(Udp); // Envoyer le message OSC
  Udp.endPacket();
  response.empty(); // Libérer la mémoire utilisée par le message
}

/*
void processCommand(String command) {
  int motor, msb, lsb, pwm;

  if (sscanf(command.c_str(), "%d %d %d", &motor, &msb, &lsb) == 3) {
    pwm = 255 * msb + lsb;
    Serial.print(motor);
    Serial.print(" ");
    Serial.println(pwm);
    if (motor > 0 && motor <= 4 && pwm >= 0 && pwm <= 1023) {
      pwmValues[motor - 1] = pwm;

      if (pwm == 0) {
        motorOff(motor - 1);
      } else {
        motorGo(motor - 1, pwm);
      }

      updateMotorDisplay();  // Met à jour l'affichage des moteurs

      // Affiche les valeurs PWM appliquées
      //Serial.print(pwmValues[0]);
      //Serial.print(" ");
      //Serial.print(pwmValues[1]);
      //Serial.print(" ");
      //Serial.print(pwmValues[2]);
      //Serial.println(pwmValues[3]);
    }
  } else {
    Serial.print("error ");
    Serial.println(command);
  }
}*/

void motorOff(int motor) {
  digitalWrite(inApin[motor], LOW);
  analogWrite(pwmpin[motor], 0);
}

void motorGo(uint8_t motor, uint8_t pwm) {
  digitalWrite(inApin[motor], HIGH);
  analogWrite(pwmpin[motor], pwm);
}

void printCurrentNet() {
  Serial.print("Connecté | SSID: ");
  Serial.print(WiFi.SSID());
  Serial.print(" (RSSI): ");
  Serial.print(WiFi.RSSI());
  //Serial.print(" Enc:");
  //Serial.print(WiFi.encryptionType(), HEX);
  Serial.print(" IP: ");
  Serial.println(WiFi.localIP());
  //Serial.println();
}

void handleBlinkLED(bool isConnected) {
  // Définir l'intervalle en fonction de l'état de connexion
  unsigned long interval = isConnected ? 2000 : 200;  // Lent (1s) si connecté, rapide (200ms) sinon

  unsigned long currentMillis = millis();

  // Vérifier si l'intervalle est écoulé
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;       // Mettre à jour le temps précédent
    ledState = !ledState;                 // Inverser l'état de la LED
    digitalWrite(LED_BUILTIN, ledState);  // Mettre à jour la LED
  }
}

void updateMotorDisplay() {
  bool needsUpdate = false;

  // Vérifier si les valeurs PWM ont changé
  for (int motor = 0; motor < 4; motor++) {
    if (pwmValues[motor] != previousPwmValues[motor]) {
      needsUpdate = true;
      previousPwmValues[motor] = pwmValues[motor]; // Mettre à jour les valeurs précédentes
    }
  }

  if (!needsUpdate) {
    return; // Ne rien faire si aucune mise à jour n'est nécessaire
  }

  // Réinitialiser la frame
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 12; x++) {
      frame[y][x] = 0; // Éteindre tous les pixels
    }
  }

  // Mettre à jour la frame en fonction des valeurs PWM des moteurs
  for (int motor = 0; motor < 4; motor++) {
    int level = map(pwmValues[motor], 0, 255, 0, 12); // Mapper PWM (0-255) sur 12 niveaux
    for (int y = 0; y < level; y++) {
      frame[7-(motor * 2)][11-y] = 1; // Allumer les pixels correspondants
      frame[7-(motor * 2 + 1)][11-y] = 1; // Allumer les pixels correspondants
    }
  }

  // Afficher la frame sur la matrice LED
  LEDMatrix.renderBitmap(frame, 8, 12);
}
