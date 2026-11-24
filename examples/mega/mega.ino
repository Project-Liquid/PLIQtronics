#include "main.h"

bool print_data = true;
unsigned long start_time = 0; 
const int runtime = -1;
TransmissionType transmissionFormat = COMPRESSED;
unsigned long time_absolute = 0;
unsigned long time_elapsed = 0;
bool static_fire_initializing = false;
unsigned long static_fire_start_ms = 0;
bool static_fire_steady = false;
unsigned long static_fire_duration_ms = 0;
bool heaters_active = false;
float ETHANE_TARGET_PRESSURE = 0;
float NITROUS_TARGET_PRESSURE = 0;
PollInterval PTLog{50, 0};
PollInterval LCLog{200, 0};
PollInterval ValveLog{200, 0};
PollInterval MBVLog{50, 0};
PollInterval TCLog{200, 0};
PollInterval RedlinePoll{50, 0};
PollInterval ValveSchedulePoll{50, 0};
PollInterval HeaterPoll{50, 0};
float ETHANE_WEIGHT_REDLINE = -27;
float NITROUS_WEIGHT_REDLINE = 19.5;
int current_highest_redline = 0;
int last_highest_redline = 0;
String cmd = "";
//======================OBJECT DEFNINTIONS=======================//

// Pressure Transducers
Transducer EthaneUpstreamPT(ETHANE_UPSTREAM_PIN, P_MIN, P_MAX_ETHANE); // 42 bytes
Transducer EthaneDownstreamPT(ETHANE_DOWNSTREAM_PIN, P_MIN, P_MAX_ETHANE);
Transducer NitrousUpstreamPT(NITROUS_UPSTREAM_PIN, P_MIN, P_MAX_NITROUS);
Transducer NitrousDownstreamPT(NITROUS_DOWNSTREAM_PIN, P_MIN, P_MAX_NITROUS);
Transducer ReroutePT(REROUTE_PT_PIN, P_MIN, P_MAX_NITROUS);
Transducer ChamberPT(CHAMBER_PT_PIN, P_MIN, P_MAX_CHAMBER);
Relay EthaneRunValve(ETHANE_RUN_PIN); // 60
Relay EthaneVent(ETHANE_VENT_PIN);
Relay NitrousRunValve(NITROUS_RUN_PIN);
Relay NitrousVent(NITROUS_VENT_PIN);
Relay Ignitor(IGNITOR_PIN);
MBV EthaneMBV(ETHANE_MBV_PINS, ETHANE_LIMIT_PINS); // 69
MBV NitrousMBV(NITROUS_MBV_PINS, NITROUS_LIMIT_PINS);
SerialDualClass SerialDual(Serial, Serial2); // 10
LoadCell EthaneLC1;
LoadCell EthaneLC2;
LoadCell EthaneLC3;
LoadCell NitrousLC1;
LoadCell NitrousLC2;
LoadCell NitrousLC3;
ThrustCell ThrustLC;
Heater EthaneHeater1(ETHANE_HEATER_1_PIN, &EthaneUpstreamPT);
Heater EthaneHeater2(ETHANE_HEATER_2_PIN, &EthaneDownstreamPT);
Heater NitrousHeater1(NITROUS_HEATER_1_PIN, &NitrousUpstreamPT);
Heater NitrousHeater2(NITROUS_HEATER_2_PIN, &NitrousDownstreamPT);
Thermocouple RerouteTC(REROUTE_TC_ADDRESS);
Thermocouple ChamberTC(CHAMBER_TC_ADDRESS);
Thermocouple RerouteTC3(0x67);
//ADS1118* ChamberTC = nullptr;
CmdBuffer serialBuf; // 9
CmdBuffer serial2Buf;

