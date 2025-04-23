#include "mbed.h"
#include "DHT11.h"

#define TX PA_9    // UART TX
#define RX PA_10   // UART RX

#define DHT11_PIN PA_5   // Pin connected to the DHT11 data pin
#define LDR_PIN   PA_1   // ADC pin connected to the LDR
#define LED_PIN   PA_0   // Pin connected to the LED
#define LIGHT_THRESHOLD 500       // Adjust based on your LDR calibration

// Define the ThingSpeak Host and API Key
#define HOST "api.thingspeak.com"
#define API_KEY "PN4EFWVLMCQ1NV7W"

Serial esp(TX, RX); // UART TX,RX
Serial pc(USBTX, USBRX); // USB serial for debugging

// Sensor and Actuator Objects
DHT11 dht11(DHT11_PIN);  // DHT11 sensor
AnalogIn ldr(LDR_PIN);   // LDR input
DigitalOut led(LED_PIN); // LED output

// Function to connect to Wi-Fi
void ConnectToWiFi()
{
    pc.baud(115200); //SET SERIAL BAUD RATE
    esp.baud(115200);

    pc.printf("Starting Wi-Fi connection test...\r\n");

    pc.printf("Resetting ESP8266...\r\n");
    esp.printf("AT+RST\r\n");
    wait(2);

    pc.printf("Setting ESP8266 to Station mode...\r\n");
    esp.printf("AT+CWMODE=1\r\n");  // Set ESP8266 to station mode
    wait(3);

    esp.printf("AT+CWJAP=\"Faisal\",\"faisal1234\"\r\n"); // Set WiFi SSID and Password
    wait(5);
    pc.printf("Done setting Wifi...\r\n");

}

// Function to send data to ThingSpeak
void SendDataToThingSpeak(float temperature, float humidity, float ldr_value)
{
    char buffer[200];

    // Reinitialize TCP connection
    esp.printf("AT+CIPSTART=\"TCP\",\"%s\",80\r\n", HOST);
    wait(2);

    // Prepare the HTTP GET request with the API Key and field data

        // Prepare HTTP GET request with all fields
    snprintf(buffer, sizeof(buffer),
             "GET /update?api_key=%s&field1=%.2f&field2=%.2f&field3=%.2f\r\n\r\n",
             API_KEY, temperature, humidity, ldr_value);

    // Send the length of data
    esp.printf("AT+CIPSEND=%d\r\n", strlen(buffer));
    wait(1);

    // Send the actual data
    esp.printf("%s", buffer);
    wait(3);

    // Close the TCP connection after sending
    esp.printf("AT+CIPCLOSE\r\n");
    wait(1);
}

int main() {
    float ldr_value;
    float temperature;
    float humidity;

    ConnectToWiFi();  // Connect to Wi-Fi

    while (1) {
        // Read LDR value
        ldr_value = ldr.read() * 1023; // Scale to 0-1023
        pc.printf("LDR Value: %.2f\n", ldr_value);

        // Control LED based on LDR value
        if (ldr_value < LIGHT_THRESHOLD) {
            led = 1; // Turn on LED
            pc.printf("LED ON: Insufficient light\n");
        } else {
            led = 0; // Turn off LED
            pc.printf("LED OFF: Sufficient light\n");
        }

        // Read data from DHT11
        if (dht11.readData() == DHT11::OK) {
            temperature = dht11.readTemperature();
            humidity = dht11.readHumidity();
            pc.printf("Temperature: %.2f C, Humidity: %.2f %%\n", temperature, humidity);
        } else {
            pc.printf("Error reading DHT11 sensor.\n");
            temperature = 0;  // Default value in case of an error
            humidity = 0;     // Default value in case of an error
        }

        // Send data to ThingSpeak
        SendDataToThingSpeak(temperature, humidity, ldr_value);
        wait(17);  // Wait 15 seconds before the next update
    }
}