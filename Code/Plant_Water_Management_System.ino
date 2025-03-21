
         ////////////////////////////////////////////////////  
        //     Arduino Plant Water Management System      //
       //                  w/ BME280                     //
      //          -------------------------             //
     //                 Arduino Nano                   //           
    //               by Kutluhan Aktar                // 
   //                                                //
  ////////////////////////////////////////////////////

// Track the total volume of water spent and evaluate approximate evaporation rates by temperature and humidity to prevent water overuse. 
//
// For more information:
// https://www.theamplituhedron.com/projects/Arduino-Plant-Water-Management-System/
// 
//
// Connections
// Arduino Nano v3:           
//                               Nokia 5110 Screen
// D2 --------------------------- SCK (Clk)
// D3 --------------------------- MOSI (Din) 
// D4 --------------------------- DC 
// D5 --------------------------- RST
// D6 --------------------------- CS (CE)
//                               Adafruit BME280 Temperature Humidity Pressure Sensor
// SCL (A5) --------------------- SCK
// SDA (A4) --------------------- SDI
//                               YF-S201 Hall Effect Water Flow Sensor
// 5V --------------------------- 5V  
// A2 --------------------------- Signal
// GND -------------------------- GND
//                               Soil Moisture Sensor
// 5V --------------------------- 5V 
// GND -------------------------- GND 
// A3 --------------------------- A0
//                               5mm Green LED
// D11 -------------------------- +
//                               5mm Red LED
// D12 -------------------------- +
//                               LEFT_BUTTON
// D7 --------------------------- S
//                               OK_BUTTON
// D8 --------------------------- S
//                               RIGHT_BUTTON
// D9 --------------------------- S
//                               EXIT_BUTTON
// D10 -------------------------- S


// Include required libraries:
#include <LCD5110_Basic.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Define screen settings.
LCD5110 myGLCD(2,3,4,5,6);

extern uint8_t SmallFont[];
extern uint8_t MediumNumbers[];
// Define the graphics for related screen modes.
extern uint8_t tem[];
extern uint8_t hum[];
extern uint8_t usage[];

// Define the Adafruit BME280 Temperature Humidity Pressure Sensor settings:
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

// Define menu options and modes using volatile booleans.
volatile boolean Tem_Eva= false;
volatile boolean Hum_Eva= false;
volatile boolean Moisture = false;
volatile boolean Usage = false;
volatile boolean Activated = false;

// Define the control buttons.
#define B_Exit 10
#define B_Right 9
#define B_OK 8
#define B_Left 7

// Define control LED pins.
#define green 11
#define red 12

// Define the Soil Moisture Sensor settings:
#define moisture_sensor A3

// Define the YF-S201 Hall Effect Water Flow Sensor settings:
#define flow_sensor A2
int up, down;
float pulse_time, frequency, flow_rate, water_spent, total_water_spent;


// Define data holders:
int Right, OK, Left, Exit;
int selected = 0;
float tem_rate, hum_rate;

void setup() {
  // Buttons:
  pinMode(B_Exit, INPUT);
  pinMode(B_Right, INPUT);
  pinMode(B_OK, INPUT);
  pinMode(B_Left, INPUT);
  // LED:
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  
  // Initiate screen.
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);

  Serial.begin(9600);

  // Initiate the Adafruit BME280 Temperature Humidity Pressure Sensor.
  unsigned status;
  status = bme.begin(); // Default I2C settings (0x77)
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  } 
}

