#define touch_up 32
#define touch_select 33
#define touch_down 27
#define touch_aux 15
#define touchThreshold 70  // Ajustado para ser menos sensible

void initTouch(){
  // Configuración inicial, si es necesario
}

// Función para realizar una lectura estable del sensor táctil
int stableTouchRead(int pin) {
    const int numReadings = 100;
    int readings[numReadings];
    int total = 0;
    for (int i = 0; i < numReadings; i++) {
        readings[i] = touchRead(pin);
        total += readings[i];      
    }
    delay(10);
    return total / numReadings;
}

int loopTouch(){
    auto touch_upraw = stableTouchRead(touch_up);
    auto touch_selectraw = stableTouchRead(touch_select);
    auto touch_downraw = stableTouchRead(touch_down);
    auto touch_auxraw = stableTouchRead(touch_aux);
    
    if(touch_upraw < touchThreshold ) {
        Serial.println("up detectado");
        return 1;
    }
    if(touch_selectraw < touchThreshold ) {
        Serial.println("select detectado");
        return 2;
    }
    if(touch_downraw < touchThreshold ) {
        Serial.println("down detectado");
        return 3;
    }
    if(touch_auxraw < touchThreshold ) {
        Serial.println("aux detectado");
        return 4;
    }      
    return 0;
}
