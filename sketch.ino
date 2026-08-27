
<<<<<<< HEAD
  for (int i = 1; i < NUM_CELLS; i++) {
>>>>>>> 4493f2dac32173dff88ea92d18c96efc263c8e84
=======
  for (int i = 0; i < NUM_CELLS; i++) {
>>>>>>> 7187336 (Updated)
    if (battery.voltage[i] < battery.lowestVoltage) {
      battery.lowestVoltage = battery.voltage[i];
      battery.weakestCell = i;
    }
    if (battery.voltage[i] > battery.highestVoltage) {
      battery.highestVoltage = battery.voltage[i];
      battery.strongestCell = i;
    }
<<<<<<< HEAD
<<<<<<< HEAD
    if (battery.voltage[i] < MIN_CELL_VOLTAGE || battery.voltage[i] > MAX_CELL_VOLTAGE) {
      battery.rangeFault = true;
    }
  }
  
  float currentImbalance = battery.highestVoltage - battery.lowestVoltage;
  battery.imbalanceDerivative = (currentImbalance - battery.imbalance) / ((float)BMS_INTERVAL / 1000.0f);
  battery.imbalance = currentImbalance;
=======
  }
  battery.imbalance = battery.highestVoltage - battery.lowestVoltage;
>>>>>>> 4493f2dac32173dff88ea92d18c96efc263c8e84
=======
    if (battery.voltage[i] < MIN_CELL_VOLTAGE || battery.voltage[i] > MAX_CELL_VOLTAGE) {
      battery.rangeFault = true;
    }
  }
  
  float currentImbalance = battery.highestVoltage - battery.lowestVoltage;
  battery.imbalanceDerivative = (currentImbalance - battery.imbalance) / ((float)BMS_INTERVAL / 1000.0f);
  battery.imbalance = currentImbalance;
>>>>>>> 7187336 (Updated)
}

void calculateSOC() {
  float avg = 0;
  for (int i = 0; i < NUM_CELLS; i++) avg += battery.voltage[i];
  avg /= NUM_CELLS;
<<<<<<< HEAD
<<<<<<< HEAD

  battery.soc = ((avg - MIN_CELL_VOLTAGE) / (MAX_CELL_VOLTAGE - MIN_CELL_VOLTAGE)) * 100.0f;
  battery.soc = constrain(battery.soc, 0.0f, 100.0f);
}

void calculateAdaptiveThreshold() {
  float socThreshold = (battery.soc > 80.0f) ? 0.05f : ((battery.soc > 50.0f) ? 0.08f : 0.12f);
  float cRateFactor = (battery.cRate > 2.0f) ? 1.5f : 1.0f;
  battery.threshold = socThreshold * cRateFactor;
  battery.imbalanceFault = (battery.imbalance > battery.threshold);
}

void detectTrend() {
  battery.smoothedImbalance = (0.3f * battery.imbalance) + (0.7f * battery.previousImbalance);
  float deadband = 0.004f;

  if (battery.smoothedImbalance > battery.previousImbalance + deadband) {
    battery.trend = TREND_INCREASING;
  } else if (battery.smoothedImbalance < battery.previousImbalance - deadband) {
    battery.trend = TREND_DECREASING;
  } else {
    battery.trend = TREND_STABLE;
  }
  battery.previousImbalance = battery.smoothedImbalance;
}

// ==================== TASK 3: FLICKER-FREE LCD ENGINE ====================
void updateLcdEngine() {
  unsigned long now = millis();

  // Fault Screen Override
  if (currentState == STATE_FAILSAFE || currentState == STATE_SHUTDOWN) {
    if (lastRenderedPage != (LcdPage)99) {
      lcd.clear();
      lastRenderedPage = (LcdPage)99;
    }
    renderFaultScreen();
    return;
  }

  // Page Rotation Timer
  if (now - lastPageRotate >= PAGE_ROTATE_INTERVAL) {
    lastPageRotate = now;
    currentPage = (LcdPage)((currentPage + 1) % 3);
  }

  if (currentPage != lastRenderedPage) {
    lcd.clear();
    lastRenderedPage = currentPage;
  }

  switch (currentPage) {
    case PAGE_BATTERY_STATUS: renderPageBatteryStatus(); break;
    case PAGE_SYSTEM_STATE:   renderPageSystemState(); break;
    case PAGE_TELEMETRY:      renderPageTelemetry(); break;
  }
}

