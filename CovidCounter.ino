#include <WiFi.h>
#include <DHT.h>
#include "ArduinoJson.h"
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <TimeLib.h>

// set up pins for the screen
#define TFT_DC 2  // register select pin
#define TFT_RST 4 // reset pin
#define TFT_CS 5  // display enable pin

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
const char *ssid = "[SSID]";
const char *password = "[PASSWORD";
const char *server = "corona.lmao.ninja";
//HTTPClient http;
StaticJsonDocument<900> parsed; //Memory pool
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
WiFiClientSecure client;

const char *root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n"
    "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n"
    "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n"
    "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n"
    "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n"
    "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n"
    "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n"
    "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n"
    "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n"
    "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n"
    "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n"
    "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n"
    "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n"
    "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n"
    "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n"
    "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n"
    "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n"
    "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n"
    "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n"
    "-----END CERTIFICATE-----\n";

void setup()
{
  Serial.begin(9600);
  btStop(); // turn off bluetooth
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.begin(ssid, password);
  delay(100);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15)
  {
    delay(500);
    Serial.print("Connecting to WiFi.. attempt: ");
    Serial.println(attempts);
    attempts = attempts + 1;
  }
  if ((WiFi.status() == WL_CONNECTED))
  {
    Serial.println("Connected to the WiFi network");
    timeClient.begin();
    // timeClient.setTimeOffset(3600); // uncomment for timezone fix, 3600 is GMT+1
    timeClient.update();

    // Start server connection
    client.setCACert(root_ca);
    Serial.println("\nStarting secure connection to server...");
    if (!client.connect(server, 443))
    {
      Serial.println("Connection failed!");
    }
    else
    {
      Serial.println("Connected to server!");
    }
  }
}

void loop()
{
  if ((WiFi.status() == WL_CONNECTED))
  { //Check the current connection status
    // Send the request to API
    // Update the timeclient
    timeClient.update(); //
    client.flush();
    if (client.connected())
    {
      client.println("GET https://corona.lmao.ninja/all HTTP/1.0");
      client.println("Host: corona.lmao.ninja");
      // Keep the connection alive so we dont have to keep opening a new one
      // which can take a couple seconds on an ESP32
      client.println("Connection: Keep-Alive");
      client.println();
      Serial.println("Sent request");

      String res = client.readString();
      // Gets the start of the payload string by searching for the start of the json object
      // This is not the best way to get the payload string
      String payload = res.substring(res.indexOf("{"), res.length());
      client.flush();
      Serial.println("Payload:");
      Serial.println(payload);

      // Deserialize the response
      auto error = deserializeJson(parsed, payload);

      if (error)
      { //Check for errors in parsing
        Serial.println("Parsing failed");
        delay(5000);
        return;
      }

      // Update the display
      updateDisplay(parsed["cases"], parsed["deaths"], parsed["recovered"], payload.substring(58, 68).toInt());

      delay(300000); // wait 2 minutes to refresh
    }
  }
  else
  {
    displayConnectionError();
  }
}

void updateDisplay(int cases, int deaths, int recoveries, int updated)
{
  // Clear the display
  tft.setCursor(0, 0);
  tft.fillScreen(ST7735_BLACK);

  // Draw top line
  tft.setTextColor(ST7735_GREEN);
  tft.drawLine(tft.getCursorX(), tft.getCursorY(), tft.getCursorX() + tft.width(), tft.getCursorY(), ST7735_GREEN);
  tft.drawLine(tft.getCursorX(), tft.getCursorY() + 1, tft.getCursorX() + tft.width(), tft.getCursorY() + 1, ST7735_GREEN);

  // Draw Headline
  tft.setTextSize(2);
  tft.setCursor(0, tft.getCursorY() + 4);
  tft.println("SARS-CoV-2");

  // Draw bottom line
  tft.drawLine(tft.getCursorX(), tft.getCursorY() + 1, tft.getCursorX() + tft.width(), tft.getCursorY() + 1, ST7735_GREEN);
  tft.drawLine(tft.getCursorX(), tft.getCursorY() + 2, tft.getCursorX() + tft.width(), tft.getCursorY() + 2, ST7735_GREEN);

  // Draw confirmed cases
  tft.setCursor(0, tft.getCursorY() + 6);
  tft.setTextColor(ST7735_RED);
  tft.print("Confirmed:");
  tft.println(cases);

  // Draw recoveries
  tft.setCursor(0, tft.getCursorY() + 3);
  tft.setTextColor(ST7735_GREEN);
  tft.print("Recovered:");
  tft.println(recoveries);

  // Draw deaths
  tft.setCursor(0, tft.getCursorY() + 3);
  tft.setTextColor(ST7735_WHITE);
  tft.println("Deaths:");
  tft.println(deaths);


  // Draw last update time and checked time
  // Update time: last time new data was published to the api
  // Checked time: last time the esp32 checked for new data
  // N.B using String in general is not a good idea
  String time = EpochToTimeString(updated);
  tft.setTextSize(1);
  tft.setCursor(0, tft.height() - 20);
  tft.print("Updated: ");
  Serial.println(time);
  tft.println(time);
  tft.print("Checked: ");
  tft.println(timeClient.getFormattedTime());
}

// TODO: fix
// If the received time is 12:05:56
// It shows it like 12:5:56
String EpochToTimeString(int epoch)
{
  TimeElements te;
  breakTime(epoch, te);

  // N.B. Using String in general is not a good idea
  String h = String(te.Hour);
  String m = String(te.Minute);
  String s = String(te.Second);

  return h + ":" + m + ":" + s;
}

void displayConnectionError()
{
  // Display connection error
  tft.setCursor(0, 0);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(1);
  tft.print("Could not connect to WiFi network with SSID: ");
  tft.println(ssid);
  delay(2000);
}
