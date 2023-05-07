# MySensors nRF24 Linky Module
IoT energy sensor for Linky electricity meters in France

### What is MySensors?
Always wanted to make your own IoT sensors to automate your home? [MySensors](https://www.mysensors.org/) does all the heavy lifting so you can focus on what matters. It integrates into a lot of [controllers](https://www.mysensors.org/controller) (Home Assistant, Jeedom, Domoticz, and more) so you can easily connect custom made devices into your existing home automation setup.

### Product description
This module plugs into french Linky electricity meters to read real-time information provided through the official consumer-side Télé-Information Client (TIC) output. That information, refreshed approximately every 10 seconds includes:
- Power (in W)
- Current (in A) for each phase
- Voltage (in V) for each phase (when available)
- Accumulated consumption (in Wh)

### Features
- Designed for [MySensors](https://www.mysensors.org/) with a nRF24 radio
- Self powered, doesn't require a battery
- Opensource firmware
- Follows [Enedis-NOI-CPT_54E](https://www.enedis.fr/media/2035/download) specification
- Auto detects baud rate and mode (1200 bps for historic, 9600 bps for standard)

### Known limitations
The TIC output provides a lot more information (heures pleines et creuses, couleur des jours de l'option tempo, électricité produite, ...). The firmware currently doesn't report those, but if you are interested, you are welcome to submit a pull request or open a ticket.

### Get one
<a href="https://www.tindie.com/products/sitronlabs/mysensors-nrf24-linky-module/?ref=offsite_badges&utm_source=sellers_sitronlabs&utm_medium=badges&utm_campaign=badge_small"><img src="https://d2ss6ovg47m0r5.cloudfront.net/badges/tindie-smalls.png" alt="I sell on Tindie" width="200" height="55"></a>
