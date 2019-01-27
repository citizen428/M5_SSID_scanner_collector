#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "Free_Fonts.h"

#define TFT_BROWN 0x38E0
// Pause in milliseconds between screens, change to 0 to time font rendering
#define WAIT 1000

unsigned long targetTime = 0; // Used for testing draw times

// A couple of counters and variables for navigation
int q = 0;       // general counter of total SSID found
int open_q = 0;  // total number of found open SSIDs
int cur_pos = 0; // current SSID position in ID

HTTPClient http;
int httpCode;

// An array to hold the found SSID data in.
String SSID[100][7];

// Basic progress bar, it takes 4 variables:
// x position, y postion, width and height
void progBar(int xpos, int ypos, int w, int h ) {
  for (int i = 0; i < w ; ++i) {
    M5.Lcd.fillRect(xpos, ypos, i, h, GREEN);

    if (i == (w - 1)) {
      // Overwrite the written bar with a black bar to erase the previous state
      M5.Lcd.fillRect(xpos, ypos, w, h, BLACK);
      delay(100);
    }

    delay(50);
  }
}

// This function draws/writes the basic information of the SSID found.
void drawData(int iq) {
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setFreeFont(FF22);
  M5.Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Lcd.drawCentreString(String(cur_pos), 10, 5, 1);

  M5.Lcd.drawLine(30, 0, 30, 30, WHITE);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.drawCentreString(String(q), 110, 6, 1);
  M5.Lcd.drawCentreString(String(open_q), 232, 6, 1);

  M5.Lcd.setFreeFont(FF18);
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Lcd.drawCentreString("Total:", 65, 5, 1);
  M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
  M5.Lcd.drawString("Open:", 150, 5, 1);
  M5.Lcd.drawLine(140, 0, 140, 30, WHITE);
  M5.Lcd.drawLine(0, 30, int(M5.Lcd.width()), 30, WHITE);
  M5.Lcd.setFreeFont(FF17);
  M5.Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Lcd.drawString("SSID :", 10, 45, GFXFF);
  M5.Lcd.drawString("MAC :", 10, 65, GFXFF);
  M5.Lcd.drawString("CH :", 10, 85, GFXFF);
  M5.Lcd.drawString("RSSI :", 10, 105, GFXFF);
  M5.Lcd.drawString("AUTH :", 10, 125, GFXFF);
  M5.Lcd.drawLine(0, 149, int(M5.Lcd.width()), 149, WHITE);
  M5.Lcd.drawString("IP :", 10, 158, GFXFF);
  M5.Lcd.drawString("Ext.IP :", 10, 177, GFXFF);
  M5.Lcd.drawLine(0, 202, int(M5.Lcd.width()), 202, WHITE);
  M5.Lcd.setFreeFont(FF21);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  // "Fill/Write/Draw" the data for each SSID
  M5.Lcd.drawString(SSID[iq][0], 70, 45, GFXFF);
  M5.Lcd.drawString(SSID[iq][1], 70, 65, GFXFF);
  M5.Lcd.drawString(SSID[iq][2], 70, 85, GFXFF);
  M5.Lcd.drawString(SSID[iq][3], 70, 105, GFXFF);
  M5.Lcd.drawString(SSID[iq][4], 70, 125, GFXFF);
  M5.Lcd.drawString(SSID[iq][5], 70, 158, GFXFF);
  M5.Lcd.drawString(SSID[iq][6], 70, 177, GFXFF);

  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Lcd.drawString("  <<   ", 30, 210, GFXFF);
  if ( (SSID[iq][4] == "Open")) {
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5.Lcd.drawString("Test", 140, 210, GFXFF);
  } else {
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.drawString("TOP" , 140, 210, GFXFF);
  }
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Lcd.drawString("   >>  ", 250, 210, GFXFF);

}

// Initial splash screen to be displayed while scanning.
void splashSc() {
  M5.Lcd.setFreeFont(FF23);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.drawCentreString("Scanning", 130, 60, 1);
}

// Write findings to files at the root of the SD card
void writeFile() {
  // write open SSID data to a file named M5open.txt
  for (int wi = 0; wi < q; ++wi) {
    // write all SSID data to a file named M5all.txt
    File allFile = SD.open("/M5all.txt", FILE_APPEND);

    if (allFile) {
      allFile.print(SSID[wi][0]);
      allFile.print(",");
      allFile.print(SSID[wi][1]);
      allFile.print(",");
      allFile.print(SSID[wi][2]);
      allFile.print(",");
      allFile.print(SSID[wi][4]);

      allFile.println();
    }

    allFile.close();

    if (SSID[wi][4] == "Open" ) {
      File openFile = SD.open("/M5open.txt", FILE_APPEND);
      if (openFile) {
        openFile.print(SSID[wi][0]);
        openFile.print(",");
        openFile.print(SSID[wi][1]);
        openFile.print(",");
        openFile.print(SSID[wi][2]);
        openFile.print(",");
        openFile.println(SSID[wi][4]);
      }
      openFile.close();
    }
  }
}

