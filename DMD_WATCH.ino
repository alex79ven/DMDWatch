/*
Copyright 2016 German Martin (gmag11@gmail.com). All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met :

1. Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and / or other materials
provided with the distribution.

THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ''AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
    ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING
        NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
    ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    The views and conclusions contained in the software and documentation are those of the
    authors and should not be interpreted as representing official policies, either expressed
    or implied, of German Martin
*/

/*
 Name:		NtpClient.ino
 Created:	20/08/2016
 Author:	gmag11@gmail.com
 Editor:	http://www.visualmicro.com
*/

#include <TimeLib.h>
#include "WifiConfig.h"
#include <NtpClientLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#ifndef WIFI_CONFIG_H
#define YOUR_WIFI_SSID "YOUR_WIFI_SSID"
#define YOUR_WIFI_PASSWD "YOUR_WIFI_PASSWD"
#endif // !WIFI_CONFIG_H

#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>

#define pin_A 16
#define pin_B 12
#define pin_sclk 4
#define pin_noe 15
#define panel_width 1
#define panel_heigh 1


SPIDMD dmd(panel_width, panel_heigh, pin_noe, pin_A, pin_B, pin_sclk);  // DMD controls the entire display
// the setup routine runs once when you press reset:
#define ONBOARDLED 2 // Built in LED on ESP-12/ESP-07

int8_t timeZone = 6;
int8_t minutesTimeZone = 0;
bool wifiFirstConnected = false;

void onSTAConnected (WiFiEventStationModeConnected ipInfo) {
    Serial.printf ("Connected to %s\r\n", ipInfo.ssid.c_str ());
}


// Start NTP only after IP network is connected
void onSTAGotIP (WiFiEventStationModeGotIP ipInfo) {
    Serial.printf ("Got IP: %s\r\n", ipInfo.ip.toString ().c_str ());
    Serial.printf ("Connected: %s\r\n", WiFi.status () == WL_CONNECTED ? "yes" : "no");
    digitalWrite (ONBOARDLED, LOW); // Turn on LED
    wifiFirstConnected = true;
}

// Manage network disconnection
void onSTADisconnected (WiFiEventStationModeDisconnected event_info) {
    Serial.printf ("Disconnected from SSID: %s\n", event_info.ssid.c_str ());
    Serial.printf ("Reason: %d\n", event_info.reason);
    digitalWrite (ONBOARDLED, HIGH); // Turn off LED
    //NTP.stop(); // NTP sync can be disabled to avoid sync errors
}

void processSyncEvent (NTPSyncEvent_t ntpEvent) {
    if (ntpEvent) {
        Serial.print ("Time Sync error: ");
        if (ntpEvent == noResponse)
            Serial.println ("NTP server not reachable");
        else if (ntpEvent == invalidAddress)
            Serial.println ("Invalid NTP server address");
    } else {
        Serial.print ("Got NTP time: ");
        Serial.println (NTP.getTimeDateString (NTP.getLastNTPSync ()));
    }
}

boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event

void setup () {

    static WiFiEventHandler e1, e2, e3;

    Serial.begin (115200);
    Serial.println ();
    WiFi.mode (WIFI_STA);
    WiFi.begin (YOUR_WIFI_SSID, YOUR_WIFI_PASSWD);

    pinMode (ONBOARDLED, OUTPUT); // Onboard LED
    digitalWrite (ONBOARDLED, HIGH); // Switch off LED

    NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
        ntpEvent = event;
        syncEventTriggered = true;
    });

    // Deprecated
    /*WiFi.onEvent([](WiFiEvent_t e) {
        Serial.printf("Event wifi -----> %d\n", e);
    });*/

    e1 = WiFi.onStationModeGotIP (onSTAGotIP);// As soon WiFi is connected, start NTP Client
    e2 = WiFi.onStationModeDisconnected (onSTADisconnected);
    e3 = WiFi.onStationModeConnected (onSTAConnected);
    // dmd.setBrightness(55);
  dmd.selectFont(SystemFont5x7);
  dmd.begin();

ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
  


void loop () {
    ArduinoOTA.handle();
    static int i = 0;
    static int last = 0;
String mnt;
    if (wifiFirstConnected) {
        wifiFirstConnected = false;
        NTP.begin ("pool.ntp.org", timeZone, true, minutesTimeZone);
        NTP.setInterval (63);
    }

    if (syncEventTriggered) {
        processSyncEvent (ntpEvent);
        syncEventTriggered = false;
    }

    if ((millis () - last) > 5100) {
        //Serial.println(millis() - last);
        last = millis ();
        Serial.print (i); Serial.print (" ");
        Serial.print (NTP.getTimeDateString ()); Serial.print (" ");
        Serial.print (NTP.isSummerTime () ? "Summer Time. " : "Winter Time. ");
        Serial.print ("WiFi is ");
        Serial.print (WiFi.isConnected () ? "connected" : "not connected"); Serial.print (". ");
        Serial.print ("Uptime: ");
        Serial.print (NTP.getUptimeString ()); Serial.print (" since ");
        Serial.println (NTP.getTimeDateString (NTP.getFirstSync ()).c_str ());

        i++;
        if (NTP.getTimeDateString ().substring(12, 14)=="01") {mnt="JAN";}
        if (NTP.getTimeDateString ().substring(12, 14)=="02") {mnt="FEB";}
        if (NTP.getTimeDateString ().substring(12, 14)=="03") {mnt="MAR";}
        if (NTP.getTimeDateString ().substring(12, 14)=="04") {mnt="APR";}
        if (NTP.getTimeDateString ().substring(12, 14)=="05") {mnt="MAY";}
        if (NTP.getTimeDateString ().substring(12, 14)=="06") {mnt="JUN";}
        if (NTP.getTimeDateString ().substring(12, 14)=="07") {mnt="JUL";}
        if (NTP.getTimeDateString ().substring(12, 14)=="08") {mnt="AUG";}
        if (NTP.getTimeDateString ().substring(12, 14)=="09") {mnt="SEP";}
        if (NTP.getTimeDateString ().substring(12, 14)=="10") {mnt="OCT";}
        if (NTP.getTimeDateString ().substring(12, 14)=="11") {mnt="NOV";}
        if (NTP.getTimeDateString ().substring(12, 14)=="12") {mnt="DEC";}

        dmd.drawString(1,0,String(NTP.getTimeDateString ().substring(0, 5)));
         dmd.drawString(1,9,String(NTP.getTimeDateString ().substring(9, 11)+mnt));
           
    }  

  
    
    delay (0);
}


 