/** TODO: MAKE ARRAY INDEXED BY PRIORITY */
Redline Redlines[NUM_REDLINES] = {
  Redline(EthaneOverpressureCondition, EthaneOverpressureResponse, ETHANE_OVERPRESSURE_PRIORITY, OVERPRESSURE_COUNTS_THRESHOLD),
  Redline(NitrousOverpressureCondition, NitrousOverpressureResponse, NITROUS_OVERPRESSURE_PRIORITY, OVERPRESSURE_COUNTS_THRESHOLD),
  Redline(EthaneMBVCloseFailureCondition, EthaneMBVCloseFailureResponse, ETHANE_MBV_CLOSE_FAILURE_PRIORITY, MBV_FAILURE_COUNTS_THRESHOLD),
  Redline(NitrousMBVCloseFailureCondition, NitrousMBVCloseFailureResponse, NITROUS_MBV_CLOSE_FAILURE_PRIORITY, MBV_FAILURE_COUNTS_THRESHOLD),
  Redline(CombustionPropogationCondition, CombustionPropogationResponse, COMBUSTION_PROPOGATION_PRIORITY, COMBUSTION_PROPOGATION_COUNTS_THRESHOLD),
  Redline(InlineThermalDecompCondition, InlineThermalDecompResponse, INLINE_THERMAL_DECOMP_PRIORITY, INLINE_THERMAL_DECOMP_COUNTS_THRESHOLD),
  Redline(LostLoadCellCondition, LostLoadCellResponse, LOST_LOAD_CELL_PRIORITY, LOST_LOAD_CELL_COUNTS_THRESHOLD),
  Redline(EthaneUnderweightCondition, EthaneUnderweightResponse, ETHANE_UNDERWEIGHT_PRIORITY, UNDERWEIGHT_COUNTS_THRESHOLD),
  Redline(NitrousUnderweightCondition, NitrousUnderweightResponse, NITROUS_UNDERWEIGHT_PRIORITY, UNDERWEIGHT_COUNTS_THRESHOLD),
  Redline(EthaneOverweightCondition, EthaneOverweightResponse, ETHANE_OVERWEIGHT_PRIORITY, OVERWEIGHT_COUNTS_THRESHOLD),
  Redline(NitrousOverweightCondition, NitrousOverweightResponse, NITROUS_OVERWEIGHT_PRIORITY, OVERWEIGHT_COUNTS_THRESHOLD)
};

//===========================FUNCTIONS============================//
/** MARK: Helpers */

/**
 * Our output function.
 * Note: Some features commented out to speed running.
 */
