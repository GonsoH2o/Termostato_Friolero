#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_BMP280.h>
#include <WifiClient.h>
#include <DHT.h>


#define MAX_SRV_CLIENTS 1
#define MAX_TIME_INACTIVE 60000000

// Set WiFi credentials
#define WIFI_SSID "Torre_Mesh"
#define WIFI_PASS "Q8REWQLB7HRFE6"

// Definir Sensores
#define SEALEVELPRESSURE_HPA (1036.6)
#define DHTPIN D6 // define en que puerto va conectado el DHT
#define DHTTYPE DHT11 // modelo de DHT (DHT11 o DHT22)

// Declarar Sensores
Adafruit_BMP280 bmp; //I2C
DHT dht(DHTPIN, DHTTYPE);

// Declarar Variables 
float tempC, tempF, hum, pres, alt = -99; // puede que no sirva, revisar y eliminar más tarde
float temperatura, humedad, presion, altitud;

// Create a new web server
ESP8266WebServer webserver(80);

unsigned long ElapsedTime = 0;

// Configuración conexion a Telnet APRS server

/* Programa para convertir coordenadas: https://www.qsl.net/on6mu/tinylocator.htm  */

const String USER    = "EA4HVF"; //write your aprs callsign
const String PAS     = "21677"; // write your aprs password
const String LAT     = "4034.75N"; //write your latitude
const String LON     = "00357.39W"; //write your longitute
const String COMMENT = "EA4HVF Torrelodones WX Weather Station (g)"; //write some comment
//const String SERVER  = "spain.d2g.com";
const String SERVER  = "Spain.aprs2.net"; // write the address of the aprs server
const int    PORT    = 14580; //write the aprs server port
// Fin configuracion TELNET

// Handle Root
void rootPage() { 
  webserver.send(200, "text/plain", "Torrelodones WX Station !"); 
}

// Handle 404
void notfoundPage(){ 
  webserver.send(404, "text/plain", "404: Nada por aquí..."); 
}

void setup()
{
  // Setup serial port
  Serial.begin(115200);
  Serial.println();
  bmp.begin(0x76);
  dht.begin();
  

  delay(10);

  //Begin WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(100); }

  // WiFi Connected
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // Start Web Server
  webserver.on("/", rootPage);
  webserver.onNotFound(notfoundPage);
  webserver.begin();

}

// Listen for HTTP requests
void loop(void){ 
  webserver.handleClient(); 



  sendAPRSPacketEvery(480000); //run every 8 minutes
  // Give a time for ESP

    yield();
}

void handle_0nConnect(){
  temperatura = bmp.readTemperature();
  humedad = dht.readHumidity();
  altitud = bmp.readAltitude(SEALEVELPRESSURE_HPA);
  presion = bmp.readPressure()/100;
}

void getDataFromBMP(){
  int count = 0;
  tempC = bmp.readTemperature();
  tempF = ((tempC *9)/5 +32);
  hum = dht.readHumidity();
  pres = bmp.readPressure();
  alt = bmp.readAltitude();
  Serial.println("Read tempC="+String(tempC)+" tempF="+String(tempF)+" hum="+String(hum)+" pres="+String(pres)+" alt="+String(alt));
  while ((isnan(tempC) || isnan(tempF) || isnan(pres) || isnan(hum) || isnan(alt) || hum == 0) && count < 1000){
  //  Debug.println("Read (isnan(tempC) || isnan(tempF) || isnan(pres) || isnan(hum) || isnan(alt) || hum == 0) && count < 1000");
  tempC = bmp.readTemperature();
  tempF = ((tempC *9)/5 +32);
  hum = dht.readHumidity();
  pres = bmp.readPressure();
  alt = bmp.readAltitude();
    Serial.println("Trying read again tempC="+String(tempC)+" tempF="+String(tempF)+" hum="+String(hum)+" pres="+String(pres)+" alt="+String(alt)+" count="+String(count));
    
    delay(2);
    count++;
  }
}
String getTemp(float pTemp){
  String strTemp;
  int intTemp;

  intTemp = (int)pTemp;
  strTemp = String(intTemp);
  //strTemp.replace(".", "");
  
  switch (strTemp.length()){
  case 1:
    return "00" + strTemp;
    break;
  case 2:
    return "0" + strTemp;
    break;
  case 3:
    return strTemp;
    break;
  default:
    return "-999";
  }

}