// The main cream of the source code. This scans for SSIDs.
void scanSSID() {
  splashSc();

  q = WiFi.scanNetworks();
  open_q = 0;
  String encryptType = "";

  if (q != 0) {

    for (int i = 0; i < q; ++i) {

      switch (WiFi.encryptionType(i)) {
        case 1:
          encryptType = "WEP";
          break;
        case 2:
          encryptType = "WPA/PSK/TKIP";
          break;
        case 3:
          encryptType = "WPA/PSK/CCMP";
          break;

        case 4:
          encryptType = "WPA2/PSK/Mixed/CCMP";
          break;
        case 8:
          encryptType = "WPA/WPA2/PSK";
          break ;
        case 0:
          encryptType = "Open";
          break ;

      }
      SSID[i][0] = WiFi.SSID(i);
      SSID[i][1] = WiFi.BSSIDstr(i);
      SSID[i][2] = WiFi.channel(i);
      SSID[i][3] = WiFi.RSSI(i);
      SSID[i][4] = encryptType;
      SSID[i][5] = "n/a";
      SSID[i][6] = "n/a";

      // If the SSID is an open SSID , increment the found openSSID counter.
      if ( WiFi.encryptionType(i) == WIFI_AUTH_OPEN ) {
        ++open_q;
      }
    }
    progBar(50, 140, 150, 20);
  }

  drawData(0);
}

// Connects to an open Wifi and tests if it can get out to the internet to test
// if its a "captive portal" or a "full open" SSID.
void  connectOpen(int oi) {
  // Redraw over the bottom navigation to make it look like it was "cleared"
  M5.Lcd.setFreeFont(FF21);
  M5.Lcd.setTextColor(TFT_BLACK, TFT_BLACK);
  M5.Lcd.drawString("  <<   ", 30, 210, GFXFF);
  M5.Lcd.setTextColor(TFT_BLACK, TFT_BLACK);
  M5.Lcd.drawString("Test", 140, 210, GFXFF);
  M5.Lcd.setTextColor(TFT_BLACK, TFT_BLACK);
  M5.Lcd.drawString("   >>  ", 250, 210, GFXFF);

  String ssid = SSID[oi][0];
  unsigned long con_start = millis();

  WiFi.begin(ssid.c_str());

  unsigned long wcon_start = millis();

  // Waiting in line for a spot on the hotspot.
  while ((WiFi.status() != WL_CONNECTED) && (millis() - wcon_start < 15000) ) {
    progBar(250, 0, 80, 10);
    delay(1);
  }

  // The WiFi spot didn't like us or the WiFi gods aren't with us. The scanner
  // could not connect to the WiFi spot.
  if (WiFi.status() != WL_CONNECTED) {
    SSID[oi][5] = "Wifi error";
    M5.Lcd.setFreeFont(FF21);
    M5.Lcd.setTextColor(TFT_BLACK, TFT_BLACK);
    M5.Lcd.drawString("n/a", 70, 158, GFXFF);
    delay(300);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.drawString(SSID[oi][5], 70, 158, GFXFF);
  }

  Serial.println("Before [6] is");
  Serial.println(SSID[oi][6]);

  // Ok so we are now connected to the WiFi spot.
  if (WiFi.status() == WL_CONNECTED) {
    http.begin("http://api.ipify.org/");
    delay(10);
    httpCode = http.GET();
    delay(1000);
    SSID[oi][5] = WiFi.localIP().toString();
    M5.Lcd.setFreeFont(FF21);
    M5.Lcd.setTextColor(TFT_BLACK, TFT_BLACK);
    M5.Lcd.drawString("n/a", 70, 158, GFXFF);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.drawString(SSID[oi][5], 70, 158, GFXFF);

    String response = http.getString();
    unsigned long con_start = millis();

    // Waiting for the server to tell me "we cool"
    while ((httpCode < 0) && (millis() - con_start < 8000)) {
      progBar(250, 0, 80, 10);
      SSID[oi][6] = "http error";
    }

    // If there is some kind of response check what it is
    if (httpCode > 0) {
      // Waiting for a response
      while (response.length() == 0)
      {
        progBar(250, 0, 80, 10);
      }

      // Since the API only returns an IP address, we can assume that
      // response.length is less than 16
      if ((response.length() < 16) && (response != NULL)) {
        SSID[oi][6] = response + " | http: " + httpCode ;
        // So there is a response but its larger than just an IP ie: has some
        // HTML in it.
      } else if ((response.length() > 16) && (response != NULL)) {
        Serial.println("possible captive portal");
        SSID[oi][6] =  "limited/captive portal ?";
      }
    }

    delay(1);

    // Erase the previously drawn string to make it look like it was cleared.
    M5.Lcd.setTextColor(TFT_BLACK, TFT_BLACK);
    M5.Lcd.drawString("n/a", 70, 177, GFXFF);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.drawString(SSID[oi][6], 70, 177, GFXFF);
    http.end();
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.drawString("  <<   ", 30, 210, GFXFF);
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5.Lcd.drawString("Test", 140, 210, GFXFF);
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.drawString("   >>  ", 250, 210, GFXFF);
  }
}

// Button triggers for the bottom navigation bar.
void navBtn() {
  if (M5.BtnA.wasPressed()) {
    if (cur_pos == 0) {
      cur_pos = q ;
    } else {
      --cur_pos;
    }
    drawData(cur_pos);
  }

  if (M5.BtnB.wasPressed()) {
    // if the current SSID is an open SSID, pass on the ID to the connectOpen()
    // function to test if it's full open.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(10);
    if (SSID[cur_pos][4] == "Open") {
      connectOpen(cur_pos);
    } else {
      cur_pos = 0;
    }
  }

  if (M5.BtnC.wasPressed()) {
    if (cur_pos == q) {
      cur_pos = 0 ;
    } else {
      ++cur_pos;
    }
    drawData(cur_pos);
  }
}

void setup() {
  // Put your setup code here, to run once:
  M5.begin();

  // Make sure that it's not connected to some previous WiFi spot.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  // Scan for SSIDs.
  scanSSID();
  delay(100);
  // While it displays the data also write it to files on the SD card.
  writeFile();
}

void loop() {
  // Put your main code here, to run repeatedly.
  // We only need the button code to run continuously.
  navBtn();
  M5.update();
}
