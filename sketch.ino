#define NUM_CELLS 4

const int cellPins[NUM_CELLS] = {34, 35, 32, 33};   
const int RELAY_PIN = 25;                            


const float MIN_CELL_VOLTAGE = 3.0;
const float MAX_CELL_VOLTAGE = 4.2;

// ---------------- Task 2: hysteresis trip/clear per cell ----------------
const float LOW_TRIP    = 3.05;
const float LOW_CLEAR   = 3.15;
const float HIGH_TRIP   = 4.15;
const float HIGH_CLEAR  = 4.10;

const unsigned long DEBOUNCE_TIME      = 300;
const unsigned long RECOVERY_HOLD_TIME = 5000;
const unsigned long SAMPLE_INTERVAL    = 100;

const float FROZEN_TOLERANCE   = 0.001;
const int   FROZEN_COUNT_LIMIT = 15;
const float MAX_REALISTIC_JUMP = 0.6;
const int   WINDOW_SIZE        = 5;
const float NOISE_STDDEV_MULT  = 3.0;

// ===================================================================
// TASK 1: Battery analysis structure
// ===================================================================
enum TrendType { TREND_INCREASING, TREND_DECREASING, TREND_STABLE };

struct BatteryInfo {
  float voltage[NUM_CELLS];
  int   weakestCell;
  int   strongestCell;
  float lowestVoltage;
  float highestVoltage;
  float imbalance;
  float previousImbalance;
  TrendType trend;
  float soc;
  float threshold;
  bool  imbalanceFault;
};
BatteryInfo battery;

// ===================================================================
// TASK 2: Per-cell protection state machine + single shared relay
// ===================================================================
enum RelayState { STATE_NORMAL, STATE_FAULT, STATE_RECOVERY };
enum FaultType {
  FAULT_NONE, FAULT_LOW_VOLTAGE, FAULT_HIGH_VOLTAGE,
  FAULT_SENSOR_FROZEN, FAULT_SENSOR_JUMP, FAULT_SENSOR_RANGE
};

RelayState currentState[NUM_CELLS];
FaultType  activeFault[NUM_CELLS];
unsigned long stateEnteredTime[NUM_CELLS];
unsigned long conditionStartTime[NUM_CELLS];
bool conditionPending[NUM_CELLS];

float voltageWindow[NUM_CELLS][WINDOW_SIZE];
int   windowIndex[NUM_CELLS];
bool  windowFilled[NUM_CELLS];
float lastVoltage[NUM_CELLS];
int   frozenCounter[NUM_CELLS];

unsigned long lastSampleTime = 0;

// ===================================================================
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);   // relay closed = normal operation

  for (int i = 0; i < NUM_CELLS; i++) {
    currentState[i] = STATE_NORMAL;
    activeFault[i]  = FAULT_NONE;
    stateEnteredTime[i] = millis();
    conditionPending[i] = false;
    lastVoltage[i] = -1;
    frozenCounter[i] = 0;
    windowIndex[i] = 0;
    windowFilled[i] = false;
    for (int w = 0; w < WINDOW_SIZE; w++) voltageWindow[i][w] = 0;
  }

  battery.previousImbalance = 0;

  Serial.println("\n===== Combined BMS Engine + Protection Relay =====");
  logTransition(-1, "BOOT", "NORMAL");
}

// ===================================================================
void loop() {
  unsigned long now = millis();

  if (now - lastSampleTime >= SAMPLE_INTERVAL) {
    lastSampleTime = now;

    readVoltages();
    analyzeCells();
    calculateSOC();
    calculateAdaptiveThreshold();
    detectTrend();

    for (int i = 0; i < NUM_CELLS; i++) {
      FaultType detected = evaluateSensorHealth(i, battery.voltage[i]);
      if (detected == FAULT_NONE) {
        detected = evaluateBatteryLimits(i, battery.voltage[i]);
      }
      updateStateMachine(i, detected, now, battery.voltage[i]);
    }

    applyRelayOutput();     // single relay reacts to ANY cell fault
    printCombinedStatus();
  }
}

