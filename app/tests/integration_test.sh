#!/bin/bash
# Integration test for SampleApp data point subscriptions
# This test verifies that SampleApp is triggered when relevant data changes
# This script automatically starts the runtime, builds and runs the app, then tests it

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

QUAD_REST_URL="http://localhost:50050"
MQTT_TOPIC="sampleapp/currentSpeed"
TEST_LOG="/tmp/sampleapp_test.log"
APP_LOG="/tmp/sampleapp_integration.log"
WORKSPACE_ROOT="$(cd $(dirname $0)/../.. && pwd)"
QUAD_LOCAL_DIR="$WORKSPACE_ROOT/quad-local"

# Function to check if Quad is running
is_quad_running() {
    curl -s http://localhost:50050 > /dev/null 2>&1
}

# Function to setup and start quad-local
setup_and_start_quad() {
    echo -e "\n${BLUE}[SETUP] Checking Quad Local...${NC}"
    
    # Check if quad-local directory exists
    if [ ! -d "$QUAD_LOCAL_DIR" ]; then
        echo "  Quad Local not installed, running setup script..."
        if [ -f "$WORKSPACE_ROOT/.devcontainer/scripts/setup-quad-local.sh" ]; then
            bash "$WORKSPACE_ROOT/.devcontainer/scripts/setup-quad-local.sh"
        else
            echo -e "${RED}  ✗ setup-quad-local.sh not found${NC}"
            exit 1
        fi
    fi
    
    # Check if Quad is already running
    if is_quad_running; then
        echo -e "${GREEN}  ✓ Quad Local is already running${NC}"
        return 0
    fi
    
    echo "  Starting Quad Local..."
    
    # Make sure scripts are executable
    chmod +x "$QUAD_LOCAL_DIR/quad/"*.sh 2>/dev/null || true
    chmod +x "$QUAD_LOCAL_DIR/quad/internal/sound/"* 2>/dev/null || true
    
    # Start quad using docker compose directly (skip sound and patch scripts that may fail)
    cd "$QUAD_LOCAL_DIR/quad"
    docker compose up -d
    
    # Wait for Quad to be ready
    echo "  Waiting for Quad REST API to be ready..."
    for i in {1..30}; do
        if is_quad_running; then
            echo -e "${GREEN}  ✓ Quad Local is ready${NC}"
            return 0
        fi
        echo "    Waiting... ($i/30)"
        sleep 1
    done
    
    echo -e "${RED}  ✗ Quad Local failed to start${NC}"
    return 1
}

# Cleanup function
cleanup() {
    echo -e "\n${YELLOW}Cleaning up...${NC}"
    
    # Stop SampleApp
    if [ ! -z "$APP_PID" ] && kill -0 $APP_PID 2>/dev/null; then
        echo "  Stopping SampleApp (PID: $APP_PID)"
        kill $APP_PID 2>/dev/null || true
        wait $APP_PID 2>/dev/null || true
    fi
    
    # Stop runtime
    echo "  Stopping Local Runtime..."
    cd "$WORKSPACE_ROOT"
    velocitas exec runtime-local down > /dev/null 2>&1 || true
    
    # Stop simulator
    echo "  Stopping DnDataModel Simulator..."
    cd "$WORKSPACE_ROOT/dndatamodel-simulator"
    docker-compose down > /dev/null 2>&1 || true
    
    # Note: We don't stop Quad Local as it may be used by other tests
    # To stop it manually: cd quad-local/quad && docker compose down
    
    echo -e "${GREEN}Cleanup complete${NC}"
}

# Set trap to cleanup on exit
trap cleanup EXIT INT TERM

echo -e "${YELLOW}================================================${NC}"
echo -e "${YELLOW}  SampleApp Integration Test - Full Run${NC}"
echo -e "${YELLOW}================================================${NC}"

# Step 0: Setup and start Quad Local if not running
setup_and_start_quad

