# SampleApp Integration Tests

This directory contains integration tests for the SampleApp to verify that it correctly responds to data point changes.

## Test Files

### 1. `integration_test.sh`
A bash script that sends data updates via curl to test all subscribed data points.

**Usage:**
```bash
cd /workspaces/vehicle-app-cpp-template/app/tests
./integration_test.sh
```

**What it tests:**
- Vehicle Speed updates
- Battery State of Charge updates
- Battery Voltage updates
- Battery Level updates
- Vehicle Range updates
- Transmission Gear updates
- Fuel Level updates
- HVAC Climate Control (12 signals)
  - Air Conditioning Active
  - Air Compressor
  - Sync Mode
  - Eco Mode
  - Air Purifiers (1-3)
  - Pollen Remove
  - Swing Function
  - PM2.5 Remove
  - Recirculation State
  - Heater
- Cabin Comfort (2 signals)
  - Rear Shade Switch
  - Rear Shade Position
- Infotainment (5 signals)
  - Media Action
  - Media Source
  - Media Volume
  - Media On/Off
  - Navigation On/Off
- Sequential rapid updates

**Total: 27 test cases covering all subscribed signals**

### 2. `test_datapoint_subscriptions.py`
A Python-based integration test suite that validates data point subscriptions with detailed reporting.

**Prerequisites:**
```bash
pip install requests
```

**Usage:**
```bash
cd /workspaces/vehicle-app-cpp-template/app/tests
python3 test_datapoint_subscriptions.py
```

**Features:**
- Prerequisite checking (verifies Quad REST API is accessible)
- Comprehensive test coverage for all subscribed data points
- Colored console output
- Detailed test summary and pass/fail reporting

## Running the Tests

### Automated Test Execution (Recommended)

Both test scripts now automatically:
1. Start the Local Runtime (Quad services + Simulator)
2. Build the SampleApp
3. Run the SampleApp
4. Execute all integration tests
5. Verify results from app logs
6. Clean up (stop app and runtime)

**Option A: Shell script**
```bash
cd app/tests
./integration_test.sh
```

**Option B: Python script (detailed reporting)**
```bash
cd app/tests
python3 test_datapoint_subscriptions.py
```

### Manual Test Execution

If you prefer to manually control each step:

**Step 1: Start the Runtime**
```bash
velocitas exec runtime-local up
```

**Step 2: Build and Run SampleApp**
```bash
./build.sh
velocitas exec runtime-local run-vehicle-app build/bin/app
```

**Step 3: Run test commands manually (see Manual Testing section)**

### Verify Results

The automated tests will show:
- Test execution results (PASS/FAIL for each data point)
- Verification checks (confirms app callbacks were triggered)
- Last 20-30 lines of SampleApp logs
- Summary of all tests

Check for:
- `=== Data Point Change Detected ===` messages
- `Signal Path:` showing the updated data point
- `*** Speed changed detected! New speed: X km/h ***` for speed updates

## Example Test Output

### Shell Script Output:
```
================================================
  SampleApp Integration Test - Full Run
================================================

[STEP 1/5] Starting Local Runtime...
  Waiting for runtime to be ready...
  ✓ Local Runtime is up

[STEP 2/5] Building SampleApp...
  ✓ SampleApp built successfully

[STEP 3/5] Starting SampleApp...
  SampleApp started (PID: 12345, logs: /tmp/sampleapp_integration.log)
  ✓ SampleApp is running

[STEP 4/5] Running Integration Tests...

[TEST 1] Vehicle Speed Update
  ✓ Successfully updated Vehicle.EgoVehicle.Motion.Locomotion.Speed = 50.0
...

[STEP 5/5] Verifying Results...
  Checking SampleApp logs for expected messages...
    ✓ Found: 'Data Point Change Detected'
    ✓ Found: 'Speed changed detected'
    ✓ Found: 'Signal Path: dndatamodel'

✓ All verification checks passed!
```