String getHum(float pHum){
  String strHum;
  int intHum;
 
  intHum = (int)pHum;
  strHum = String(intHum);
  
  switch (strHum.length()){
  case 1:
    return "0" + strHum;
    break;
  case 2:
    return strHum;
    break;
  case 3:
    if (intHum == 100){
       return "00";
    }else {
       return "-99";
    }
    break;
  default:
    return "-99";
  }
}

String getPres(float pPress){
  String strPress;
  int intPress = 0;
  intPress = (int)(pPress/10);
  strPress = String(intPress);
  
  
  //strPress.replace(".", "");
  ///strPress = strPress.substring(0,5);
  //return strPress;
  
  switch (strPress.length()){
  case 1:
    return "0000" + strPress;
    break;
  case 2:
    return "000" + strPress;
    break;
  case 3:
    return "00" + strPress;
    break;
  case 4:
    return "0" + strPress;
    break;
  case 5:
    return strPress;
    break;
  default:
    return "-99999";
  }
}

void sendAPRSPacketEvery(unsigned long t){
  unsigned long currentTime;
  currentTime = millis();
  if (currentTime < ElapsedTime){
    Serial.println("Run: (currentTime = millis()) < ElapsedTime.\ncurrentTime ="+String(currentTime)+"\nElapsedTime="+String(ElapsedTime));
    ElapsedTime = 0;    
    Serial.println("Set ElapsedTime=0");
  }
  if ((currentTime - ElapsedTime) >= t){
    Serial.println("Tried : (currentTime - ElapsedTime) >= t.\ncurrentTime ="+String(currentTime)+"\nElapsedTime="+String(ElapsedTime));
    clientConnectTelNet();
    ElapsedTime = currentTime;  
    Serial.println("Set ElapsedTime = currentTime");
  }
}

void clientConnectTelNet(){
  WiFiClient client;
  int count = 0;
  String packet, aprsauth, tempStr, humStr, presStr;
  Serial.println("Run clientConnectTelNet()");
  getDataFromBMP();
  while (!client.connect(SERVER.c_str(), PORT) && count <20){
    Serial.println("Didn't connect with server: "+String(SERVER)+" Port: "+String(PORT));
    delay (1000);
    client.stop();
    client.flush();
    Serial.println("Run client.stop");
    Serial.println("Trying to connect with server: "+String(SERVER)+" Port: "+String(PORT));
    count++;
    Serial.println("Try: "+String(count));
  }
  if (count == 20){
    Serial.println("Tried: "+String(count)+" don't send the packet!");
  }else{
    Serial.println("Connected with server: "+String(SERVER)+" Port: "+String(PORT));
    tempStr = getTemp(tempF);
    humStr = getHum(hum);
    presStr = getPres(pres);
    Serial.println("Leu tempStr="+tempStr+" humStr="+humStr+" presStr="+presStr);
    while (client.connected()){ //there is some problem with the original code from WiFiClient.cpp on procedure uint8_t WiFiClient::connected()
      // it don't check if the connection was close, so you need to locate and change the line below:
      //if (!_client ) to: 
      //if (!_client || _client->state() == CLOSED)
      delay(1000);
      Serial.println("Run client.connected()");
      if(tempStr != "-999" || presStr != "-99999" || humStr != "-99"){
          aprsauth = "user " + USER + " pass " + PAS + "\n";
          client.write(aprsauth.c_str());
          delay(500);
          Serial.println("Send client.write="+aprsauth);
                  
          packet = USER + ">APRMCU,TCPIP*,qAC,T2SPAIN:=" + LAT + "/" + LON +
               "_.../...g...t" + tempStr +
               "r...p...P...h" + humStr +
               "b" + presStr + COMMENT + "\n";
        
          client.write(packet.c_str());
          delay(500);
          Serial.println("Send client.write="+packet);
       
          client.stop();
          client.flush();
          Serial.println("Telnet client disconnect ");
      }
    }
  }
  
}