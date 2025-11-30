#!/usr/bin/env python3
"""
Integration tests for SampleApp data point subscriptions.
Tests that the app is triggered when relevant vehicle data changes.
This script automatically starts the runtime, builds and runs the app, then tests it.
"""

import json
import requests
import time
import sys
import subprocess
import os
import signal
import atexit

# Configuration
QUAD_REST_URL = "http://localhost:50050"
QUAD_GRPC_URL = "http://localhost:50051"
WORKSPACE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))
APP_LOG = "/tmp/sampleapp_integration_py.log"

# Global process references
runtime_process = None
app_process = None

class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'  # No Color

def print_colored(message, color):
    """Print colored message to console."""
    print(f"{color}{message}{Colors.NC}")

def send_datapoint_update(signal_path, value, json_payload):
    """
    Send a datapoint update via REST API.
    
    Args:
        signal_path: The full signal path
        value: The value to set
        json_payload: The JSON payload to send
    
    Returns:
        True if successful, False otherwise
    """
    url = f"{QUAD_REST_URL}/reports/vss"
    headers = {"Content-Type": "application/json"}
    
    try:
        response = requests.patch(url, headers=headers, json=json_payload, timeout=2)
        if response.status_code == 200:
            print_colored(f"  ✓ Successfully updated {signal_path} = {value}", Colors.GREEN)
            return True
        else:
            print_colored(f"  ✗ Failed to update {signal_path}: {response.status_code}", Colors.RED)
            return False
    except requests.exceptions.RequestException as e:
        print_colored(f"  ✗ Error updating {signal_path}: {e}", Colors.RED)
        return False