### Python Script Output:
```
======================================================================
  SampleApp Data Point Subscription Integration Tests - Full Run
======================================================================

[STEP 1/4] Starting Local Runtime...
  Waiting for runtime to be ready...
  ✓ Local Runtime is up and accessible

[STEP 2/4] Building SampleApp...
  ✓ SampleApp built successfully

[STEP 3/4] Starting SampleApp...
  SampleApp started (PID: 12345, logs: /tmp/sampleapp_integration_py.log)
  ✓ SampleApp is running

[STEP 4/4] Running Integration Tests...

[TEST 1] Vehicle Speed Update
  ✓ Successfully updated Vehicle.EgoVehicle.Motion.Locomotion.Speed = 50.0
  ...

=== Verifying Results ===
  Checking SampleApp logs for expected messages...
    ✓ Found: 'Data Point Change Detected'
    ✓ Found: 'Speed changed detected'
    ✓ Found: 'Signal Path: dndatamodel'

======================================================================
  Test Summary
======================================================================
  ✓ PASS: Speed Update
  ✓ PASS: Battery SoC Update
  ...
  
  Data Update Tests: 7/7 passed
  Verification: PASSED
======================================================================
```

## Expected SampleApp Behavior

When tests are run, SampleApp should log:

```
=== Data Point Change Detected ===
Signal Path: dndatamodel/Vehicle.EgoVehicle.Motion.Locomotion.Speed
Data point dndatamodel/Vehicle.EgoVehicle.Motion.Locomotion.Speed has been updated
*** Speed changed detected! New speed: 188.8 km/h ***
```

For each data point update, you should see:
1. Generic `onDataPointChanged` handler triggered
2. For speed updates specifically: `onSpeedChanged` handler also triggered
3. MQTT message published to appropriate topic (e.g., `sampleapp/currentSpeed`, `sampleapp/hvac/acActive`, `sampleapp/infotainment/mediaVolume`)

All signals publish to their respective MQTT topics:
- **Powertrain:** `sampleapp/currentSpeed`, `sampleapp/batterySOC`, etc.
- **HVAC:** `sampleapp/hvac/acActive`, `sampleapp/hvac/ecoMode`, etc.
- **Cabin:** `sampleapp/cabin/rearShadeSwitch`, `sampleapp/cabin/rearShadePosition`
- **Infotainment:** `sampleapp/infotainment/mediaAction`, `sampleapp/infotainment/mediaVolume`, etc.

## Troubleshooting

**Problem:** Build fails
- **Solution:** Ensure dependencies are installed: `./install_dependencies.sh`

**Problem:** Runtime fails to start
- **Solution:** Check if ports 50050/50051/1883 are available: `netstat -tuln | grep -E "50050|50051|1883"`
- **Solution:** Stop any existing containers: `docker ps -a` and `docker stop <container>`

**Problem:** SampleApp crashes on startup
- **Solution:** Check the log file (path shown in test output) for error messages
- **Solution:** Verify Quad services are accessible: `curl http://localhost:50050`

**Problem:** Python script fails with import error
- **Solution:** Install required packages: `pip install requests`

**Problem:** Tests timeout waiting for runtime
- **Solution:** Increase wait time in script or manually start runtime first

**Problem:** Cleanup fails
- **Solution:** Manually stop services:
  ```bash
  velocitas exec runtime-local down
  docker-compose -f dndatamodel-simulator/docker-compose.yml down
  pkill -f "build/bin/app"
  ```

## Manual Testing with curl

You can also manually test individual data points:

