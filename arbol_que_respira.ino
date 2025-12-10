#include <FastLED.h>

// ---------------- CONFIGURACIÓN ----------------
#define DATA_PIN    4       // GPIO 4 (D2 en NodeMCU/Wemos)
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    30      
#define BRIGHTNESS  255     
#define FRAMES_PER_SECOND  60

CRGB leds[NUM_LEDS];

// Variables globales
uint8_t gCurrentPatternNumber = 0; 
uint8_t gHue = 0; 

// Paletas
CRGBPalette16 auroraPalette;
CRGBPalette16 icePalette;

// =================================================================
// 1. PROTOTIPOS DE FUNCIONES
// =================================================================
void auroraBoreal();
void nieveAcumulada();
void chimeneaClasica();
void chimeneaHielo();
void arbolQueRespira();
void estrellasCalidas(); 
void fire2012Original(); 
void nextPattern();
void setupAuroraPalette();
void setupIcePalette();

// =================================================================
// 2. LISTA DE EFECTOS
// =================================================================
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { 
  auroraBoreal, 
  estrellasCalidas,  
  fire2012Original,  
  nieveAcumulada, 
  chimeneaClasica, 
  chimeneaHielo, 
  arbolQueRespira 
};

// =================================================================
// 3. SETUP Y LOOP
// =================================================================
void setup() {
  delay(3000); 

  // Configuración de la tira
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  // Protección de energía
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 450); 
  FastLED.setBrightness(BRIGHTNESS);

  setupAuroraPalette();
  setupIcePalette();
}

void loop() {
  // Ejecuta efecto actual
  gPatterns[gCurrentPatternNumber]();

  FastLED.show(); 
  FastLED.delay(1000 / FRAMES_PER_SECOND); 

  EVERY_N_MILLISECONDS(20) { gHue++; } 
  
  // Cambia de efecto cada 60 segundos
  EVERY_N_SECONDS(300) { nextPattern(); }
}

void nextPattern() {
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % 7;
  FastLED.clear();
}

// =================================================================
// 4. IMPLEMENTACIÓN DE LOS EFECTOS
// =================================================================

// --- AURORA BOREAL (Lenta y Suave) ---
void auroraBoreal() {
  static uint16_t dist = 0; 
  
  // Movimiento lento (cada 30ms avanza 1 paso)
  EVERY_N_MILLISECONDS(30) { 
    dist++; 
  }
  
  for (int i = 0; i < NUM_LEDS; i++) {
    // i*5 para ondas anchas
    uint8_t loc = cubicwave8( dist + i * 5);
    leds[i] = ColorFromPalette(auroraPalette, loc, 255, LINEARBLEND);
  }
}

// --- ESTRELLAS CÁLIDAS ---
void estrellasCalidas() {
  // AQUÍ ESTABA EL ERROR. Ahora le decimos a QUÉ leds aplicar el efecto:
  fadeToBlackBy(leds, NUM_LEDS, 10);
  
  // Probabilidad de encender nueva estrella
  if (random8() < 40) {
    int pos = random16(NUM_LEDS);
    if (leds[pos].r == 0) { // Si está apagado o casi apagado
      // Color Blanco Cálido
      leds[pos] = CHSV(45, 80, 255); 
    }
  }
}

// --- FIRE2012 ORIGINAL ---
// Parámetros originales del archivo adjunto
#define COOLING  55
#define SPARKING 120

void fire2012Original() {
  static byte heat[NUM_LEDS];

  // 1. Cool down
  for( int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // 2. Heat drifts up
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }
  
  // 3. Sparks
  if( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160,255) );
  }

  // 4. Map to color
  for( int j = 0; j < NUM_LEDS; j++) {
      leds[j] = HeatColor( heat[j]);
  }
}

// --- NIEVE ACUMULADA ---
void nieveAcumulada() {
  static int pixelPos = NUM_LEDS - 1; 
  static int pileHeight = 0;          
  static unsigned long lastMove = 0;
  
  // Dibuja la nieve acumulada
  for(int i = 0; i < pileHeight; i++) leds[i] = CRGB::Grey; 
  
  // Lógica de caída
  if (millis() - lastMove > 100) { 
    lastMove = millis();
    if (pixelPos < NUM_LEDS) leds[pixelPos] = CRGB::Black;
    pixelPos--; 
    if (pixelPos < pileHeight) { 
      pileHeight++; 
      pixelPos = NUM_LEDS - 1; 
      if (pileHeight >= NUM_LEDS) { 
        fill_solid(leds, NUM_LEDS, CRGB::White);
        FastLED.show(); delay(200); pileHeight = 0; FastLED.clear();
      }
    }
  }
  if (pixelPos >= pileHeight && pixelPos < NUM_LEDS) leds[pixelPos] = CRGB::White; 
}

// --- FUEGO MODIFICADO (Motor compartido para chimeneas) ---
void runFireMod(bool isIce) {
  static byte heat[NUM_LEDS];
  for( int i = 0; i < NUM_LEDS; i++) heat[i] = qsub8( heat[i],  random8(0, ((55 * 10) / NUM_LEDS) + 2));
  for( int k = NUM_LEDS - 1; k >= 2; k--) heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  if( random8() < 120 ) { int y = random8(7); heat[y] = qadd8( heat[y], random8(160,255) ); }
  for( int j = 0; j < NUM_LEDS; j++) {
    if (isIce) leds[j] = ColorFromPalette(icePalette, heat[j]);
    else leds[j] = HeatColor(heat[j]);
  }
}

void chimeneaClasica() { runFireMod(false); }
void chimeneaHielo() { runFireMod(true); }

// --- ÁRBOL QUE RESPIRA ---
void arbolQueRespira() {
  uint8_t brightness = beatsin8(12, 60, 255); 
  fill_solid(leds, NUM_LEDS, CHSV(42, 220, brightness));
}

// --- SETUP PALETAS ---
void setupAuroraPalette() {
  auroraPalette = CRGBPalette16(
    CRGB::Black, CRGB::Green, CRGB::Teal, CRGB::Purple,
    CRGB::Magenta, CRGB::Green, CRGB::Black, CRGB::Blue,
    CRGB::Teal, CRGB::Green, CRGB::Magenta, CRGB::Black,
    CRGB::Purple, CRGB::Teal, CRGB::Green, CRGB::Black
  );
}

void setupIcePalette() {
  icePalette = CRGBPalette16(
    CRGB::Black,  CRGB::Blue,   CRGB::Cyan,  CRGB::White,
    CRGB::Black,  CRGB::Blue,   CRGB::Cyan,  CRGB::White,
    CRGB::Black,  CRGB::Navy,   CRGB::SkyBlue,  CRGB::White,
    CRGB::Black,  CRGB::Blue,   CRGB::Aqua,  CRGB::White
  );
}
