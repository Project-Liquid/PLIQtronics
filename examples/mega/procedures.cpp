#include "procedures.h"

#define OPEN true
#define CLOSE false

//============================REDLINES============================//

// Trigger Conditions
bool EthaneOverpressureCondition() { return EthaneUpstreamPT.readPressure() > ETHANE_PRESSURE_REDLINE; }
bool NitrousOverpressureCondition() { return NitrousUpstreamPT.readPressure() > NITROUS_PRESSURE_REDLINE; }
bool EthaneMBVCloseFailureCondition() { return false; }  //Unessecary, controlled by cold flow/ static fire
bool NitrousMBVCloseFailureCondition() { return false; } //Unessecary, controlled by cold flow/ static fire
bool CombustionPropogationCondition() { 
  return static_fire_steady && ((fabs((ReroutePT.readPressure()-ChamberPT.readPressure()))/(ReroutePT.readPressure()+1E-12) >0.1)||
    (fabs(EthaneDownstreamPT.readPressure()-ChamberPT.readPressure())/(EthaneDownstreamPT.readPressure()+1E-12) >0.1)||
    ((0.95*ReroutePT.readPressure() < ChamberPT.readPressure())) ||
    (0.95*EthaneDownstreamPT.readPressure() < ChamberPT.readPressure()));}
bool InlineThermalDecompCondition() { return RerouteTC->readHot() > REROUTE_TC_REDLINE;}
bool LostLoadCellCondition() { return false; } //Unnecessary, operator control
bool EthaneUnderweightCondition() { return EthaneLC1->readJoint() < ETHANE_WEIGHT_REDLINE; }
bool NitrousUnderweightCondition() { return false;} //Unnecessary, operator control
bool EthaneOverweightCondition() { return false; } //Unnecessary, operator control
bool NitrousOverweightCondition() { return NitrousLC1->readJoint() > NITROUS_WEIGHT_REDLINE;}

// Redline Responses
void EthaneOverpressureResponse() {
  // Clear schedules to prevent conflicts
  EthaneRunValve.clearSchedule();
  EthaneVent.clearSchedule();
  
  // Solenoid
  EthaneRunValve.open();

  // VENT
  EthaneVent.setNextActuation(0, OPEN);
  EthaneVent.setNextActuation(VENT_TIME, CLOSE);
}

void NitrousOverpressureResponse() {
  // Clear schedules to prevent conflicts
  NitrousRunValve.clearSchedule();
  NitrousVent.clearSchedule();

  // Solenoid
  NitrousRunValve.open();

  // VENT
  NitrousVent.setNextActuation(0, OPEN);
  NitrousVent.setNextActuation(VENT_TIME, CLOSE);  
}

void EthaneMBVCloseFailureResponse() {

}
void NitrousMBVCloseFailureResponse() {

}
void CombustionPropogationResponse() {
  neutralizeAll();

  NitrousMBV->close();
  NitrousRunValve.close();
  EthaneMBV->close();
  EthaneRunValve.close();

  NitrousVent.open();
  NitrousVent.setNextActuation(3000, CLOSE);
  EthaneVent.open();
  EthaneVent.setNextActuation(3000, CLOSE);
}
void InlineThermalDecompResponse() {
  neutralizeAll();

  NitrousMBV->close();
  NitrousRunValve.close();
  EthaneMBV->setNextActuation(1000, CLOSE);
  EthaneRunValve.setNextActuation(1000, CLOSE);
}
void LostLoadCellResponse() {}
void EthaneUnderweightResponse() {
  EthaneRunValve.neutralize();
  EthaneRunValve.close();
  }
void NitrousUnderweightResponse() {}
void EthaneOverweightResponse() {}
void NitrousOverweightResponse() {
  NitrousRunValve.neutralize();
  NitrousRunValve.close();
  }

//=========================TEST SEQUENCES=========================//
void neutralizeAll() {
  EthaneVent.neutralize();
  NitrousVent.neutralize();
  EthaneRunValve.neutralize();
  NitrousRunValve.neutralize();
  EthaneMBV->neutralize();
  NitrousMBV->neutralize();
  EthaneHeater1->off();
  EthaneHeater2->off();
  NitrousHeater1->off();
  NitrousHeater2->off();
}

void EMERGENCY_VENT() {
  // clear the schedules
  neutralizeAll();

  if (EthaneMBV->isOpen()) {
    EthaneMBV->close();
  }
  if (NitrousMBV->isOpen()) {
    NitrousMBV->close();
  }
  ventEthane();
  ventNitrous();
}

void ventEthane() {
  EthaneRunValve.close();
  EthaneVent.setNextActuation(VENT_DELAY, OPEN);
  EthaneVent.setNextActuation(VENT_DELAY + VENT_TIME, CLOSE);
}

void ventNitrous() {
  NitrousRunValve.close();
  NitrousVent.setNextActuation(VENT_DELAY, OPEN);
  NitrousVent.setNextActuation(VENT_DELAY + VENT_TIME, CLOSE);
}

void coldFlowEthane(long duration_ms) {
  EthaneRunValve.clearSchedule();
  EthaneMBV->clearSchedule();

  EthaneRunValve.open();

  EthaneMBV->setNextActuation(RUN_EQUALIZE_TIME, OPEN);
  EthaneMBV->setNextActuation(RUN_EQUALIZE_TIME + duration_ms, CLOSE);

  EthaneRunValve.setNextActuation(RUN_EQUALIZE_TIME + duration_ms, CLOSE);
}

void coldFlowNitrous(long duration_ms) {
  NitrousRunValve.open();

  NitrousMBV->setNextActuation(RUN_EQUALIZE_TIME, OPEN);
  NitrousMBV->setNextActuation(RUN_EQUALIZE_TIME + duration_ms, CLOSE);

  NitrousRunValve.setNextActuation(RUN_EQUALIZE_TIME + duration_ms, false);
}

void staticFire() {
  static_fire_start_ms = millis();
  //NitrousRunValve.open();
  EthaneRunValve.open();

  Ignitor.setNextActuation(5000, true);

  // if (ChamberTC->readHot() > 100) {
    NitrousMBV->open();
    EthaneMBV->setNextActuation(ETHANE_DELAY, OPEN);
    NitrousMBV->setNextActuation(ETHANE_DELAY + static_fire_duration_ms, CLOSE);
    EthaneMBV->setNextActuation(ETHANE_DELAY + static_fire_duration_ms + BURNOUT_DELAY, CLOSE);
    NitrousRunValve.setNextActuation(ETHANE_DELAY + static_fire_duration_ms, CLOSE);
    EthaneRunValve.setNextActuation(ETHANE_DELAY + static_fire_duration_ms + BURNOUT_DELAY, CLOSE);
    
    // Vent
    EthaneVent.setNextActuation(ETHANE_DELAY + static_fire_duration_ms + BURNOUT_DELAY + 100 + VENT_DELAY, OPEN);
    EthaneVent.setNextActuation(ETHANE_DELAY + static_fire_duration_ms + BURNOUT_DELAY + 100 + VENT_DELAY + VENT_TIME, CLOSE);
    NitrousVent.setNextActuation(ETHANE_DELAY + static_fire_duration_ms + BURNOUT_DELAY + VENT_DELAY + VENT_TIME + 1100, OPEN);
    NitrousVent.setNextActuation(ETHANE_DELAY + static_fire_duration_ms + BURNOUT_DELAY + VENT_DELAY + 2*VENT_TIME + 1100, CLOSE);

    static_fire_initializing = false;
    static_fire_steady = false;
  // }
}