```bash
# Update Speed
curl -X PATCH http://localhost:50050/reports/vss \
  -H "Content-Type: application/json" \
  -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Motion":{"Locomotion":{"Speed":120.5}}}}}}}'

# Update Battery Level
curl -X PATCH http://localhost:50050/reports/vss \
  -H "Content-Type: application/json" \
  -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Powertrain":{"TractionBattery":{"BatteryLevel":75}}}}}}}'

# Update Current Gear
curl -X PATCH http://localhost:50050/reports/vss \
  -H "Content-Type: application/json" \
  -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Powertrain":{"Transmission":{"CurrentGear":3}}}}}}}'

# Update HVAC Air Conditioning
curl -X PATCH http://localhost:50050/reports/vss \
  -H "Content-Type: application/json" \
  -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"IsAirConditioningActive":true}}}}}}}'

# Update HVAC Eco Mode
curl -X PATCH http://localhost:50050/reports/vss \
  -H "Content-Type: application/json" \
  -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"EcoMode":{"IsEcoModeOn":true}}}}}}}}'

# Update Cabin Rear Shade
curl -X PATCH http://localhost:50050/reports/vss \
  -H "Content-Type: application/json" \
  -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"RearShade":{"Position":50}}}}}}}'

# Update Infotainment Media Volume
curl -X PATCH http://localhost:50050/reports/vss \
  -H "Content-Type: application/json" \
  -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"Infotainment":{"Media":{"Volume":75}}}}}}}}'

# Update Infotainment Navigation
curl -X PATCH http://localhost:50050/reports/vss \
  -H "Content-Type: application/json" \
  -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"Infotainment":{"Navigation":{"IsOn":true}}}}}}}}'
```

## Monitored Data Points

The SampleApp subscribes to these data points (27 total):

### Motion & Speed (1)
1. `dndatamodel/Vehicle.EgoVehicle.Motion.Locomotion.Speed` (float, km/h)

### Powertrain - Battery (4)
2. `dndatamodel/Vehicle.EgoVehicle.Powertrain.TractionBattery.StateOfCharge.Current` (float, %)
3. `dndatamodel/Vehicle.EgoVehicle.Powertrain.TractionBattery.CurrentVoltage` (float, V)
4. `dndatamodel/Vehicle.EgoVehicle.Powertrain.TractionBattery.BatteryLevel` (uint16, kWh)
5. `dndatamodel/Vehicle.EgoVehicle.Powertrain.Range` (uint32, m)

### Powertrain - Transmission & Fuel (2)
6. `dndatamodel/Vehicle.EgoVehicle.Powertrain.Transmission.CurrentGear` (int8)
7. `dndatamodel/Vehicle.EgoVehicle.Powertrain.FuelSystem.Level` (uint8, %)

### HVAC - Climate Control (12)
8. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.IsAirConditioningActive` (boolean)
9. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.AirCompressor.IsAirCompressorOn` (boolean)
10. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.Sync.IsSyncOn` (boolean)
11. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.EcoMode.IsEcoModeOn` (boolean)
12. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier1.IsAirPurifier1On` (boolean)
13. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier2.IsAirPurifier2On` (boolean)
14. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier3.IsAirPurifier3On` (boolean)
15. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.PollenRemove.IsPollenRemoveOn` (boolean)
16. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.Swing.IsSwingOn` (boolean)
17. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.PmRemove.IsPmRemoveOn` (boolean)
18. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.Recirculation.RecirculationState` (string)
19. `dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.Heater.IsHeaterOn` (boolean)

### Cabin - Comfort (2)
20. `dndatamodel/Vehicle.EgoVehicle.Cabin.RearShade.Switch` (string)
21. `dndatamodel/Vehicle.EgoVehicle.Cabin.RearShade.Position` (uint8, %)

### Infotainment (5)
22. `dndatamodel/Vehicle.EgoVehicle.Cabin.Infotainment.Media.Action` (string)
23. `dndatamodel/Vehicle.EgoVehicle.Cabin.Infotainment.Media.Played.Source` (string)
24. `dndatamodel/Vehicle.EgoVehicle.Cabin.Infotainment.Media.Volume` (uint8)
25. `dndatamodel/Vehicle.EgoVehicle.Cabin.Infotainment.Media.IsOn` (boolean)
26. `dndatamodel/Vehicle.EgoVehicle.Cabin.Infotainment.Navigation.IsOn` (boolean)