void renderPageBatteryStatus() {
  lcd.setCursor(0, 0);
  lcd.print("SOC:"); lcd.print(battery.soc, 1); lcd.print("%   ");
  lcd.setCursor(0, 1);
  lcd.print("Imb:"); lcd.print(battery.imbalance, 2); lcd.print("V ");
  switch (battery.trend) {
    case TREND_INCREASING: lcd.print("TR:UP"); break;
    case TREND_DECREASING: lcd.print("TR:DN"); break;
    case TREND_STABLE:     lcd.print("TR:ST"); break;
  }
}

void renderPageSystemState() {
  lcd.setCursor(0, 0);
  lcd.print("SYS:"); lcd.print(stateToString(currentState)); lcd.print("  ");
  lcd.setCursor(0, 1);
  lcd.print("RLY:");
  lcd.print(digitalRead(RELAY_PIN) == HIGH ? "ON " : "OFF");
  lcd.print(" FLT:"); lcd.print((int)activeFault);
}

void renderPageTelemetry() {
  lcd.setCursor(0, 0);
  lcd.print("CUR:"); lcd.print(battery.packCurrent, 1); lcd.print("A "); lcd.print(battery.cRate, 1); lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("W:"); lcd.print(battery.weakestCell + 1); lcd.print(" S:"); lcd.print(battery.strongestCell + 1); lcd.print(" D:"); lcd.print(battery.imbalance, 2);
}

void renderFaultScreen() {
  lcd.setCursor(0, 0);
  if (currentState == STATE_SHUTDOWN) lcd.print("!! SHUTDOWN !!  ");
  else lcd.print("!! FAILSAFE !!  ");

  lcd.setCursor(0, 1);
  switch (activeFault) {
    case FAULT_CELL_IMBALANCE:    lcd.print("CELL IMBALANCE "); break;
    case FAULT_CELL_OUT_OF_RANGE: lcd.print("CELL OUT OF RNG"); break;
    case FAULT_ADC_FROZEN:        lcd.print("ADC SENSOR FROZ"); break;
    case FAULT_ADC_JUMP:          lcd.print("ADC STEP JUMP  "); break;
    case FAULT_RELAY_MISMATCH:    lcd.print("RELAY MISMATCH "); break;
    default:                      lcd.print("OVERVOLT TRIP  "); break;
  }
}
=======
  battery.soc = ((avg - MIN_CELL_VOLTAGE) / (MAX_CELL_VOLTAGE - MIN_CELL_VOLTAGE)) * 100.0;
  battery.soc = constrain(battery.soc, 0, 100);
=======

  battery.soc = ((avg - MIN_CELL_VOLTAGE) / (MAX_CELL_VOLTAGE - MIN_CELL_VOLTAGE)) * 100.0f;
  battery.soc = constrain(battery.soc, 0.0f, 100.0f);
>>>>>>> 7187336 (Updated)
}

void calculateAdaptiveThreshold() {
  float socThreshold = (battery.soc > 80.0f) ? 0.05f : ((battery.soc > 50.0f) ? 0.08f : 0.12f);
  float cRateFactor = (battery.cRate > 2.0f) ? 1.5f : 1.0f;
  battery.threshold = socThreshold * cRateFactor;
  battery.imbalanceFault = (battery.imbalance > battery.threshold);
}

