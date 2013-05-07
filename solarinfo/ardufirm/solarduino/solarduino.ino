#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 42);
IPAddress gateway(192,168,0,1);	
IPAddress subnet(255, 255, 255, 0);
IPAddress destination(192, 168, 0, 72);
unsigned int localPort = 8888;      // local port to listen on
unsigned int destPort = 8888;
// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;


int relais_Pin = 9;      // select the pin for the LED
int sol_volt_Pin = A5;      // select the pin for the LED
int batt_volt_Pin = A4;

int sol_sampleVolt = 0;
int batt_sampleVolt = 0;

int batt_R1 = 11980; // Resistance of R1 in ohms
int batt_R2 = 5195; // Resistance of R2 in ohms
float batt_ratio = (float)batt_R1 / (float)batt_R2;

int sol_R1 = 11980; // Resistance of R1 in ohms
int sol_R2 = 1875; // Resistance of R2 in ohms
float sol_ratio = (float)sol_R1 / (float)sol_R2;

float battery_Voltage = 0.0;
float solar_Voltage = 0.0;

long lastReadingTime = 0;
long lastTestTime = 0;
long lastTestStart = 0;

boolean relais_state = false;
boolean test_mode = true;


void setup() {
  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);
  Serial.begin(9600);
  //inputoutput config 
  pinMode(relais_Pin, OUTPUT);  
  Serial.println(" MOOGAA SOOOLAA V0.1 ");
  Serial.println(" says hello im alive! ");  
}

void read_analog (){
  sol_sampleVolt = 0;
  batt_sampleVolt = 0;

  for (int x = 0; x < 10; x++){ // run through loop 10x
    // read the analog in value:
    sol_sampleVolt = sol_sampleVolt + analogRead(sol_volt_Pin); // add samples together
    batt_sampleVolt = batt_sampleVolt + analogRead(batt_volt_Pin); // add samples together
    delay (50); // let ADC settle before next sample
  }
}

void calc_volt(){
  float avgval = batt_sampleVolt / 10; //divide by 10 (number of samples) to get a steady reading
  float pinVoltage = avgval * 0.00685;       //  Calculate the voltage on the A/D pin
  battery_Voltage = pinVoltage * batt_ratio;    //  Use the ratio calculated for the voltage divider

  avgval = sol_sampleVolt / 10; //divide by 10 (number of samples) to get a steady reading
  pinVoltage = avgval * 0.00525;       //  Calculate the voltage on the A/D pin
  solar_Voltage = pinVoltage * sol_ratio;    //  Use the ratio calculated for the voltage divider
  /*  A reading of 1 for the A/D = 0.0048mV
   if we multiply the A/D reading by 0.00488 then
   we get the voltage on the pin. 
   
   NOTE! .00488 is ideal. I had to adjust
   to .00610 to match fluke meter.
   
   Also, depending on wiring and
   where voltage is being read, under
   heavy loads voltage displayed can be
   well under voltage at supply. monitor
   at load or supply and decide.
   */
}

void send_serial(){

  Serial.print("Solar Volt: ");
  Serial.print(solar_Voltage);
  Serial.print(" Battery Volt: ");
  Serial.print(battery_Voltage);
  //  Serial.print(" Battery Ratio: ");
  //  Serial.print(batt_ratio);
  //  Serial.print(" Solar Ratio: ");
  //  Serial.print(sol_ratio);
  Serial.print(" Relais: ");
  if (relais_state){
    Serial.println("ON");
  }
  else{
    Serial.println("OFF");
  }
}


void send_udp(){
  // send a reply, to the IP address and port that sent us the packet we received
  Udp.beginPacket(destination, destPort);
  Udp.write("solar volt");
  Udp.write(";");
  Udp.print(solar_Voltage);
  Udp.write(";");
  Udp.write("battery volt");
  Udp.write(";");
  Udp.print(battery_Voltage);  
  Udp.write(";");  
  Udp.write("relais");
  Udp.write(";");
  if (relais_state){
    Udp.write("ON");
  }
  else{
    Udp.write("OFF");
  }
  Udp.endPacket();
}

void loop() {
    read_analog();
    calc_volt();
//    relais(solar_Voltage);      //relais mit solar voltage aufrufen
    
  // check for a reading no more than once a second.
  if (millis() - lastReadingTime > 1000){
    send_udp();
    send_serial();
    lastReadingTime = millis();
  }


  //alle 15 min = 1000*60*15 in den testmode
  if ((millis() - lastTestTime > 1000*15) &&! test_mode ){
    test_mode = true;
    
    lastTestStart = millis();
    lastTestTime = millis();    
    Serial.println("test START"); 
  }
  //10 second later schau ob saft am panel
  if (millis() - lastTestStart > 1000 && test_mode){
    //wenn solar größer als batt
    if ( solar_Voltage > battery_Voltage + 0.5 )
    {
      test_mode = false;
      lastTestTime = millis();
      Serial.println("test OK");      
    }
    else {
      lastTestStart = millis() ;    
      Serial.println("test NOK");      
    }

  }

  relais_state = test_mode;
  digitalWrite(relais_Pin, relais_state); 

}






















