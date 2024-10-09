# M5Stick C Plus 2 - tvbgone

Just a fast booting port of tvbgone for the M5Stick C Plus 2.
Features: battery percentage, Region switch, nothing else


Port of popular agrimpelhuber/esp8266-tvbgone
Send 100+ IR-Codes by Buttonpress, to turn off any TV, Projector etc. 
It is meant to be flashed to an M5Stack Stick C Plus 2, using the Arduino Development Platform. 

## Credits

- original TV-B-Gone by Mitch Altman
- Ken Shirriff's [here](https://github.com/shirriff/Arduino-TV-B-Gone)) Arduino port of the original TV-B-Gone
- Mark Szabo's [IRremoteESP8266](https://github.com/markszabo/IRremoteESP8266)
- agrimpelhuber/esp8266-tvbgone based on the original TV-B-Gone by Mitch Altman



## firmware file *.bin
This file was created with [M5 Burner](https://docs.m5stack.com/en/download) from M5Stack and is successfully tested on my M5Stick CP2
You can use this tool to easy burn the TV-B-Gone.bin


## Features

- press C-button 1 sec for power on , press 6 sec for power off
- press A-button to start/stop the IR-Code sequence
- press B-button if region change is required (default is NA=North america)
- Battery percentage is updated once a minute

![PXL_20241009_080502313](https://github.com/user-attachments/assets/d6482421-fa71-4deb-a324-93e44674f464)

![M5StickCP2](https://github.com/user-attachments/assets/829a0f93-1b61-439f-bbf3-d7849efc14ff)
