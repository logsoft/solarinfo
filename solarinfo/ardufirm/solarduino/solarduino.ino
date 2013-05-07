#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include <LiquidCrystal.h>

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

/*
 initialize the library with the numbers of the interface pins
 * LCD RS pin to digital pin 7
 * LCD Enable pin to digital pin 6
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 */
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

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
float battery_leerlauf_Volt = 0.0;
float solar_leerlauf_Volt = 0.0;

long lastReadingTime = 0;
long lastTestTime = 0;
long lastTestStart = 0;

boolean relais_state = false;
boolean test_mode = true;

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);


void setup() {
  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);
  Serial.begin(57600);
  //inputoutput config 
  pinMode(relais_Pin, OUTPUT);  

  // start the Ethernet connection and the server:
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  lcd.begin(16, 2);      // set up the LCD's number of columns and rows: 
  lcd.print("MOOGAA SOOLAA   V0.2");// Print a message to the LCD.
  Serial.println(" MOOGAA SOOOLAA V0.2 ");
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
    battery_leerlauf_Volt = battery_Voltage;
    solar_leerlauf_Volt = solar_Voltage;
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

  ///  WEBSERVA ///////

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // add a meta refresh tag, so the browser pulls again every 5 seconds:
          client.println("<meta http-equiv=\"refresh\" content=\"5\">");
          // output the value of each analog input pin

            client.print("Solar Spannung: ");
          client.print(solar_Voltage);
          client.println("<br />");       

          client.print("Batterie Spannung: ");
          client.print(battery_Voltage);
          client.println("<br />");       

          client.print("Batterie Leerlauf Spannung: ");
          client.print(battery_leerlauf_Volt);
          client.println("<br />");

          client.print("Solar Leerlauf Spannung: ");
          client.print(solar_leerlauf_Volt);
          client.println("<br />");

          client.print("Batterie wird : ");
          if (test_mode){
            client.print("entladen");
          }
          else{
            client.print("geladen");
          }
          client.println("<br />");

          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }



}
