void loop() {

    read_buttons();
  
    change_menu_options(4);

    interface();

    if(Tem_Eva == true){
      do{
        myGLCD.invertText(true);
        myGLCD.print("A.Tem. Eva.", 0, 16);
        myGLCD.invertText(false);
        delay(100);
        if(OK == HIGH){
          myGLCD.clrScr();
          Activated = true;
          while(Activated == true){
            read_buttons();
            // Get temperature:
            myGLCD.drawBitmap(0, 0, tem, 16, 16);
            myGLCD.print((String)bme.readTemperature() + " *C", 20, 0);
            myGLCD.print("Approx.Eva. =>:", 0, 24);
            calculate_approx_evaporation("tem");
            myGLCD.print((String)tem_rate + " kg/m3", 0, 32);
            // Define the threshold to activate control LEDs - Green and Red.
            float threshold = 1.50;
            if(tem_rate > threshold){ digitalWrite(green, LOW); digitalWrite(red, HIGH); }
            if(tem_rate <= threshold){ digitalWrite(green, HIGH); digitalWrite(red, LOW); }

            // Exit and clear.
            if(Exit == HIGH) { Activated = false; myGLCD.clrScr(); digitalWrite(green, LOW); digitalWrite(red, LOW); }
          }
        }
      }while(Tem_Eva == false);
    }

    if(Hum_Eva == true){
      do{
        myGLCD.invertText(true);
        myGLCD.print("B.Hum. Eva.", 0, 24);
        myGLCD.invertText(false);
        delay(100);
        if(OK == HIGH){
          myGLCD.clrScr();
          Activated = true;
          while(Activated == true){
            read_buttons();
            // Get humidity:
            myGLCD.drawBitmap(0, 0, hum, 16, 16);
            myGLCD.print((String)bme.readHumidity() + "%", 20, 0);
            myGLCD.print("Approx.Eva. =>:", 0, 24);
            calculate_approx_evaporation("hum");    
            myGLCD.print((String)abs(hum_rate) + " kg/h", 0, 32);
            // Define the threshold to activate control LEDs - Green and Red.
            float threshold = 1.3;
            if(abs(hum_rate) > threshold){ digitalWrite(green, LOW); digitalWrite(red, HIGH); }
            if(abs(hum_rate) <= threshold){ digitalWrite(green, HIGH); digitalWrite(red, LOW); }
            
            // Exit and clear.
            if(Exit == HIGH) { Activated = false; myGLCD.clrScr(); digitalWrite(green, LOW); digitalWrite(red, LOW); }
          }
        }
      }while(Hum_Eva == false);
    }

    if(Moisture == true){
      do{
        myGLCD.invertText(true);
        myGLCD.print("C.Moisture", 0, 32);
        myGLCD.invertText(false);
        delay(100);
        if(OK == HIGH){
          myGLCD.clrScr();
          Activated = true;
          while(Activated == true){
            read_buttons();
            // Print variables - Moisture (%), Pressure (hPa), and Approx. Altitude (m).
            myGLCD.print("Moisture:", 0, 0);
            int moisture = map(analogRead(moisture_sensor), 330, 1023, 100, 0);
            myGLCD.print((String)moisture + "%", 0, 16);
            myGLCD.print("Pr: " + (String)(bme.readPressure() / 100.0F) + "hPa", 0, 32);
            myGLCD.print("Al: " + (String)bme.readAltitude(SEALEVELPRESSURE_HPA) + " m", 0, 40);
            // Define the threshold to activate control LEDs - Green and Red.
            float threshold = 35;
            if(moisture <= threshold){ digitalWrite(green, LOW); digitalWrite(red, HIGH); }
            if(moisture > threshold){ digitalWrite(green, HIGH); digitalWrite(red, LOW); }
            
            // Exit and clear.
            if(Exit == HIGH) { Activated = false; myGLCD.clrScr(); digitalWrite(green, LOW); digitalWrite(red, LOW); }
          }
        }
      }while(Moisture == false);
    }

    if(Usage == true){
      do{
        myGLCD.invertText(true);
        myGLCD.print("D.Usage", 0, 40);
        myGLCD.invertText(false);
        delay(100);
        if(OK == HIGH){
          myGLCD.clrScr();
          Activated = true;
          while(Activated == true){
            read_buttons();
            // Get flow rate and total water usage:
            read_water_flow_sensor();
            // Print variables:
            myGLCD.drawBitmap(0, 0, usage, 32, 32);
            myGLCD.print(" F_Rate:", 32, 0);
            myGLCD.print(" " + (String)water_spent, 32, 8);
            myGLCD.print(" Total:", 32, 32);
            myGLCD.print(" " + (String)total_water_spent, 32, 40);
            // Define the threshold to activate control LEDs - Green and Red.
            float threshold = 100.0;
            if(total_water_spent > threshold){ digitalWrite(green, LOW); digitalWrite(red, HIGH); }
            if(total_water_spent <= threshold){ digitalWrite(green, HIGH); digitalWrite(red, LOW); }
                        
            // Exit and clear.
            if(Exit == HIGH) { Activated = false; myGLCD.clrScr(); digitalWrite(green, LOW); digitalWrite(red, LOW); }
          }
        }
      }while(Usage == false);
    }

}

