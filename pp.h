{
  "version": 1,
  "author": "Gatul",
  "editor": "wokwi",
  "parts": [
    { "type": "wokwi-arduino-nano", "id": "nano", "top": 175.09, "left": 22.03, "attrs": {} },
    { "type": "chip-pcf8574", "id": "chip1", "top": 50, "left": 0, "attrs": {} },
    {
      "type": "wokwi-led",
      "id": "led8",
      "top": -26,
      "left": 110,
      "attrs": { "color": "blue", "label": "P7" }
    },
    {
      "type": "wokwi-led",
      "id": "led7",
      "top": -26,
      "left": 140,
      "attrs": { "color": "blue", "label": "P6" }
    },
    {
      "type": "wokwi-led",
      "id": "led6",
      "top": -26,
      "left": 170,
      "attrs": { "color": "blue", "label": "P5" }
    },
    {
      "type": "wokwi-led",
      "id": "led5",
      "top": -26,
      "left": 200,
      "attrs": { "color": "blue", "label": "P4" }
    },
    {
      "type": "wokwi-led",
      "id": "led4",
      "top": -26,
      "left": 230,
      "attrs": { "color": "blue", "label": "P3" }
    },
    {
      "type": "wokwi-led",
      "id": "led3",
      "top": -26,
      "left": 260,
      "attrs": { "color": "blue", "label": "P2" }
    },
    {
      "type": "wokwi-led",
      "id": "led2",
      "top": -26,
      "left": 290,
      "attrs": { "color": "blue", "label": "P1" }
    },
    {
      "type": "wokwi-led",
      "id": "led1",
      "top": -26,
      "left": 320,
      "attrs": { "color": "blue", "label": "P0" }
    }
  ],
  "connections": [
    [ "led8:C", "led7:C", "black", [ "v15", "h30" ] ],
    [ "led7:C", "led6:C", "black", [ "v15", "h30" ] ],
    [ "led6:C", "led5:C", "black", [ "v15", "h30" ] ],
    [ "led5:C", "led4:C", "black", [ "v15", "h30" ] ],
    [ "led4:C", "led3:C", "black", [ "v15", "h30" ] ],
    [ "led3:C", "led2:C", "black", [ "v15", "h30" ] ],
    [ "led2:C", "led1:C", "black", [ "v15", "h30" ] ],
    [ "chip1:P0", "led1:A", "#8f4814", [ "h0" ] ],
    [ "chip1:P1", "led2:A", "purple", [ "h0" ] ],
    [ "chip1:P2", "led3:A", "orange", [ "h0" ] ],
    [ "chip1:P3", "led4:A", "gold", [ "h0" ] ],
    [ "chip1:P4", "led5:A", "green", [ "h0" ] ],
    [ "chip1:P5", "led6:A", "blue", [ "h0" ] ],
    [ "chip1:P6", "led7:A", "violet", [ "h0" ] ],
    [ "chip1:P7", "led8:A", "gray", [ "h0" ] ],
    [ "chip1:VCC", "nano:5V", "red", [ "h-32", "v148", "h170" ] ],
    [ "chip1:SDA", "nano:A4", "cyan", [ "h-42", "v235", "h145" ] ],
    [ "chip1:SCL", "nano:A5", "white", [ "h-52", "v235", "h165" ] ],
    [ "chip1:GND", "nano:GND.1", "black", [ "h-22", "v128", "h180" ] ],
    [ "nano:GND.2", "led1:C", "black", [ "v-32", "h218", "v-118", "h-30" ] ]
  ],
  "dependencies": {}
}