# Step 1: Start Local Runtime (MQTT broker)
echo -e "\n${BLUE}[STEP 1/5] Starting Local Runtime (MQTT)...${NC}"
cd "$WORKSPACE_ROOT"
velocitas exec runtime-local up > /dev/null 2>&1 &
echo "  Waiting for runtime to be ready..."
sleep 5

# Check if Quad services are up
if ! is_quad_running; then
    echo -e "${RED}  ✗ Quad REST API not accessible${NC}"
    exit 1
fi
echo -e "${GREEN}  ✓ Local Runtime is up${NC}"

# Step 2: Build SampleApp
echo -e "\n${BLUE}[STEP 2/5] Building SampleApp...${NC}"
cd "$WORKSPACE_ROOT"
if ! ./build.sh > /dev/null 2>&1; then
    echo -e "${RED}  ✗ Build failed${NC}"
    exit 1
fi
echo -e "${GREEN}  ✓ SampleApp built successfully${NC}"

# Step 3: Start SampleApp
echo -e "\n${BLUE}[STEP 3/5] Starting SampleApp...${NC}"
cd "$WORKSPACE_ROOT"

# Set required environment variables
export SDV_MIDDLEWARE_TYPE="native"
export SDV_QUADGRPC_ADDRESS="grpc://127.0.0.1:50051"
export SDV_MQTT_ADDRESS="127.0.0.1:1883"
export KUKSA_DATABROKER_API="duo"

"$WORKSPACE_ROOT/build/bin/app" > "$APP_LOG" 2>&1 &
APP_PID=$!
echo "  SampleApp started (PID: $APP_PID, logs: $APP_LOG)"
echo "  Waiting for app to initialize and subscribe..."
sleep 5

# Check if app is still running
if ! kill -0 $APP_PID 2>/dev/null; then
    echo -e "${RED}  ✗ SampleApp crashed during startup${NC}"
    echo "  Last log lines:"
    tail -n 20 "$APP_LOG"
    exit 1
fi
echo -e "${GREEN}  ✓ SampleApp is running${NC}"

# Step 4: Run Tests
echo -e "\n${BLUE}[STEP 4/5] Running Integration Tests...${NC}"
echo -e "${YELLOW}================================================${NC}"

# Function to send data update via curl
send_data_update() {
    local signal_path=$1
    local value=$2
    local json_path=$3
    
    echo -e "\n${YELLOW}Testing: ${signal_path}${NC}"
    echo "  Sending value: ${value}"
    
    curl -s -X PATCH "${QUAD_REST_URL}/reports/vss" \
        -H "Content-Type: application/json" \
        -d "${json_path}" | jq '.' || echo "  Response: Success"
    
    sleep 1
}

# Test 1: Vehicle Speed
echo -e "\n${GREEN}[TEST 1] Vehicle Speed Update${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Motion.Locomotion.Speed" \
    "188.8" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Motion":{"Locomotion":{"Speed":188.8}}}}}}}'

# Test 2: Battery State of Charge
echo -e "\n${GREEN}[TEST 2] Battery State of Charge Update${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Powertrain.TractionBattery.StateOfCharge.Current" \
    "75.5" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Powertrain":{"TractionBattery":{"StateOfCharge":{"Current":75.5}}}}}}}}'

# Test 3: Battery Voltage
echo -e "\n${GREEN}[TEST 3] Battery Voltage Update${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Powertrain.TractionBattery.CurrentVoltage" \
    "395.2" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Powertrain":{"TractionBattery":{"CurrentVoltage":395.2}}}}}}}'

# Test 4: Battery Level
echo -e "\n${GREEN}[TEST 4] Battery Level Update${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Powertrain.TractionBattery.BatteryLevel" \
    "45" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Powertrain":{"TractionBattery":{"BatteryLevel":45}}}}}}}'

# Test 5: Vehicle Range
echo -e "\n${GREEN}[TEST 5] Vehicle Range Update${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Powertrain.Range" \
    "250000" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Powertrain":{"Range":250000}}}}}}'

# Test 6: Current Gear
echo -e "\n${GREEN}[TEST 6] Transmission Current Gear Update${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Powertrain.Transmission.CurrentGear" \
    "4" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Powertrain":{"Transmission":{"CurrentGear":4}}}}}}}'