void status(TransmissionType format = COMPRESSED) {
  time_elapsed = millis()-start_time;
  bool print_current_poll = false;

  switch (format) {
    case DIAGNOSTIC: {
      SerialDual.println("====================");
      // Time
      SerialDual.print(time_elapsed/1000.0); SerialDual.print("s");
      // PTs
      SerialDual.print("Ethane Upstream: \t");  SerialDual.print(EthaneUpstreamPT.status());
      SerialDual.print("Ethane Downstream: \t");  SerialDual.print(EthaneDownstreamPT.status());
      SerialDual.print("Nitrous Upstream: \t");  SerialDual.print(NitrousUpstreamPT.status());
      SerialDual.print("Nitrous Downstream: \t");  SerialDual.print(NitrousDownstreamPT.status());
      SerialDual.print("Reroute: \t");  SerialDual.print(ReroutePT.status());
      // Load Cells
      SerialDual.print("Ethane: \t");
      SerialDual.print("LC1: ");  SerialDual.print(EthaneLC1.read()); 
      SerialDual.print("\tLC2: ");  SerialDual.print(EthaneLC2.read());
      SerialDual.print("\tLC3: ");  SerialDual.print(EthaneLC3.read()); 
      SerialDual.println();
      SerialDual.print("Nitrous: \t");
      SerialDual.print("LC1: ");  SerialDual.print(NitrousLC1.read()); 
      SerialDual.print("\tLC2: ");  SerialDual.print(NitrousLC2.read()); 
      SerialDual.print("\tLC3: ");  SerialDual.print(NitrousLC3.read()); 
      SerialDual.println();
      SerialDual.print("Thrust: "); SerialDual.print(ThrustLC.read());
      SerialDual.println();
      // Tank Heaters
      SerialDual.print("Ethane Tank 1: "); SerialDual.print(EthaneHeater1.isOn() ? "ON" : "OFF");
      SerialDual.print("Nitrous Tank 1: "); SerialDual.print(NitrousHeater1.isOn() ? "ON" : "OFF");
      // Thermocouple
      // SerialDual.println("Reroute TC: ");  SerialDual.print(RerouteTC.readHot());  SerialDual.print("F");
      // SerialDual.println("Reroute TC2: ");  SerialDual.print(RerouteTC2.readHot());  SerialDual.print("F");
      // SerialDual.println("Reroute TC3: ");  SerialDual.print(RerouteTC3.readHot());  SerialDual.print("F");
      // MBVs
      SerialDual.println("Ethane MBV: ");  SerialDual.print(EthaneMBV.isOpen() ? "OPEN" : "CLOSED");
      SerialDual.println("Nitrous MBV: ");  SerialDual.print(NitrousMBV.isOpen() ? "OPEN" : "CLOSED");
      break;
    }

    case COMPRESSED: {
      // FORMAT: DATA|millis|KEY:VAL|KEY:VAL|...
      if(time_absolute - PTLog.last_trigger_ms >= PTLog.interval_ms) {
        SerialDual.print("DATA|"); SerialDual.print((time_elapsed/1000.0), 3);
        SerialDual.print("|PT_EU:");  SerialDual.print(EthaneUpstreamPT.readPressure(), 3);
        SerialDual.print("|PT_ED:");  SerialDual.print(EthaneDownstreamPT.readPressure(), 3);
        SerialDual.print("|PT_NU:");  SerialDual.print(NitrousUpstreamPT.readPressure(), 3);
        SerialDual.print("|PT_ND:");  SerialDual.print(NitrousDownstreamPT.readPressure(), 3);
        SerialDual.print("|PT_RR:");  SerialDual.print(ReroutePT.readPressure(), 3);
        SerialDual.print("|PT_CH:");  SerialDual.print(ChamberPT.readPressure(), 3);
        PTLog.last_trigger_ms = time_absolute - (time_absolute % PTLog.interval_ms);
        print_current_poll = true;
      }
      
      if(time_absolute - LCLog.last_trigger_ms >= LCLog.interval_ms) {
        if (!print_current_poll) { SerialDual.print("DATA|"); SerialDual.print((time_elapsed/1000.0), 3); }
        float lc1= EthaneLC1.read();
        if (lc1 > 0.2 || EthaneLC2.read() < 0.3) {SerialDual.print("|LC_E1:");  SerialDual.print(lc1); }
        SerialDual.print("|LC_E2:");  SerialDual.print(EthaneLC2.read());
        SerialDual.print("|LC_E3:");  SerialDual.print(EthaneLC3.read());
        SerialDual.print("|LC_N1:");  SerialDual.print(NitrousLC1.read());
        SerialDual.print("|LC_N2:");  SerialDual.print(NitrousLC2.read());
        SerialDual.print("|LC_N3:");  SerialDual.print(NitrousLC3.read());
        SerialDual.print("|LC_T:");   SerialDual.print(ThrustLC.read());
        LCLog.last_trigger_ms = time_absolute - (time_absolute % LCLog.interval_ms);
        print_current_poll = true;
      }

      if(time_absolute - ValveLog.last_trigger_ms >= ValveLog.interval_ms) {
        if (!print_current_poll) { SerialDual.print("DATA|"); SerialDual.print((time_elapsed/1000.0), 3); }
        SerialDual.print("|ERV:");    SerialDual.print(EthaneRunValve.state());
        SerialDual.print("|EV:");     SerialDual.print(EthaneVent.state());
        SerialDual.print("|NRV:");    SerialDual.print(NitrousRunValve.state());
        SerialDual.print("|NV:");     SerialDual.print(NitrousVent.state());
        SerialDual.print("|MBV_E:");  SerialDual.print(EthaneMBV.isOpen());
        SerialDual.print("|MBV_N:");  SerialDual.print(NitrousMBV.isOpen());
        ValveLog.last_trigger_ms = time_absolute - (time_absolute % ValveLog.interval_ms);
        print_current_poll = true;
      }

      if(time_absolute - TCLog.last_trigger_ms >= TCLog.interval_ms) {
        if (!print_current_poll) { SerialDual.print("DATA|"); SerialDual.print((time_elapsed/1000.0), 3); }
        SerialDual.print("|TC_C:"); SerialDual.print(ChamberTC.readHot());
        SerialDual.print("|TC_R:"); SerialDual.print(RerouteTC.readHot());
        TCLog.last_trigger_ms = time_absolute - (time_absolute % TCLog.interval_ms);
        print_current_poll = true;
      }

      if(time_absolute - HeaterPoll.last_trigger_ms >= HeaterPoll.interval_ms){
        if (!print_current_poll) {SerialDual.print("DATA|"); SerialDual.print ((time_elapsed/1000.0),3);}
        SerialDual.print("|NH_1:"); SerialDual.print(NitrousHeater1.isOn());
        SerialDual.print("|NH_2:"); SerialDual.print(NitrousHeater2.isOn());
        SerialDual.print("|EH_1:"); SerialDual.print(EthaneHeater1.isOn());
        SerialDual.print("|EH_2:"); SerialDual.print(EthaneHeater2.isOn());
        HeaterPoll.last_trigger_ms = time_absolute - (time_absolute % HeaterPoll.interval_ms);
        print_current_poll = true;
      }

      if (print_current_poll) { SerialDual.println(); }
      break;
    }

    case RAW: {
      // FORMAT: DATA|millis|KEY:VAL|KEY:VAL|...
      if(time_absolute - PTLog.last_trigger_ms >= PTLog.interval_ms) {
        SerialDual.print("DATA|"); SerialDual.print((time_elapsed/1000.0), 3);
        SerialDual.print("|PT_EU:");  SerialDual.print(EthaneUpstreamPT.readPressure(), 3);
        SerialDual.print("|PT_ED:");  SerialDual.print(EthaneDownstreamPT.readPressure(), 3);
        SerialDual.print("|PT_NU:");  SerialDual.print(NitrousUpstreamPT.readPressure(), 3);
        SerialDual.print("|PT_ND:");  SerialDual.print(NitrousDownstreamPT.readPressure(), 3);
        PTLog.last_trigger_ms = time_absolute - (time_absolute % PTLog.interval_ms);
        print_current_poll = true;
      }
      
      if(time_absolute - LCLog.last_trigger_ms >= LCLog.interval_ms) {
        if (!print_current_poll) { SerialDual.print("DATA|"); SerialDual.print((time_elapsed/1000.0), 3); }
        SerialDual.print("|LC_E1:");  SerialDual.print(EthaneLC1.read());
        SerialDual.print("|LC_E2:");  SerialDual.print(EthaneLC2.read());
        SerialDual.print("|LC_E3:");  SerialDual.print(EthaneLC3.read());
        SerialDual.print("|LC_N1:");  SerialDual.print(NitrousLC1.read());
        SerialDual.print("|LC_N2:");  SerialDual.print(NitrousLC2.read());
        SerialDual.print("|LC_N3:");  SerialDual.print(NitrousLC3.read());
        SerialDual.print("|LC_T:");   SerialDual.print(ThrustLC.read());
        LCLog.last_trigger_ms = time_absolute - (time_absolute % LCLog.interval_ms);
        print_current_poll = true;
      }

      if(time_absolute - ValveLog.last_trigger_ms >= ValveLog.interval_ms) {
        if (!print_current_poll) { SerialDual.print("DATA|"); SerialDual.print((time_elapsed/1000.0), 3); }
        SerialDual.print("|ERV:");    SerialDual.print(EthaneRunValve.state());
        SerialDual.print("|EV:");     SerialDual.print(EthaneVent.state());
        SerialDual.print("|NRV:");    SerialDual.print(NitrousRunValve.state());
        SerialDual.print("|NV:");     SerialDual.print(NitrousVent.state());
        SerialDual.print("|MBV_E:");  SerialDual.print(EthaneMBV.isOpen());
        SerialDual.print("|MBV_N:");  SerialDual.print(NitrousMBV.isOpen());
        ValveLog.last_trigger_ms = time_absolute - (time_absolute % ValveLog.interval_ms);
        print_current_poll = true;
      }

      if(time_absolute - TCLog.last_trigger_ms >= TCLog.interval_ms) {
        if (!print_current_poll) { SerialDual.print("DATA|"); SerialDual.print((time_elapsed/1000.0), 3); }
        // // SerialDual.print("|TC_C:"); SerialDual.print((ChamberTC.getMilliVolts()/0.041) + ChamberTC.getTemperature());
        // SerialDual.print("|TC_R:"); SerialDual.print(RerouteTC.readHot());
        TCLog.last_trigger_ms = time_absolute - (time_absolute % TCLog.interval_ms);
        print_current_poll = true;
      }

      if (print_current_poll) { SerialDual.println(); }
      break;
    }
  }
}

