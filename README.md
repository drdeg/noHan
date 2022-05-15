# noHan

Interface to RJ45 (Norwegian) HAN port with decoding of Swedish OBIS messages. Based on ESPHome,
this is made for interfacing with Home assistant over MQTT or the native esphome API.

My local electrical power provider Skellefteå Kraft has for some reason decided to use
a Norwegian hardware interface but with Swedish encoding of the OBIS messages. The HAND "standard"
seems to be so open that every operator can find its own variant...

## Hardware

The hardware for this project was created in KiCad. As I have a lot of dependencies to my custom
libraries, I havent cleand the project up so I can make it available publically without being
too embarrased... You can find the schematics and the board layout in the pcb folder.

### Han interface

The interface hardware is connected to the HAN port with a standard (straight) network cable.
A TSS721 is used to convert the awkward signal levels to 3.3V.

### Power supply

As the Norwegian HAN port doesn't supply enough current to power the ESP8266, you'll need to
power the card from an external supply. The ESP8266 side of the decoder is electrically isolated
from the interface side. A standard LM1117 linear regulator is used to provide stable power
to the ESP8266, and it can handle 3.5 to 12V input. There is an option to place a diode on the
power input to protect from reverse voltage (Who would ever do that?), but I often cheat...

## Software

I use [ESPHome](https://esphome.io) to have time for a life and still have fun with custom
ESP hardware. For this project, I had to implement a decoder for the signals as I failed to
find a solution out there that was suitable for my odd needs.

### Configuration

The file [example.yaml](example.yaml) contains an example espHome configuration for the project.
There are some configuration options that you need to add before you can use it, like WiFi and
MQTT broker settings, but that shouldn't be too difficult. I'm sure you'll figure it out.

### Flash

There is a programmin header available on the board that can also be used to supply power
to the ESP during programming. I found some rumours on the internet that there could be
some disturbances during programming from the TSS721 decoder, so I added a jumper (JP1)
that should be open during programming. The pinout is as follows:

1. GND
2. ESP_TX (3.3V serial)
3. ESP_RX (3.3V serial)
4. Vin (Feeds the LM1117 power regulator). 5-12V is recommended.

Flashing with esphome is wonderfully easy! After first flas, OTA is really convenient!
The documentation at esphome is useful for this purpose, in particular the
[getting started command line](https://esphome.io/guides/getting_started_command_line.html) guide.
(I don't use the Docker approach, as I prefer to install esphome using pip)