# Test 7: Fuel Level
echo -e "\n${GREEN}[TEST 7] Fuel System Level Update${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Powertrain.FuelSystem.Level" \
    "65" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Powertrain":{"FuelSystem":{"Level":65}}}}}}}'

# HVAC Tests
echo -e "\n${GREEN}[TEST 8] HVAC - Air Conditioning Active${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.IsAirConditioningActive" \
    "true" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"IsAirConditioningActive":true}}}}}}}'

echo -e "\n${GREEN}[TEST 9] HVAC - Air Compressor${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.AirCompressor.IsAirCompressorOn" \
    "true" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"AirCompressor":{"IsAirCompressorOn":true}}}}}}}}'

echo -e "\n${GREEN}[TEST 10] HVAC - Sync Mode${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.Sync.IsSyncOn" \
    "false" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"Sync":{"IsSyncOn":false}}}}}}}}'

echo -e "\n${GREEN}[TEST 11] HVAC - Eco Mode${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.EcoMode.IsEcoModeOn" \
    "true" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"EcoMode":{"IsEcoModeOn":true}}}}}}}}'

echo -e "\n${GREEN}[TEST 12] HVAC - Air Purifier 1${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier1.IsAirPurifier1On" \
    "true" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"AirPurifier1":{"IsAirPurifier1On":true}}}}}}}}'

echo -e "\n${GREEN}[TEST 13] HVAC - Air Purifier 2${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier2.IsAirPurifier2On" \
    "false" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"AirPurifier2":{"IsAirPurifier2On":false}}}}}}}}'

echo -e "\n${GREEN}[TEST 14] HVAC - Air Purifier 3${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier3.IsAirPurifier3On" \
    "true" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"AirPurifier3":{"IsAirPurifier3On":true}}}}}}}}'

echo -e "\n${GREEN}[TEST 15] HVAC - Pollen Remove${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.PollenRemove.IsPollenRemoveOn" \
    "true" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"PollenRemove":{"IsPollenRemoveOn":true}}}}}}}}'

echo -e "\n${GREEN}[TEST 16] HVAC - Swing Function${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.Swing.IsSwingOn" \
    "false" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"Swing":{"IsSwingOn":false}}}}}}}}'

echo -e "\n${GREEN}[TEST 17] HVAC - PM2.5 Remove${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.PmRemove.IsPmRemoveOn" \
    "true" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"PmRemove":{"IsPmRemoveOn":true}}}}}}}}'

echo -e "\n${GREEN}[TEST 18] HVAC - Recirculation${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.Recirculation.RecirculationState" \
    "kRecirculatedAir" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"Recirculation":{"RecirculationState":"kRecirculatedAir"}}}}}}}}'

echo -e "\n${GREEN}[TEST 19] HVAC - Heater${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.HVAC.Heater.IsHeaterOn" \
    "true" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"Heater":{"IsHeaterOn":true}}}}}}}}'

# Cabin Tests
echo -e "\n${GREEN}[TEST 20] Cabin - Rear Shade Switch${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.RearShade.Switch" \
    "kOpen" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"RearShade":{"Switch":"kOpen"}}}}}}}'

echo -e "\n${GREEN}[TEST 21] Cabin - Rear Shade Position${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.RearShade.Position" \
    "75" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"RearShade":{"Position":75}}}}}}}'

# Infotainment Tests
echo -e "\n${GREEN}[TEST 22] Infotainment - Media Action${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.Infotainment.Media.Action" \
    "kPlay" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"Infotainment":{"Media":{"Action":"kPlay"}}}}}}}}'

echo -e "\n${GREEN}[TEST 23] Infotainment - Media Source${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.Infotainment.Media.Played.Source" \
    "kBluetooth" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"Infotainment":{"Media":{"Played":{"Source":"kBluetooth"}}}}}}}}}'

echo -e "\n${GREEN}[TEST 24] Infotainment - Media Volume${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.Infotainment.Media.Volume" \
    "65" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"Infotainment":{"Media":{"Volume":65}}}}}}}}}'