// ===================================================================
// TASK 1 FUNCTIONS
// ===================================================================
void readVoltages() {
  for (int i = 0; i < NUM_CELLS; i++) {
    int adc = analogRead(cellPins[i]);
    battery.voltage[i] = MIN_CELL_VOLTAGE +
        ((float)adc / 4095.0) * (MAX_CELL_VOLTAGE - MIN_CELL_VOLTAGE);
  }
}

void analyzeCells() {
  battery.lowestVoltage = battery.voltage[0];
  battery.highestVoltage = battery.voltage[0];
  battery.weakestCell = 0;
  battery.strongestCell = 0;

  for (int i = 1; i < NUM_CELLS; i++) {
    if (battery.voltage[i] < battery.lowestVoltage) {
      battery.lowestVoltage = battery.voltage[i];
      battery.weakestCell = i;
    }
    if (battery.voltage[i] > battery.highestVoltage) {
      battery.highestVoltage = battery.voltage[i];
      battery.strongestCell = i;
    }
  }
  battery.imbalance = battery.highestVoltage - battery.lowestVoltage;
}

void calculateSOC() {
  float avg = 0;
  for (int i = 0; i < NUM_CELLS; i++) avg += battery.voltage[i];
  avg /= NUM_CELLS;
  battery.soc = ((avg - MIN_CELL_VOLTAGE) / (MAX_CELL_VOLTAGE - MIN_CELL_VOLTAGE)) * 100.0;
  battery.soc = constrain(battery.soc, 0, 100);
}

void calculateAdaptiveThreshold() {
  if (battery.soc > 80) battery.threshold = 0.05;
  else if (battery.soc > 50) battery.threshold = 0.08;
  else if (battery.soc > 20) battery.threshold = 0.12;
  else battery.threshold = 0.18;
  battery.imbalanceFault = battery.imbalance > battery.threshold;
}

void detectTrend() {
  float tolerance = 0.003;
  if (battery.imbalance > battery.previousImbalance + tolerance) battery.trend = TREND_INCREASING;
  else if (battery.imbalance < battery.previousImbalance - tolerance) battery.trend = TREND_DECREASING;
  else battery.trend = TREND_STABLE;
  battery.previousImbalance = battery.imbalance;
}

// ===================================================================
// TASK 2 FUNCTIONS (per-cell)
// ===================================================================
FaultType evaluateSensorHealth(int i, float voltage) {
  if (voltage < 0.0 || voltage > 5.0) return FAULT_SENSOR_RANGE;

  if (lastVoltage[i] >= 0 && fabs(voltage - lastVoltage[i]) < FROZEN_TOLERANCE) {
    frozenCounter[i]++;
  } else {
    frozenCounter[i] = 0;
  }
  if (frozenCounter[i] >= FROZEN_COUNT_LIMIT) return FAULT_SENSOR_FROZEN;

  if (lastVoltage[i] >= 0 && fabs(voltage - lastVoltage[i]) > MAX_REALISTIC_JUMP) {
    int count = windowFilled[i] ? WINDOW_SIZE : windowIndex[i];
    if (count > 1) {
      float mean = 0, variance = 0;
      for (int w = 0; w < count; w++) mean += voltageWindow[i][w];
      mean /= count;
      for (int w = 0; w < count; w++) variance += pow(voltageWindow[i][w] - mean, 2);
      variance /= count;
      float stddev = sqrt(variance);
      float deviation = fabs(voltage - mean);

      if (deviation > NOISE_STDDEV_MULT * stddev && stddev > 0.0005) {
        lastVoltage[i] = voltage;
        return FAULT_SENSOR_JUMP;
      }
    } else {
      lastVoltage[i] = voltage;
      return FAULT_SENSOR_JUMP;
    }
  }

  voltageWindow[i][windowIndex[i]] = voltage;
  windowIndex[i] = (windowIndex[i] + 1) % WINDOW_SIZE;
  if (windowIndex[i] == 0) windowFilled[i] = true;

  lastVoltage[i] = voltage;
  return FAULT_NONE;
}

FaultType evaluateBatteryLimits(int i, float voltage) {
  if (voltage < LOW_TRIP)  return FAULT_LOW_VOLTAGE;
  if (voltage > HIGH_TRIP) return FAULT_HIGH_VOLTAGE;
  return FAULT_NONE;
}

