#include <Arduino.h>
#include <FastLED.h>

#define B1 15
#define B2 20

#define NUM_PANEL 8
#define DATA_PIN 10
#define POT_PIN 5
#define NUM_PER_SECTION 31 //31 final
#define NUM_LEDS NUM_PANEL * NUM_PER_SECTION
#define TEENSY_LED 13

typedef struct {
  int s_num; //section ID
  int s_state; //active 1 - inactive 0 pattern 2
  int intensity;
  CRGB ledColor;
} Section;

long t; //time
long frameCount;
float _fade = 0.02;
int direction = 1; //snake direction
int snakeStart = 0; //snake start position
int selectedPanel = 0;
int snakeOG = -1;


int pushTime = 0;
bool buttonState = false;
bool inputReady = true;

Section panel[NUM_PANEL];
CRGB leds[NUM_LEDS]; //final value
CRGB colors[NUM_LEDS]; //OG color

void setup() {
  delay(500);
  Serial.begin(9600);
  while (!Serial) {
    if (millis() > 1500)
      break;
  }

  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(TEENSY_LED, OUTPUT);

  FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
  Serial.printf("Added %d LEDS.", NUM_LEDS);

  //Leds goes to black
  for(int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::White;
  }

  // Initalize the panels
  for(int i = 0; i < NUM_PANEL; i++) {
      panel[i] = {.s_num = i, .s_state = 0, .intensity = 1, .ledColor = CRGB::Black};
  }
  //panel[0].s_state = 0;
  panel[1].s_state = 3;
  //panel[3].s_state = 0;
}

void checkInput() {
  if(!digitalRead(B1) && !digitalRead(B2) && inputReady == true) {
    if (snakeOG == -1)
      snakeStart = snakeOG = selectedPanel * NUM_PER_SECTION;
    panel[selectedPanel].s_state = 1;
    pushTime++;
    buttonState = true;
    digitalWrite(TEENSY_LED, 1);
    Serial.println(pushTime);
  }

  if(pushTime > 300 && buttonState){
    panel[selectedPanel].s_state = 3;
    buttonState = false;
    inputReady = false;
  }

  if(digitalRead(B1) && digitalRead(B2)) {
    snakeOG = -1;
    digitalWrite(TEENSY_LED, 0);
    inputReady = true;
    pushTime = 0;
    panel[selectedPanel].s_state = 0;
  }
}

CRGB snake(int l) {
  int snakeLenght = 10;
  CRGB r = leds[l];

  if(l > snakeStart - snakeLenght && l < snakeStart) {
    r = CRGB::White;
    float s = (float)((snakeStart - l) + snakeLenght)/snakeLenght;
    s = (s - 1.0f) * direction;
    r = r.subtractFromRGB(255.0f * s);
  }

  return r;
}

void loop() {
  t = millis();

  int pot = map(analogRead(A0), 0, 1023, 0, 7);
  selectedPanel = pot;
  Serial.printf("Pot -> %d\n", pot);

  checkInput();

  for(int i = 0; i < NUM_PANEL; i++) {
    int l = panel[i].s_num * NUM_PER_SECTION;

    //SNAKE
    if(panel[i].s_state == 1) {
      int _min =  selectedPanel * NUM_PER_SECTION;
      int _max = (selectedPanel + 1) * NUM_PER_SECTION;

      Serial.printf("Snake active -> %d\n", snakeStart);

      if(snakeStart < _min || snakeStart > _max) {
        direction = -direction;
        Serial.printf("Switching!");
      }

      if(frameCount % 3 == 0)
        snakeStart += direction;
    }

    for(int j = 0; j < NUM_PER_SECTION; j++) {
      int currentLed = l + j;
      CRGB l_color = CRGB::Black;

      if(panel[i].s_state == 0) {
        l_color = CRGB::Black;
      }
      //BEING CLICKED
      if(panel[i].s_state == 1) {
        l_color = snake(currentLed);
      }
      //ACTIVE
      if(panel[i].s_state == 3) {
        l_color.setRGB(0, 0, inoise8(t/3.0f, (i+j) * 150));
        l_color.addToRGB(inoise8(t/15.0f)/5);
      }
      leds[l + j] = l_color;
    }
  }
  delay(5);
  frameCount++;
  if(frameCount > 30000)
    frameCount = 0;

  FastLED.show();
}
