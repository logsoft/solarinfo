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
// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged";       // a string to send back
// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;


long lastReadingTime = 0;
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

void setup() {
  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);

  Serial.begin(9600);

  //inputoutput config
  pinMode(relais_Pin, OUTPUT);

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
  Udp.endPacket();
}

void relais(float volt){
  if (volt >= 14.0){
    digitalWrite(relais_Pin, LOW);
    Serial.println("Relais OFF");
  }
  else if (volt <= 13.5){
    digitalWrite(relais_Pin, HIGH);
    Serial.println("Relais ON");
  }
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

void calc_battery_volt(){
  float avgval = batt_sampleVolt / 10; //divide by 10 (number of samples) to get a steady reading
  float pinVoltage = avgval * 0.00685;       //  Calculate the voltage on the A/D pin
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

  //float ratio = (float)R1 / (float)R2;
  battery_Voltage = pinVoltage * batt_ratio;    //  Use the ratio calculated for the voltage divider
  //  to calculate the battery voltage
}

void calc_solar_volt(){
  float avgval = sol_sampleVolt / 10; //divide by 10 (number of samples) to get a steady reading
  float pinVoltage = avgval * 0.00525;       //  Calculate the voltage on the A/D pin
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

  //float ratio = (float)R1 / (float)R2;

  solar_Voltage = pinVoltage * sol_ratio;    //  Use the ratio calculated for the voltage divider
  //  to calculate the battery voltage
}


void loop() {

  // check for a reading no more than once a second.
  if (millis() - lastReadingTime > 1000){
    read_analog();
    calc_battery_volt();
    calc_solar_volt();
    send_udp();
    //relais mit solar voltage aufrufen
    relais(solar_Voltage);

    Serial.print("Solar Volt: ");
    Serial.print(solar_Voltage);
    Serial.print(" Battery Volt: ");
    Serial.print(battery_Voltage);
    Serial.print(" Battery Ratio: ");
    Serial.print(batt_ratio);
    Serial.print(" Solar Ratio: ");
    Serial.println(sol_ratio);
    // send_udp();

    lastReadingTime = millis();
  }
}
