def test_speed_update():
    """Test Vehicle Speed data point."""
    print_colored("\n[TEST 1] Vehicle Speed Update", Colors.BLUE)
    
    test_values = [50.0, 75.5, 100.0, 120.5, 188.8]
    success_count = 0
    
    for speed in test_values:
        payload = {
            "item": {
                "dndatamodel": {
                    "Vehicle": {
                        "EgoVehicle": {
                            "Motion": {
                                "Locomotion": {
                                    "Speed": speed
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if send_datapoint_update("Vehicle.EgoVehicle.Motion.Locomotion.Speed", speed, payload):
            success_count += 1
        time.sleep(0.5)
    
    print_colored(f"  Speed updates: {success_count}/{len(test_values)} successful", Colors.YELLOW)
    return success_count == len(test_values)

def test_battery_soc_update():
    """Test Battery State of Charge data point."""
    print_colored("\n[TEST 2] Battery State of Charge Update", Colors.BLUE)
    
    test_values = [25.0, 50.0, 75.5, 100.0]
    success_count = 0
    
    for soc in test_values:
        payload = {
            "item": {
                "dndatamodel": {
                    "Vehicle": {
                        "EgoVehicle": {
                            "Powertrain": {
                                "TractionBattery": {
                                    "StateOfCharge": {
                                        "Current": soc
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if send_datapoint_update("Vehicle.EgoVehicle.Powertrain.TractionBattery.StateOfCharge.Current", soc, payload):
            success_count += 1
        time.sleep(0.5)
    
    print_colored(f"  Battery SoC updates: {success_count}/{len(test_values)} successful", Colors.YELLOW)
    return success_count == len(test_values)

def test_battery_voltage_update():
    """Test Battery Voltage data point."""
    print_colored("\n[TEST 3] Battery Voltage Update", Colors.BLUE)
    
    test_values = [350.0, 380.5, 395.2, 410.0]
    success_count = 0
    
    for voltage in test_values:
        payload = {
            "item": {
                "dndatamodel": {
                    "Vehicle": {
                        "EgoVehicle": {
                            "Powertrain": {
                                "TractionBattery": {
                                    "CurrentVoltage": voltage
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if send_datapoint_update("Vehicle.EgoVehicle.Powertrain.TractionBattery.CurrentVoltage", voltage, payload):
            success_count += 1
        time.sleep(0.5)
    
    print_colored(f"  Battery voltage updates: {success_count}/{len(test_values)} successful", Colors.YELLOW)
    return success_count == len(test_values)

def test_battery_level_update():
    """Test Battery Level data point."""
    print_colored("\n[TEST 4] Battery Level Update", Colors.BLUE)
    
    test_values = [20, 35, 45, 60, 80]
    success_count = 0
    
    for level in test_values:
        payload = {
            "item": {
                "dndatamodel": {
                    "Vehicle": {
                        "EgoVehicle": {
                            "Powertrain": {
                                "TractionBattery": {
                                    "BatteryLevel": level
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if send_datapoint_update("Vehicle.EgoVehicle.Powertrain.TractionBattery.BatteryLevel", level, payload):
            success_count += 1
        time.sleep(0.5)
    
    print_colored(f"  Battery level updates: {success_count}/{len(test_values)} successful", Colors.YELLOW)
    return success_count == len(test_values)

def test_range_update():
    """Test Vehicle Range data point."""
    print_colored("\n[TEST 5] Vehicle Range Update", Colors.BLUE)
    
    test_values = [100000, 200000, 250000, 300000]  # in meters
    success_count = 0
    
    for range_val in test_values:
        payload = {
            "item": {
                "dndatamodel": {
                    "Vehicle": {
                        "EgoVehicle": {
                            "Powertrain": {
                                "Range": range_val
                            }
                        }
                    }
                }
            }
        }
        
        if send_datapoint_update("Vehicle.EgoVehicle.Powertrain.Range", range_val, payload):
            success_count += 1
        time.sleep(0.5)
    
    print_colored(f"  Range updates: {success_count}/{len(test_values)} successful", Colors.YELLOW)
    return success_count == len(test_values)

def test_gear_update():
    """Test Transmission Current Gear data point."""
    print_colored("\n[TEST 6] Transmission Current Gear Update", Colors.BLUE)
    
    test_values = [1, 2, 3, 4, 5]
    success_count = 0
    
    for gear in test_values:
        payload = {
            "item": {
                "dndatamodel": {
                    "Vehicle": {
                        "EgoVehicle": {
                            "Powertrain": {
                                "Transmission": {
                                    "CurrentGear": gear
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if send_datapoint_update("Vehicle.EgoVehicle.Powertrain.Transmission.CurrentGear", gear, payload):
            success_count += 1
        time.sleep(0.5)
    
    print_colored(f"  Gear updates: {success_count}/{len(test_values)} successful", Colors.YELLOW)
    return success_count == len(test_values)

def test_fuel_level_update():
    """Test Fuel Level data point."""
    print_colored("\n[TEST 7] Fuel System Level Update", Colors.BLUE)
    
    test_values = [25, 45, 65, 85, 100]  # percentage
    success_count = 0
    
    for level in test_values:
        payload = {
            "item": {
                "dndatamodel": {
                    "Vehicle": {
                        "EgoVehicle": {
                            "Powertrain": {
                                "FuelSystem": {
                                    "Level": level
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if send_datapoint_update("Vehicle.EgoVehicle.Powertrain.FuelSystem.Level", level, payload):
            success_count += 1
        time.sleep(0.5)
    
    print_colored(f"  Fuel level updates: {success_count}/{len(test_values)} successful", Colors.YELLOW)
    return success_count == len(test_values)

def cleanup():
    """Cleanup function to stop all services."""
    global app_process, runtime_process
    
    print_colored("\n=== Cleaning Up ===", Colors.YELLOW)
    
    # Stop SampleApp
    if app_process:
        print("  Stopping SampleApp...")
        try:
            app_process.terminate()
            app_process.wait(timeout=5)
        except:
            app_process.kill()
        print_colored("  ✓ SampleApp stopped", Colors.GREEN)
    
    # Stop Local Runtime
    print("  Stopping Local Runtime...")
    try:
        subprocess.run(
            ["velocitas", "exec", "runtime-local", "down"],
            cwd=WORKSPACE_ROOT,
            capture_output=True,
            timeout=30
        )
        print_colored("  ✓ Local Runtime stopped", Colors.GREEN)
    except Exception as e:
        print_colored(f"  ⚠ Warning: Could not stop runtime: {e}", Colors.YELLOW)
    
    # Stop Simulator
    print("  Stopping DnDataModel Simulator...")
    try:
        subprocess.run(
            ["docker-compose", "down"],
            cwd=os.path.join(WORKSPACE_ROOT, "dndatamodel-simulator"),
            capture_output=True,
            timeout=30
        )
        print_colored("  ✓ Simulator stopped", Colors.GREEN)
    except Exception as e:
        print_colored(f"  ⚠ Warning: Could not stop simulator: {e}", Colors.YELLOW)

def start_runtime():
    """Start the Local Runtime."""
    print_colored("\n[STEP 1/4] Starting Local Runtime...", Colors.BLUE)
    
    try:
        # Start runtime
        subprocess.run(
            ["velocitas", "exec", "runtime-local", "up"],
            cwd=WORKSPACE_ROOT,
            capture_output=True,
            timeout=60
        )
        
        print("  Waiting for runtime to be ready...")
        time.sleep(5)
        
        # Verify runtime is accessible
        for i in range(10):
            try:
                requests.get(f"{QUAD_REST_URL}/reports/vss", timeout=2)
                print_colored("  ✓ Local Runtime is up and accessible", Colors.GREEN)
                return True
            except:
                if i < 9:
                    time.sleep(2)
        
        print_colored("  ✗ Runtime started but not accessible", Colors.RED)
        return False
        
    except Exception as e:
        print_colored(f"  ✗ Failed to start runtime: {e}", Colors.RED)
        return False

def build_app():
    """Build the SampleApp."""
    print_colored("\n[STEP 2/4] Building SampleApp...", Colors.BLUE)
    
    try:
        result = subprocess.run(
            ["./build.sh"],
            cwd=WORKSPACE_ROOT,
            capture_output=True,
            timeout=300
        )
        
        if result.returncode == 0:
            print_colored("  ✓ SampleApp built successfully", Colors.GREEN)
            return True
        else:
            print_colored("  ✗ Build failed", Colors.RED)
            print(result.stderr.decode())
            return False
            
    except Exception as e:
        print_colored(f"  ✗ Failed to build: {e}", Colors.RED)
        return False

def start_app():
    """Start the SampleApp."""
    global app_process
    
    print_colored("\n[STEP 3/4] Starting SampleApp...", Colors.BLUE)
    
    try:
        app_binary = os.path.join(WORKSPACE_ROOT, "build/bin/app")
        
        # Set required environment variables
        env = os.environ.copy()
        env['SDV_MIDDLEWARE_TYPE'] = 'native'
        env['SDV_QUADGRPC_ADDRESS'] = 'grpc://127.0.0.1:50051'
        env['SDV_MQTT_ADDRESS'] = '127.0.0.1:1883'
        env['KUKSA_DATABROKER_API'] = 'duo'
        
        with open(APP_LOG, 'w') as log_file:
            app_process = subprocess.Popen(
                [app_binary],
                cwd=WORKSPACE_ROOT,
                stdout=log_file,
                stderr=subprocess.STDOUT,
                env=env
            )
        
        print(f"  SampleApp started (PID: {app_process.pid}, logs: {APP_LOG})")
        print("  Waiting for app to initialize and subscribe...")
        time.sleep(5)
        
        # Check if app is still running
        if app_process.poll() is not None:
            print_colored("  ✗ SampleApp crashed during startup", Colors.RED)
            with open(APP_LOG, 'r') as f:
                print("  Last log lines:")
                print(f.read())
            return False
        
        print_colored("  ✓ SampleApp is running", Colors.GREEN)
        return True
        
    except Exception as e:
        print_colored(f"  ✗ Failed to start app: {e}", Colors.RED)
        return False

def verify_results():
    """Verify that the app responded correctly to data changes."""
    print_colored("\n=== Verifying Results ===", Colors.YELLOW)
    
    time.sleep(2)
    
    try:
        with open(APP_LOG, 'r') as f:
            log_content = f.read()
        
        expected_messages = [
            "Data Point Change Detected",
            "Speed changed detected",
            "Signal Path: dndatamodel"
        ]
        
        print("  Checking SampleApp logs for expected messages...")
        success_count = 0
        for msg in expected_messages:
            if msg in log_content:
                print_colored(f"    ✓ Found: '{msg}'", Colors.GREEN)
                success_count += 1
            else:
                print_colored(f"    ✗ Missing: '{msg}'", Colors.RED)
        
        print_colored(f"\n  Verification: {success_count}/{len(expected_messages)} checks passed", 
                     Colors.GREEN if success_count == len(expected_messages) else Colors.YELLOW)
        
        return success_count == len(expected_messages)
        
    except Exception as e:
        print_colored(f"  ✗ Error verifying results: {e}", Colors.RED)
        return False

def main():
    """Run all integration tests."""
    # Register cleanup handler
    atexit.register(cleanup)
    
    print_colored("=" * 70, Colors.YELLOW)
    print_colored("  SampleApp Data Point Subscription Integration Tests - Full Run", Colors.YELLOW)
    print_colored("=" * 70, Colors.YELLOW)
    
    # Step 1: Start Runtime
    if not start_runtime():
        print_colored("\n✗ Failed to start runtime", Colors.RED)
        return 1
    
    # Step 2: Build App
    if not build_app():
        print_colored("\n✗ Failed to build app", Colors.RED)
        return 1
    
    # Step 3: Start App
    if not start_app():
        print_colored("\n✗ Failed to start app", Colors.RED)
        return 1
    
    # Step 4: Run Tests
    print_colored("\n[STEP 4/4] Running Integration Tests...", Colors.BLUE)
    print_colored("=" * 70, Colors.YELLOW)
    
    results = []
    results.append(("Speed Update", test_speed_update()))
    results.append(("Battery SoC Update", test_battery_soc_update()))
    results.append(("Battery Voltage Update", test_battery_voltage_update()))
    results.append(("Battery Level Update", test_battery_level_update()))
    results.append(("Range Update", test_range_update()))
    results.append(("Gear Update", test_gear_update()))
    results.append(("Fuel Level Update", test_fuel_level_update()))
    
    # Verify results
    verification_passed = verify_results()
    
    # Print summary
    print_colored("\n" + "=" * 70, Colors.YELLOW)
    print_colored("  Test Summary", Colors.YELLOW)
    print_colored("=" * 70, Colors.YELLOW)
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for test_name, result in results:
        status = "✓ PASS" if result else "✗ FAIL"
        color = Colors.GREEN if result else Colors.RED)
        print_colored(f"  {status}: {test_name}", color)
    
    print_colored("\n" + "-" * 70, Colors.YELLOW)
    print_colored(f"  Data Update Tests: {passed}/{total} passed", Colors.YELLOW)
    print_colored(f"  Verification: {'PASSED' if verification_passed else 'FAILED'}", 
                 Colors.GREEN if verification_passed else Colors.RED)
    print_colored("=" * 70, Colors.YELLOW)
    
    print_colored(f"\n📋 SampleApp logs: {APP_LOG}", Colors.BLUE)
    print_colored("   To view: cat " + APP_LOG, Colors.BLUE)
    
    # Show last lines of log
    try:
        with open(APP_LOG, 'r') as f:
            lines = f.readlines()
            print_colored("\n📄 Last 20 lines of SampleApp log:", Colors.BLUE)
            print_colored("-" * 70, Colors.YELLOW)
            print(''.join(lines[-20:]))
            print_colored("-" * 70, Colors.YELLOW)
    except:
        pass
    
    return 0 if (passed == total and verification_passed) else 1

if __name__ == "__main__":
    sys.exit(main())
