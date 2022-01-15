/***************************************************
 Code tiré de l'exemple permettant l'enrigstrement d'empreintes issu de la bibliothèque Adrafruit,
 modifié pour répondre à nos besoins en fonction de la carte et des ports notamment. 

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include <Adafruit_Fingerprint.h>


#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
SoftwareSerial mySerial(18, 19); // on se connecte aux ports TX et RX du capteur

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1

#endif


Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial); 

uint8_t id;

void setup() //début de l'initialisation du code
{
  Serial.begin(9600);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit Fingerprint sensor enrollment");

  // set the data rate for the sensor serial port
  finger.begin(57600);

  if (finger.verifyPassword()) { 
    Serial.println("Un capteur a été trouvé !");  //vérification de l'existence ou non du capteur d'empreinte digitale
  } else {
    Serial.println("Aucun capteur n'a été détecté");
    while (1) { delay(1); } //va faire pause indéfiniment 
  }

  Serial.println(F("lecture des paramètres du capteur"));
  finger.getParameters(); //on récupère les différents paramètres intrinsèques au capteur
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop()                     // se lance continuellement 
{
  Serial.println("Prêt à enregistrer une empreinte digitale !");
  Serial.println("Entrez l'ID # (de 1 à 127) que vous voulez associer à l'empreinte...");
  id = readnumber(); //on associe la variable id au chiffre rentré par l'utilisateur
  if (id == 0) {// l'ID 0 est impossible
     return;
  }
  Serial.print("Enregistrement de l'ID #");
  Serial.println(id);

  while (!  getFingerprintEnroll() ); // on lance la fonction d'enregistrement de l'empreinte digitale
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Attente d'un doigt valide à enregistrer #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage(); // on récupère l'image enregistrée de notre empreinte
    switch (p) { //différents cas 
    case FINGERPRINT_OK:
      Serial.println("Image taken"); //ça marche
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("."); // il n'y a pas de doigt, on attend
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error"); // erreur en réception de l'image
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error"); //erreur dans la prise de l'image
      break;
    default:
      Serial.println("Unknown error"); // toutes les autres erreurs possibles
      break;
    }
  }

  // OK success! 

  p = finger.image2Tz(1);
  switch (p) { // on convertit ici l'image pour pouvoir la stocker dans la mémoire flash de l'empreinte digitale, avec une nouvelle fois les différents problèmes possibles
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger"); // tout a marché, on enlève le doigt
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again"); //pour avoir plus de sûreté, on demande à remettre le doigt afin de valider la saisie de l'empreinte, même chose qu'avant :
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id); //la deuxième image du même doigt est convertie

  p = finger.createModel();
  if (p == FINGERPRINT_OK) { // vérification de la similitude des deux images, on veut être sûr qu'elles appartiennent au même doigt
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id); //on stocke les données liées à l'ID dans la mémoire flash, en vérifiant une nouvelle fois qu'il n'y a pas de problèmes.
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}
