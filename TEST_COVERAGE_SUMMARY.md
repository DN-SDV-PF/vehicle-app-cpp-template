# Integration Test Coverage Summary

## Updated Test Suite
The integration test has been expanded to comprehensively test all 27 signals subscribed by SampleApp.

## Test Coverage Breakdown

### Powertrain Tests (7 tests)
1. **Speed** - Motion.Locomotion.Speed
2. **Battery SoC** - TractionBattery.StateOfCharge.Current
3. **Battery Voltage** - TractionBattery.CurrentVoltage
4. **Battery Level** - TractionBattery.BatteryLevel
5. **Range** - Powertrain.Range
6. **Current Gear** - Transmission.CurrentGear
7. **Fuel Level** - FuelSystem.Level

### HVAC Tests (12 tests)
8. **AC Active** - HVAC.IsAirConditioningActive
9. **Air Compressor** - HVAC.AirCompressor.IsAirCompressorOn
10. **Sync Mode** - HVAC.Sync.IsSyncOn
11. **Eco Mode** - HVAC.EcoMode.IsEcoModeOn
12. **Air Purifier 1** - HVAC.AirPurifier1.IsAirPurifier1On
13. **Air Purifier 2** - HVAC.AirPurifier2.IsAirPurifier2On
14. **Air Purifier 3** - HVAC.AirPurifier3.IsAirPurifier3On
15. **Pollen Remove** - HVAC.PollenRemove.IsPollenRemoveOn
16. **Swing Function** - HVAC.Swing.IsSwingOn
17. **PM2.5 Remove** - HVAC.PmRemove.IsPmRemoveOn
18. **Recirculation** - HVAC.Recirculation.RecirculationState
19. **Heater** - HVAC.Heater.IsHeaterOn

### Cabin Tests (2 tests)
20. **Rear Shade Switch** - Cabin.RearShade.Switch
21. **Rear Shade Position** - Cabin.RearShade.Position

### Infotainment Tests (5 tests)
22. **Media Action** - Infotainment.Media.Action
23. **Media Source** - Infotainment.Media.Played.Source
24. **Media Volume** - Infotainment.Media.Volume
25. **Media On/Off** - Infotainment.Media.IsOn
26. **Navigation On/Off** - Infotainment.Navigation.IsOn

### Sequential Tests (1 test)
27. **Multiple Speed Updates** - Rapid sequential speed changes

## Test Statistics
- **Total Test Cases:** 27
- **Total Data Updates:** 31 (including 4 rapid sequential speed updates)
- **Signal Categories:** 5 (Motion, Powertrain, HVAC, Cabin, Infotainment)
- **MQTT Topics Verified:** 27

## Running the Tests

### Automated Full Test Suite
```bash
cd /workspaces/vehicle-app-cpp-template/app/tests
./integration_test.sh
```

This will:
1. Start Local Runtime automatically
2. Build SampleApp
3. Run SampleApp
4. Execute all 27 test cases
5. Verify MQTT publications
6. Show comprehensive results
7. Clean up all processes

### Expected Output
```
================================================
  SampleApp Integration Test - Full Run
================================================

[STEP 1/5] Starting Local Runtime...
[STEP 2/5] Building SampleApp...
[STEP 3/5] Starting SampleApp...
[STEP 4/5] Running Integration Tests...
[TEST 1-27] ... (individual test results)
[STEP 5/5] Verifying Results...

================================================
  Test Summary
================================================
  Tests executed: 27
  Data updates sent: 31
  Categories tested:
    - Motion & Speed: 1 test
    - Powertrain (Battery): 4 tests
    - Powertrain (Transmission & Fuel): 2 tests
    - HVAC (Climate Control): 12 tests
    - Cabin (Comfort): 2 tests
    - Infotainment: 5 tests
    - Sequential Updates: 1 test
```

## Files Modified
1. `app/tests/integration_test.sh` - Added 20 new test cases (7→27 total)
2. `app/tests/README_TESTS.md` - Updated documentation with all signal types
3. `app/src/SampleApp.cpp` - Added handlers for all 27 signals
4. `app/AppManifest.json` - Declared all 27 data points and MQTT topics

## Verification
Each test verifies:
- ✅ Signal update sent successfully via REST API
- ✅ SampleApp receives and processes the update
- ✅ Appropriate MQTT message published to correct topic
- ✅ DEBUG logs show signal processing

## MQTT Topic Monitoring
To monitor all MQTT topics during testing:
```bash
mosquitto_sub -h localhost -t 'sampleapp/#' -v
```

This will show all published messages organized by category:
- `sampleapp/currentSpeed`
- `sampleapp/battery*`
- `sampleapp/hvac/*`
- `sampleapp/cabin/*`
- `sampleapp/infotainment/*`
