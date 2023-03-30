#include <MKRWAN.h>
#include <DHT.h>

#define DHTTYPE DHT22
#define DHTPIN 2

#define BUFFER_SIZE  4 // Data buffer size in bytes, 2 bytes by values (humidity and temperature for now)

DHT dht(DHTPIN, DHTTYPE);

LoRaModem modem;

// appEUI and appKey given by the Helium network when registering the device
String appEui = "B6280ADA9AA64F14";
String appKey = "909DD3E7BD937C6C0E9B1B4BD1B39B89";

void setup() {
  Serial.begin(115200);
  dht.begin();


  while (!Serial);
  Serial.println("Register to your favourite LoRa network and we are ready to go!");
    
  // change this to your regional band (eg. US915, AS923, ...)
  if (!modem.begin(EU868)) {
    Serial.println("Failed to start module");
    while (1) {}
  };

  // code from the first configuration sketch in MKRWAN library (useful for udapte)
  Serial.print("Your module version is: ");
  Serial.println(modem.version());
    if (modem.version() != ARDUINO_FW_VERSION) {
      Serial.println("Please make sure that the latest modem firmware is installed.");
      Serial.println("To update the firmware upload the 'MKRWANFWUpdate_standalone.ino' sketch.");
    }

  // getting the device EUI (unique number) ro register the board in Helium  
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());
  
  //connection to the Helium Network
  int connected = modem.joinOTAA(appEui, appKey);

  // better connection in outdoor environments. If the message appears, reupload the code
  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    while (1) {}
  }

  
}

// conversion to byte for better recup of the data
void int_to_byte_array(int16_t n, uint8_t* buf) {
  buf[0] = (n >> 8) & 0xFF;
  buf[1] = n & 0xFF;
}

void loop() {
  
  uint8_t buffer[BUFFER_SIZE] = {0};
  
  int16_t h = dht.readHumidity();
  int16_t t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Error reading from DHT sensor!");
    return;
  }

  //each value takes 2 bytes. Change the buffer_size if you want to add values, only size 4 now
  int_to_byte_array(h, &buffer[0]);
  int_to_byte_array(t, &buffer[2]);  

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print("%  Temperature: ");
  Serial.print(t);
  Serial.println("°C ");

  Serial.print("Readings byte array: ");
  for (int i = 0; i < BUFFER_SIZE; i++) {
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // data sending
  int err = 0;
  modem.beginPacket();
  modem.write(buffer, BUFFER_SIZE);
  err = modem.endPacket(true);

  if (err > 0) {
    Serial.println("Message sent correctly!");
  } else {
    Serial.println("Error sending message");
  }

  //data sending and receiving happens at the same time with Helium network
  
  // Check if there is any incoming message
  int packetSize = modem.parsePacket();
  if (packetSize) {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);

    //read packet
    while(modem.available()){
      Serial.write(modem.read());
      }
  }

  // deepsleep should be here, but we got issues when implementing it so we put a delay of 1h
  delay(60 * 60 * 1000); 
  
}