bool isConditionCleared(int i, float voltage) {
  if (activeFault[i] == FAULT_LOW_VOLTAGE)  return voltage > LOW_CLEAR;
  if (activeFault[i] == FAULT_HIGH_VOLTAGE) return voltage < HIGH_CLEAR;
  return true;
}

void updateStateMachine(int i, FaultType detected, unsigned long now, float voltage) {
  switch (currentState[i]) {
    case STATE_NORMAL:
      if (detected != FAULT_NONE) {
        if (!conditionPending[i]) { conditionPending[i] = true; conditionStartTime[i] = now; }
        if (now - conditionStartTime[i] >= DEBOUNCE_TIME) {
          activeFault[i] = detected;
          currentState[i] = STATE_FAULT;
          stateEnteredTime[i] = now;
          conditionPending[i] = false;
          logTransition(i, "NORMAL", "FAULT");
        }
      } else {
        conditionPending[i] = false;
      }
      break;

    case STATE_FAULT:
      if (detected == FAULT_NONE && isConditionCleared(i, voltage)) {
        if (!conditionPending[i]) { conditionPending[i] = true; conditionStartTime[i] = now; }
        if (now - conditionStartTime[i] >= DEBOUNCE_TIME) {
          currentState[i] = STATE_RECOVERY;
          stateEnteredTime[i] = now;
          conditionPending[i] = false;
          logTransition(i, "FAULT", "RECOVERY");
        }
      } else {
        conditionPending[i] = false;
        if (detected != FAULT_NONE) activeFault[i] = detected;
      }
      break;

    case STATE_RECOVERY:
      if (detected != FAULT_NONE || !isConditionCleared(i, voltage)) {
        activeFault[i] = (detected != FAULT_NONE) ? detected : activeFault[i];
        currentState[i] = STATE_FAULT;
        stateEnteredTime[i] = now;
        logTransition(i, "RECOVERY", "FAULT (relapse)");
      } else if (now - stateEnteredTime[i] >= RECOVERY_HOLD_TIME) {
        activeFault[i] = FAULT_NONE;
        currentState[i] = STATE_NORMAL;
        stateEnteredTime[i] = now;
        logTransition(i, "RECOVERY", "NORMAL");
      }
      break;
  }
}


bool anyCellFaulted() {
  for (int i = 0; i < NUM_CELLS; i++) if (currentState[i] == STATE_FAULT) return true;
  return false;
}

void applyRelayOutput() {
  bool packSafe = !anyCellFaulted();
  digitalWrite(RELAY_PIN, packSafe ? HIGH : LOW);
}

// ===================================================================
void logTransition(int i, const char* from, const char* to) {
  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] ");
  if (i >= 0) { Serial.print("Cell "); Serial.print(i + 1); Serial.print(": "); }
  Serial.print(from);
  Serial.print(" -> ");
  Serial.println(to);
}

void printCombinedStatus() {
  Serial.println("-----------------------------------------------");
  for (int i = 0; i < NUM_CELLS; i++) {
    Serial.print("Cell "); Serial.print(i + 1);
    Serial.print(": "); Serial.print(battery.voltage[i], 3);
    Serial.print("V | State: ");
    Serial.println(currentState[i] == STATE_NORMAL ? "NORMAL" :
                    currentState[i] == STATE_FAULT ? "FAULT" : "RECOVERY");
  }
  Serial.print("Weakest: Cell "); Serial.print(battery.weakestCell + 1);
  Serial.print(" | Strongest: Cell "); Serial.print(battery.strongestCell + 1);
  Serial.print(" | Imbalance: "); Serial.print(battery.imbalance, 3);
  Serial.print("V | SoC: "); Serial.print(battery.soc, 1);
  Serial.print("% | Trend: ");
  Serial.print(battery.trend == TREND_INCREASING ? "Increasing" :
               battery.trend == TREND_DECREASING ? "Decreasing" : "Stable");
  Serial.print(" | Relay: ");
  Serial.println(digitalRead(RELAY_PIN) ? "CLOSED" : "OPEN");
}


BatteryInfo getBatteryInfo() { return battery; }
RelayState  getCellState(int i) { return currentState[i]; }
FaultType   getCellFault(int i) { return activeFault[i]; }