void read_buttons(){
  // Read the control buttons:
  Right = digitalRead(B_Right);
  OK = digitalRead(B_OK);
  Left = digitalRead(B_Left);
  Exit = digitalRead(B_Exit);
}

void interface(){
   // Define options.
   myGLCD.print("Menu Options :", 0, 0);
   myGLCD.print("A.Tem. Eva.", 0, 16);
   myGLCD.print("B.Hum. Eva.", 0, 24);
   myGLCD.print("C.Moisture", 0, 32);
   myGLCD.print("D.Usage", 0, 40);
}

void change_menu_options(int options){
  // Increase or decrease the option number using Right and Left buttons.
  if(Right == true) selected++;
  if(Left == true) selected--;
  if(selected < 0) selected = options;
  if(selected > options) selected = 1;
  delay(100);
  // Depending on the selected option number, change boolean status.
  switch(selected){
    case 1:
      Tem_Eva = true;
      Hum_Eva = false;
      Moisture = false;
      Usage = false;
    break;
    case 2:     
      Tem_Eva = false;
      Hum_Eva = true;
      Moisture = false;
      Usage = false;
    break;
    case 3:
      Tem_Eva = false;
      Hum_Eva = false;
      Moisture = true;
      Usage = false;
    break;
    case 4:
      Tem_Eva = false;
      Hum_Eva = false;
      Moisture = false;
      Usage = true;
    break;
  }
}

void read_water_flow_sensor(){
  /*
    Q (flow rate) = V (velocity) x A (area)

    Sensor Frequency (Hz) = 7.5 * Q (Litres/min)
    Litres = Q * time elapsed (seconds) / 60 (seconds/minute)
    Litres = (Frequency (Pulses/second) / 7.5) * time elapsed (seconds) / 60
    Litres = Pulses / (7.5 * 60)  
  */

  pulse_time = pulseIn(flow_sensor, HIGH) + pulseIn(flow_sensor, LOW);
  frequency = 1000000 / pulse_time; // 1000000 - 1 second to microseconds
  if(frequency > 0 && !isinf(frequency)){
    flow_rate = frequency / 7.5;
    water_spent = flow_rate / 60;
    delay(300);
  }else{
    flow_rate = 0;
    water_spent = 0;
  }

  total_water_spent += water_spent;
}

void calculate_approx_evaporation(String dat){
  // You can get more information about the formulas I implemented on the project page.
  if(dat == "tem"){
    // n / V = P / RT => the number of moles per cubic meter
    float P = (bme.readPressure() / 100.0F) * 100.0; // Pa
    float R = 8.31; // the ideal gas constant => J/mol*K
    float T = bme.readTemperature() + 273.15; // Kelvin
    // Calculate the density of water vapor (d) in g/m3:
    float n_V = P / (R * T); // mol/m3
    float d = n_V * 18.015; // the molecular mass of water is 18.015 g/mol
    // percent relative humidity = (vapor density / saturation vapor density) * 100
    float saturation = (d / bme.readHumidity()) * 100.0;
    tem_rate = (saturation - d) / 1000.0; // kg/m3
  }
  if(dat == "hum"){
    // gh = Θ A (xs - x) => amount of evaporated water per hour (kg/h)
    float v = 0.30; // velocity of air above the water surface (m/s) => change it before executing the code
    float A = 0.25 * 0.25; // surface area (m2)
    float Q = 25.0 + (19.0 * v); // evaporation coefficient (kg/m2h)
    float xs = 0.019826; // maximum humidity ratio of saturated air by temperature(kg/kg) => approx. for 25 *C
    float x = (bme.readHumidity() / 100.0) * 0.62198; // approx. humidity ratio air (kg/kg)
    hum_rate = Q * A * (xs - x);
  }
}
