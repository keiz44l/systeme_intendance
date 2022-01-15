/*code permettant le recueil des données biométrique des élèves à leur entrée en cours et leur envoi automatique à la base de données pour annoter leur présence*/



#include <Adafruit_Fingerprint.h> //importation des librairies voulues
#include <Ethernet.h>
#include<SPI.h>
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x86, 0xD6 }; //adresse MAC unique issue de l'ethernet shield que nous avons commandé
char server[] = "api.pushingbox.com";   //Le serveur auquel on veut se connecter pour l'envoi des requêtes
IPAddress ip(192,168,138,177); //on associe une adresse IP non existante au préalable à notre connexion ethernet issu du shield
IPAddress subnet(255,255,255,255); //on associe également un masque de sous réseau classique dans le cas où la connection le requiert
IPAddress gateway(192, 168, 0, 1); //passerelle permettant de relier le réseau local et le réseau internet 

EthernetClient client; //on initialise le client ethernet, nous en l'occurence
int allowed = 0; //initialisation d'une variable permettant de dire si l'empreinte est existante dans la base de données ou non

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)

SoftwareSerial mySerial(18, 19); //on connecte une nouvelle fois aux ports de notre arduino 

#else
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup()
{
  Serial.begin(9600);
  while (!Serial) {
    ; // attend la connexion des ports ( seulement pour les USB natifs
  }

  // On commence la connexion ethernet
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP"); // si la connexon avec le protocole DHCP ( qui fournit automatiquement un hôte IP, une passerelle et un masque de sous réseau ) échoue, 
    // on vérifie que tout est bien connecté et qu'ucun problème physique existe
    if (Ethernet.hardwareStatus() == EthernetNoHardware) { //si l'ethernet shield n'est pas detecté on attend indéfiniment, on ne peut rien faire
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true) {
        delay(1); 
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected."); //on notifie si le câble ethernet n'est pas branché
    }
    // on essaie de se connecter manuellement avec les informations rentrées au début si le DHCP n'a pas fonctionné
    Ethernet.begin(mac, ip, subnet, gateway);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // on attend une seconde avant de se connecter au serveur
  delay(1000);
  Serial.print("connecting to ");
  Serial.print(server);
  Serial.println("...");

  // on vérifie que tout est branché est fonctionne pour le capteur d'empreinte digitale
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters")); //on lit les paramètres du capteur
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount(); //on lance la fonction qui permet de retrouver le nombre d'IDs présents dans la mémoire flash

  if (finger.templateCount == 0) { //si il y en a aucun, on demande à lancer la fonction d'enregistrement d'empreintes.
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
}

void loop()                     // se lance continuellement 
{
  getFingerprintID(); // on lance la fonction de récupération des empreintes
  delay(50);  
  }
  


 void Sending_To_spreadsheet()   //Fonction qui fait le lien entre le serveur qui envoie la requête à notre google sheet et l'ID de l'élève
 {
   if (client.connect(server, 80)) {
    Serial.println("connected");
    // Make a HTTP request:
    client.print("GET /pushingbox?devid=vC58DFEFDAA5D25B&allowed_members=");     //La requête avec notre url qui va se positionner à la fin de l'url de notre serveur
    if(allowed == 1) //si l'empreinte existe bien
    {
      client.print('1'); //on rajoute 1 à allowed_members, l'élève existe bien, on s'intéresse à lui pour la base de données
//    Serial.print('1');
    }
    else
    {
      client.print('0');
    }
    
    client.print("&Member_ID="); //on rajoute le query string associé à l'ID
    client.print(finger.fingerID);
                                  
         
    client.print(" ");      //SPACE BEFORE HTTP/1.1
    client.print("HTTP/1.1");
    client.println();
    client.println("Host: api.pushingbox.com");
    client.println("Connection: close");
    client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
 }

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage(); // de la même manière qu'enroll, on cherche à prendre l'image associée à notre empreint et à la convertir après si aucun problème n'est survenu 
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
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
  p = finger.fingerSearch(); //l'image est convertie, on cherche maintenant si elle apparaît déjà dans la mémoire flash
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); //on a trouvé, on indique l'ID associé dans la mémoire
  allowed = 1;
  Serial.print(" with confidence of "); Serial.println(finger.confidence); //on indique ensuite un taux de confiance sur la ressemblance entre les données issues de l'enregistrement et du test que nous venons de faire
  Sending_To_spreadsheet(); // on appelle la fonction qui fait le lien avec le google sheet 
  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() { //fonction qui permet q'envoyer l'id associé si il existe
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  return finger.fingerID;
}
