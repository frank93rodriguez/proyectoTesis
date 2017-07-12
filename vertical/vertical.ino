
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
// Define NodeMCU D3 pin  connect to LED
#define LED_PIN0 5
const int AnalogIn  = A0;
int BUTTON_PIN = 12; 
int HIGRO_PIN= 14;
unsigned long readTime;
int readingIn = 0; ////variable para lectura de higrometro
int humedad=0;
// Update these with values suitable for your network.
const char* ssid = "TazAtherton";
const char* password = "29605091";
const char* mqtt_server = "192.168.0.20";
//const char* mqtt_server = "iot.eclipse.org";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
bool pressed0=true;
bool recirculando=false;

void setup_wifi() {
  int intentos=0;
   delay(100);
  // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED && intentos<=10) 
    {
      delay(500);
      Serial.print(".");
      intentos++;
    }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Command from MQTT broker is : [");
  Serial.print(topic);
  int p =(char)payload[0]-'0';
  // if MQTT comes a 0 turn off LED on D2
  if(p==1) 
  {
     digitalWrite(LED_PIN0, HIGH);
     recirculando=true; 
    Serial.println(" Turn ON LED! " );
    //client.publish("casa4","hola");
  } 
  // if MQTT comes a 1, turn on LED on pin D2
  if(p==0)
  {
     digitalWrite(LED_PIN0, LOW);
     recirculando=false; 
    Serial.println(" Turn OFF LED! " );
    //readingIn = analogRead(AnalogIn);
    //client.publish("casa4",String(readingIn));
  }
  Serial.println();
} //end callback

void reconnect() {
  // Loop until we're reconnected
  int intentos=0;
  while (!client.connected()&&intentos<=2) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      client.subscribe("casaHR");
      //client.subscribe("casaHH");
      //client.subscribe("casaHH2");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(6000);
    }
    intentos++;
  }
} //end reconnect()

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(BUTTON_PIN,INPUT);
  pinMode(HIGRO_PIN,INPUT);
  pinMode(LED_PIN0, OUTPUT);
  digitalWrite(LED_PIN0, LOW);
  recirculando=false; 
}

void loop() {
  if (!client.connected()&&WiFi.localIP()!=0) {
    reconnect();
  }
  client.loop();
  long now = millis();
  int status0;
  if (now - lastMsg > 50) {
     lastMsg = now;
     status0=digitalRead(BUTTON_PIN);
     String msg="";
     //Serial.print("STATUS 0: " );
     //Serial.println(status0 );
     //Serial.print("estado rele: " );Serial.println(recirculando);
     //Serial.print("estado HIGRO: " );
     //Serial.println(digitalRead(HIGRO_PIN) );
     ///STATUS 0
     if(status0==HIGH){
      if(pressed0){
        if(recirculando){
           msg= msg+ "0";
           char message[58];
           msg.toCharArray(message,58);
           Serial.println(message);
           digitalWrite(LED_PIN0, LOW);
           recirculando=false; 
           Serial.println(" ILUMINACION OFF " );
           //publish sensor data to MQTT broker
           client.publish("casaHR", message);
        }else{
           msg= msg+ "1";
           char message[58];
           msg.toCharArray(message,58);
           Serial.println(message);
           digitalWrite(LED_PIN0, HIGH);
           recirculando=true; 
           Serial.println(" ILUMINACION ON " );
           //publish sensor data to MQTT broker
           client.publish("casaHR", message);
        }
        pressed0=false;
      }
   }
   else
   {
      pressed0=true;
   }
  }
     
  // /////////////////////////////////revisa si han pasado 5 segundos desde la ultima lectura al sensor
  if(millis() > readTime+10000){
     //////PRENDER  O APAGAR VALVULA////
     Serial.print("ESTADO RELE: ");Serial.println(recirculando);
     humedad = sensorRead();
      if(humedad>210){//debe regar
        digitalWrite(LED_PIN0, HIGH);
        recirculando=true;
        client.publish("casaHR","1"); 
        Serial.println("ESTA REGANDO!!");
      }
      else if(humedad<116){//no debe regar
        if(recirculando==true){
          digitalWrite(LED_PIN0, LOW);
          client.publish("casaHR","0");
        }
        recirculando=false;
        Serial.println("NO ESTA REGANDO!!");  
       }
      //////PRENDER  O APAGAR VALVULA////
  }
}

int sensorRead(){
  readTime = millis();
  readingIn = analogRead(AnalogIn);
  Serial.print("LECTURA HIGRO: ");Serial.println(readingIn);
  //Serial.print("HUMEDAD: ");Serial.println(readingIn);
  char buffer[10];
  dtostrf(readingIn,0, 0, buffer);
  client.publish("casaHH",buffer);
  if(digitalRead(HIGRO_PIN)==1){
    client.publish("casaHH2","SI");
  }else if(digitalRead(HIGRO_PIN)==0){
    client.publish("casaHH2","NO");
  }
  return readingIn;
  //client.publish("openhab/himitsu/humidity","mensaje de higrometro");
}