echo -e "\n${GREEN}[TEST 25] Infotainment - Media On${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.Infotainment.Media.IsOn" \
    "true" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"Infotainment":{"Media":{"IsOn":true}}}}}}}}}'

echo -e "\n${GREEN}[TEST 26] Infotainment - Navigation On${NC}"
send_data_update \
    "Vehicle.EgoVehicle.Cabin.Infotainment.Navigation.IsOn" \
    "true" \
    '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"Infotainment":{"Navigation":{"IsOn":true}}}}}}}}}'

# Test 27: Multiple updates in sequence (Speed variations)
echo -e "\n${GREEN}[TEST 27] Multiple Sequential Speed Updates${NC}"
echo "  Testing rapid sequential speed updates..."
for speed in 50.0 75.5 100.0 120.5; do
    curl -s -X PATCH "${QUAD_REST_URL}/reports/vss" \
        -H "Content-Type: application/json" \
        -d "{\"item\":{\"dndatamodel\":{\"Vehicle\":{\"EgoVehicle\":{\"Motion\":{\"Locomotion\":{\"Speed\":${speed}}}}}}}}" > /dev/null
    echo "  Sent speed: ${speed} km/h"
    sleep 0.5
done

echo -e "\n${YELLOW}================================================${NC}"
echo -e "${GREEN}All test data updates sent!${NC}"
echo -e "${YELLOW}================================================${NC}"

# Step 5: Verify Results
echo -e "\n${BLUE}[STEP 5/5] Verifying Results...${NC}"
sleep 2

# Temporarily disable exit on error for verification
set +e

# Check app logs for expected messages
EXPECTED_MESSAGES=(
    "Data Point Change Detected"
    "Speed changed detected"
    "Signal Path: dndatamodel"
)

echo "  Checking SampleApp logs for expected messages..."
SUCCESS_COUNT=0
for msg in "${EXPECTED_MESSAGES[@]}"; do
    grep -iq "$msg" "$APP_LOG"
    if [ $? -eq 0 ]; then
        echo -e "    ${GREEN}✓${NC} Found: '$msg'"
        ((SUCCESS_COUNT++))
    else
        echo -e "    ${RED}✗${NC} Missing: '$msg'"
    fi
done

# Re-enable exit on error
set -e

echo ""
if [ $SUCCESS_COUNT -eq ${#EXPECTED_MESSAGES[@]} ]; then
    echo -e "${GREEN}✓ All verification checks passed!${NC}"
    FINAL_STATUS=0
else
    echo -e "${YELLOW}⚠ Some verification checks failed (${SUCCESS_COUNT}/${#EXPECTED_MESSAGES[@]})${NC}"
    FINAL_STATUS=1
fi

# Show summary
echo -e "\n${YELLOW}================================================${NC}"
echo -e "${YELLOW}  Test Summary${NC}"
echo -e "${YELLOW}================================================${NC}"
echo "  Tests executed: 27"
echo "  Data updates sent: 31"
echo "  Verification checks: ${SUCCESS_COUNT}/${#EXPECTED_MESSAGES[@]}"
echo ""
echo "  Categories tested:"
echo "    - Motion & Speed: 1 test"
echo "    - Powertrain (Battery): 4 tests"
echo "    - Powertrain (Transmission & Fuel): 2 tests"
echo "    - HVAC (Climate Control): 12 tests"
echo "    - Cabin (Comfort): 2 tests"
echo "    - Infotainment: 5 tests"
echo "    - Sequential Updates: 1 test"
echo ""
echo "  SampleApp logs: $APP_LOG"
echo ""
echo -e "${YELLOW}To view full app logs:${NC}"
echo "  cat $APP_LOG"
echo ""

# Show last 30 lines of app log
echo -e "${YELLOW}Last 30 lines of SampleApp log:${NC}"
echo -e "${YELLOW}------------------------------------------------${NC}"
tail -n 30 "$APP_LOG"
echo -e "${YELLOW}------------------------------------------------${NC}"

exit $FINAL_STATUS