void processCommand(float time_elapsed) {
  bool gotCmd = false;
  cmd = "";
  if (serialBuf.feed(Serial, cmd)) {
    gotCmd = true;
  } else if (serial2Buf.feed(Serial2, cmd)) {
    gotCmd = true;
  }

  if (!gotCmd) return;  // nothing ready yet — non-blocking exit

  // New command section
  SerialDual.println();
  SerialDual.print("COMMAND|"); SerialDual.print((time_elapsed/1000.0), 3);
  SerialDual.print("|CMD_RQ:"); SerialDual.print(cmd);
  SerialDual.print("|CMD_EX:");
  // SerialDual.println(cmd);

  cmd.trim();
  cmd.toUpperCase();

  if (cmd.length() == 0) {
    SerialDual.print("Empty command");
    SerialDual.println();
    return;
  }

  // vent
  if (cmd.equals("")) {
    SerialDual.print("Empty command");
  }
  else if (cmd.equalsIgnoreCase("ETHANE_VENT_ON")) {
    EthaneVent.open();
    if(full_output) { SerialDual.print("ETHANE VENT OPEN"); }
  }
  else if (cmd.equalsIgnoreCase("ETHANE_VENT_OFF")) {
    EthaneVent.close();
    if(full_output) { SerialDual.print("ETHANE VENT CLOSED"); }
  } 
  else if (cmd.equalsIgnoreCase("NITROUS_VENT_ON")) {
    NitrousVent.open();
    if(full_output) { SerialDual.print("NITROUS VENT OPEN"); }
  }
  else if (cmd.equalsIgnoreCase("NITROUS_VENT_OFF")) {
    NitrousVent.close();
    if(full_output) { SerialDual.print("NITROUS VENT CLOSED"); }
  }

  // solenoid
  else if (cmd.equalsIgnoreCase("ETHANE_RUN_ON")) {
    EthaneRunValve.open();
    if(full_output) { SerialDual.print("ETHANE RUN VALVE OPEN"); }
  }
  else if (cmd.equalsIgnoreCase("ETHANE_RUN_OFF")) {
    EthaneRunValve.close();
    if(full_output) { SerialDual.print("ETHANE RUN VALVE CLOSED"); }
  } 
  else if (cmd.equalsIgnoreCase("NITROUS_RUN_ON")) {
    NitrousRunValve.open();
    if(full_output) { SerialDual.print("NITROUS RUN VALVE OPEN"); }
  } 
  else if (cmd.equalsIgnoreCase("NITROUS_RUN_OFF")) {
    NitrousRunValve.close();
    if(full_output) { SerialDual.print("NITROUS RUN VALVE CLOSED"); }
  }
  
  // run control -- this is now deprecated
  else if (cmd.equalsIgnoreCase("START")) {
    print_data = true;
    start_time = millis();
  } 
  else if (cmd.equalsIgnoreCase("STOP")) {
    print_data = false;
  }
  else if (cmd.equalsIgnoreCase("STATUS")) {
    status(transmissionFormat);
  }
  
  // ball
  else if (cmd.equalsIgnoreCase("ETHANE_MBV_OPEN")) {
    EthaneMBV.open();
  }
  else if (cmd.equalsIgnoreCase("ETHANE_MBV_CLOSE")) {
    EthaneMBV.close();
  }
  else if (cmd.indexOf("ETHANE_MBV_JOG_") >= 0) {
    EthaneMBV.jog(cmd.substring(16).toInt());
  }
  else if (cmd.equalsIgnoreCase("ETHANE_MBV_BRAKE")) {
    EthaneMBV.brake();
  }
  else if (cmd.equalsIgnoreCase("ETHANE_MBV_COAST")) {
    EthaneMBV.coast();
  }
  else if (cmd.equalsIgnoreCase("NITROUS_MBV_OPEN")) {
    NitrousMBV.open();
  }
  else if (cmd.equalsIgnoreCase("NITROUS_MBV_CLOSE")) {
    NitrousMBV.close();
  }
  else if (cmd.indexOf("NITROUS_MBV_JOG_") >= 0) {
    NitrousMBV.jog(cmd.substring(16).toInt());
  }
  else if (cmd.equalsIgnoreCase("NITROUS_MBV_BRAKE")) {
    NitrousMBV.brake();
  }
  else if (cmd.equalsIgnoreCase("NITROUS_MBV_COAST")) {
    NitrousMBV.coast();
  }
  
  else if (cmd.equalsIgnoreCase("E_STOP")) {
    EMERGENCY_VENT();
  }
  else if (cmd.indexOf("COLD_FLOW_") >= 0) {
    String prop = cmd.substring(10, cmd.lastIndexOf("_"));
    String duration = cmd.substring(cmd.lastIndexOf("_") + 1);
    if (duration.toInt()) {
      if (prop.equalsIgnoreCase("ETHANE")) {
        coldFlowEthane(duration.toInt());
      } else if (prop.equalsIgnoreCase("NITROUS")) {
        coldFlowNitrous(duration.toInt());
      }
    }
  }
  else if (cmd.indexOf("STATIC_FIRE_") >= 0) {
    String duration = cmd.substring(cmd.lastIndexOf("_") + 1);
    if (duration.toInt()) {
      static_fire_duration_ms = duration.toInt();
      static_fire_initializing = true;
    }
  }
  else if (cmd.indexOf("ETHANE_TARGET_PRESSURE_") >= 0) {
    String pressure = cmd.substring(cmd.lastIndexOf("_") + 1);
    if (pressure.toFloat()) {
      ETHANE_TARGET_PRESSURE = pressure.toFloat();
    }
  }
  else if (cmd.indexOf("NITROUS_TARGET_PRESSURE_") >= 0) {
    String pressure = cmd.substring(cmd.lastIndexOf("_") + 1);
    if (pressure.toFloat()) {
      NITROUS_TARGET_PRESSURE = pressure.toFloat();
    }
  }
  else if (cmd.equalsIgnoreCase("ETHANE_LC_TARE")) {
    EthaneLC1.tare();
    EthaneLC2.tare();
    EthaneLC3.tare();
  }
  else if (cmd.equalsIgnoreCase("NITROUS_LC_TARE")) {
    NitrousLC1.tare();
    NitrousLC2.tare();
    NitrousLC3.tare();
  } else if (cmd.equalsIgnoreCase("THRUST_LC_TARE")) {
    ThrustLC.tare();
  } else if (cmd.equalsIgnoreCase("NITROUS_H1_ON")) {
    NitrousHeater1.on();
  } else if (cmd.equalsIgnoreCase("NITROUS_H1_OFF")) {
    NitrousHeater1.off();
  } else if (cmd.equalsIgnoreCase("NITROUS_H2_ON")) {
    NitrousHeater2.on();
  } else if (cmd.equalsIgnoreCase("NITROUS_H2_OFF")) {
    NitrousHeater2.off();
  } else if (cmd.equalsIgnoreCase("ETHANE_H1_ON")) {
    EthaneHeater1.on();
  } else if (cmd.equalsIgnoreCase("ETHANE_H1_OFF")) {
    EthaneHeater1.off();
  } else if (cmd.equalsIgnoreCase("ETHANE_H2_ON")) {
    EthaneHeater2.on();
  } else if (cmd.equalsIgnoreCase("ETHANE_H2_OFF")) {
    EthaneHeater2.off();
  } else if (cmd.equalsIgnoreCase("HEATERS_OFF")) {
    heaters_active = false;
    EthaneHeater1.off();
    EthaneHeater2.off();
    NitrousHeater1.off();
    NitrousHeater2.off();
  }

  else {
    if(full_output) { SerialDual.print("Unknown command.");}
  }
  SerialDual.println();
}

