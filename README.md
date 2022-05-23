# powerwheels
Powerwheels ESC Mod

This code runs on ESP32.

The features include:

Setup button. Using the same existant acelarator buttom.
128x64 Screen used for telemetry.
Controls 2 ESCs
18V battery monitor and power reduction to avoid battery over drain
Temperadure monitoring and power reduction to avoid heat damage

Schematics:
![image](https://user-images.githubusercontent.com/1641239/169619249-4a894549-1af1-4fa7-86e1-9406303b5d04.png)

Car Picture soon:

Max speed:

Board Picture:
Since I'm inexperienced on this my initial thought was to just have pins and connect the cables. But after the second connection I realized that I could use multi-pin connectors and make the setup much easier.
![image](https://user-images.githubusercontent.com/1641239/169619348-85db2b91-0ad7-48db-a3ab-c04aaddd628d.png)
![image](https://user-images.githubusercontent.com/1641239/169620510-5ac5646e-b7c2-4a98-a989-6bcb5d6be8b3.png)


After installing it, I see that I could have used a single string connector and reduced the amount of wires. However I didn't have this info before installing the components as I only took the car apart after creating all the electronics. So if doing it again I would reduce the number of connectors and re-use some of the ground wires, example Display, pedal and programming buttom.

Parts:


Motor: 2 x 3670 (size) / 2150kv https://www.aliexpress.com/item/1005002345878377.html  any 36XX motor will fit as they match the bolt pattern of the original motor.

ESC: Got an 100Amp BLHELI ESC thinking it would be needed, but so far the ESC does not get hot, only the motor https://www.aliexpress.com/item/3256802671531095.html I use the programming cable of the ESC to enable hi-temp cutoff. I wanted a BLHeli32 ESC but could not find one around $30.