void detectTrend() {
  battery.smoothedImbalance = (0.3f * battery.imbalance) + (0.7f * battery.previousImbalance);
  float deadband = 0.004f;

  if (battery.smoothedImbalance > battery.previousImbalance + deadband) {
    battery.trend = TREND_INCREASING;
  } else if (battery.smoothedImbalance < battery.previousImbalance - deadband) {
    battery.trend = TREND_DECREASING;
  } else {
    battery.trend = TREND_STABLE;
  }
  battery.previousImbalance = battery.smoothedImbalance;
}

// ==================== TASK 3: FLICKER-FREE LCD ENGINE ====================
void updateLcdEngine() {
  unsigned long now = millis();

  if (currentState == STATE_FAILSAFE || currentState == STATE_SHUTDOWN) {
    if (lastRenderedPage != (LcdPage)99) {
      lcd.clear();
      lastRenderedPage = (LcdPage)99;
    }
    renderFaultScreen();
    return;
  }

  if (now - lastPageRotate >= PAGE_ROTATE_INTERVAL) {
    lastPageRotate = now;
    currentPage = (LcdPage)((currentPage + 1) % 3);
  }

  if (currentPage != lastRenderedPage) {
    lcd.clear();
    lastRenderedPage = currentPage;
  }

  switch (currentPage) {
    case PAGE_BATTERY_STATUS: renderPageBatteryStatus(); break;
    case PAGE_SYSTEM_STATE:   renderPageSystemState(); break;
    case PAGE_TELEMETRY:      renderPageTelemetry(); break;
  }
}

void renderPageBatteryStatus() {
  lcd.setCursor(0, 0);
  lcd.print("SOC:"); lcd.print(battery.soc, 1); lcd.print("%   ");
  lcd.setCursor(0, 1);
  lcd.print("Imb:"); lcd.print(battery.imbalance, 2); lcd.print("V ");
  switch (battery.trend) {
    case TREND_INCREASING: lcd.print("TR:UP"); break;
    case TREND_DECREASING: lcd.print("TR:DN"); break;
    case TREND_STABLE:     lcd.print("TR:ST"); break;
  }
}

void renderPageSystemState() {
  lcd.setCursor(0, 0);
  lcd.print("SYS:"); lcd.print(stateToString(currentState)); lcd.print("  ");
  lcd.setCursor(0, 1);
  lcd.print("RLY:");
  lcd.print(digitalRead(RELAY_PIN) == HIGH ? "ON " : "OFF");
  lcd.print(" FLT:"); lcd.print((int)activeFault);
}

<<<<<<< HEAD
BatteryInfo getBatteryInfo() { return battery; }
RelayState  getCellState(int i) { return currentState[i]; }
FaultType   getCellFault(int i) { return activeFault[i]; }
>>>>>>> 4493f2dac32173dff88ea92d18c96efc263c8e84
=======
void renderPageTelemetry() {
  lcd.setCursor(0, 0);
  lcd.print("CUR:"); lcd.print(battery.packCurrent, 1); lcd.print("A "); lcd.print(battery.cRate, 1); lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("W:"); lcd.print(battery.weakestCell + 1); lcd.print(" S:"); lcd.print(battery.strongestCell + 1); lcd.print(" D:"); lcd.print(battery.imbalance, 2);
}

void renderFaultScreen() {
  lcd.setCursor(0, 0);
  if (currentState == STATE_SHUTDOWN) lcd.print("!! SHUTDOWN !!  ");
  else lcd.print("!! FAILSAFE !!  ");

  lcd.setCursor(0, 1);
  switch (activeFault) {
    case FAULT_CELL_IMBALANCE:    lcd.print("CELL IMBALANCE "); break;
    case FAULT_CELL_OUT_OF_RANGE: lcd.print("CELL OUT OF RNG"); break;
    case FAULT_ADC_FROZEN:        lcd.print("ADC SENSOR FROZ"); break;
    case FAULT_ADC_JUMP:          lcd.print("ADC STEP JUMP  "); break;
    case FAULT_RELAY_MISMATCH:    lcd.print("RELAY MISMATCH "); break;
    default:                      lcd.print("OVERVOLT TRIP  "); break;
  }
}