//===========================EXECUTION============================//
/**
 * MARK: Execution
 * */ 

void setup()
{
  // TURN SERIAL OFF; TURN SERIAL2 ON
  SerialDual.setActive(true, true);
  SerialDual.begin(BAUD_RATE);
  //SerialDual.flush();
  SerialDual.println("START");
  cmd.reserve(32);
  delay(2000);

  EthaneMBV.begin();
  NitrousMBV.begin();

  SerialDual.println("START1");
  EthaneLC1.begin(ETHANE_LC1_PINS);
  EthaneLC2.begin(ETHANE_LC2_PINS);
  EthaneLC3.begin(ETHANE_LC3_PINS);
  NitrousLC1.begin(NITROUS_LC1_PINS);
  NitrousLC2.begin(NITROUS_LC2_PINS);
  NitrousLC3.begin(NITROUS_LC3_PINS);

  EthaneLC1.setCalFactor(43.27);
  EthaneLC2.setCalFactor(43.92);
  EthaneLC3.setCalFactor(42.46);
  NitrousLC1.setCalFactor(51.225);
  NitrousLC2.setCalFactor(51.225);
  NitrousLC3.setCalFactor(51.225);
  EthaneLC1.join(&EthaneLC2, &EthaneLC3);
  NitrousLC1.join(&NitrousLC2, &NitrousLC3);

  ThrustLC.begin();
  ThrustLC.setCalFactor(-5.83);


  RerouteTC.begin();
  ChamberTC.begin();
  RerouteTC3.begin();

  // pinMode(CHAMBER_TC_PIN, OUTPUT);      // Force SS high to lock Mega in master mode
  // digitalWrite(CHAMBER_TC_PIN, HIGH);
  // ChamberTC = new ADS1118(CHAMBER_TC_PIN);
  // delay(100);

  // ChamberTC.begin();
  // ChamberTC.setSamplingRate(ChamberTC.RATE_16SPS);
  // ChamberTC.setInputSelected(ChamberTC.DIFF_0_1);
  // ChamberTC.setFullScaleRange(ChamberTC.FSR_0256);

  EthaneHeater1.begin();
  EthaneHeater2.begin();
  NitrousHeater1.begin();
  NitrousHeater2.begin();

  EthaneHeater1.setTarget(ETHANE_TARGET_PRESSURE);
  EthaneHeater2.setTarget(ETHANE_TARGET_PRESSURE);
  NitrousHeater1.setTarget(NITROUS_TARGET_PRESSURE);
  NitrousHeater2.setTarget(NITROUS_TARGET_PRESSURE);
  
  delay(1200);
  if (print_data) {
    start_time = millis();
  }
  if(full_output) {
    SerialDual.println("Commands:");
    SerialDual.println("  ETHANE_VENT_ON / ETHANE_VENT_OFF");
    SerialDual.println("  NITROUS_VENT_ON / NITROUS_VENT_OFF");
    SerialDual.println("  ETHANE_RUN_ON / ETHANE_RUN_OFF");
    SerialDual.println("  NITROUS_RUN_ON / NITROUS_RUN_OFF");
  }  SerialDual.println("END");

}

void loop()
{
  time_absolute = millis();
  time_elapsed = time_absolute - start_time;

  EthaneMBV.update();
  NitrousMBV.update();

  // LOG
  if (print_data)
  {
    status(transmissionFormat);
  }

  if (time_elapsed/1000.0 > runtime && runtime >= 0) {
    print_data = false;
  }

  // COMMANDS
  processCommand(time_elapsed);

  // Scheduling Updates
  if(time_absolute - ValveSchedulePoll.last_trigger_ms >= ValveSchedulePoll.interval_ms) {
    EthaneVent.checkScheduledActuation();
    NitrousVent.checkScheduledActuation();
    EthaneRunValve.checkScheduledActuation();
    NitrousRunValve.checkScheduledActuation();
    EthaneMBV.checkScheduledActuation();
    NitrousMBV.checkScheduledActuation();

    ValveSchedulePoll.last_trigger_ms = time_absolute - (time_absolute % ValveSchedulePoll.interval_ms);
  }

  if(heaters_active && time_absolute - HeaterPoll.last_trigger_ms >= HeaterPoll.interval_ms) {
    EthaneHeater1.update();
    EthaneHeater2.update();
    NitrousHeater1.update();
    NitrousHeater2.update();

    HeaterPoll.last_trigger_ms = time_absolute - (time_absolute % HeaterPoll.interval_ms);
  }

  // Redlines
  if(time_absolute - RedlinePoll.last_trigger_ms >= RedlinePoll.interval_ms) {

    if (current_highest_redline > 0) {
      last_highest_redline = current_highest_redline;
      current_highest_redline--;
    }

    for (int i = 0; i < NUM_REDLINES; i++) {
      current_highest_redline = max(Redlines[i].checkTrigger(current_highest_redline), current_highest_redline);
    }

    if (current_highest_redline > 0) {
      SerialDual.print("DATA|"); SerialDual.print((time_elapsed/1000.0), 3);
      SerialDual.print("|RD_H:"); SerialDual.print(current_highest_redline);
      SerialDual.print("|RD_L:"); SerialDual.print(last_highest_redline);
      SerialDual.println();
    }
    RedlinePoll.last_trigger_ms = time_absolute - (time_absolute % RedlinePoll.interval_ms);
  }

  if(static_fire_initializing) {
    staticFire();
  }

  if (!static_fire_steady && static_fire_start_ms > 0 && time_absolute - static_fire_start_ms >= 500) {
    static_fire_steady = true;
  }
  

}