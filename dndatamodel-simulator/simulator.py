#!/usr/bin/env python3
"""
DnDataModel Simulator

Generates vehicle signals based on the custom data model defined in dataModel_basic.json.
Updates signals to the Quad broker via REST API at http://localhost:50050/reports/vss
with dndatamodel namespace.
"""

import requests
import json
import time
import random
import sys
from datetime import datetime


class DnDataModelSimulator:
    """Simulator for DN custom data model signals."""
    
    def __init__(self, base_url: str = "http://localhost:50050"):
        self.base_url = base_url
        self.running = True
        
        # Initial signal values
        self.speed = 0.0
        self.speed_direction = 1  # 1 for increasing, -1 for decreasing
        
    def update_signal(self, path_dict: dict) -> dict:
        """
        Send signal update to Quad broker.
        
        Args:
            path_dict: Dictionary with nested structure for the signal path
            
        Returns:
            Response from server
        """
        url = f"{self.base_url}/reports/vss"
        headers = {"Content-Type": "application/json"}
        
        payload = {
            "item": {
                "dndatamodel": path_dict
            }
        }
        
        # Generate curl command for logging
        payload_str = json.dumps(payload)
        curl_cmd = f"curl -s -X PATCH {url} -H \"Content-Type: application/json\" -d '{payload_str}'"
        
        try:
            response = requests.patch(url, headers=headers, json=payload, timeout=5)
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            return {"error": str(e)}
    
    def generate_speed(self) -> float:
        """Generate realistic speed values with smooth transitions."""
        # Simulate acceleration and deceleration
        change = random.uniform(0.5, 3.0)
        self.speed += change * self.speed_direction
        
        # Reverse direction at boundaries
        if self.speed >= 120.0:
            self.speed = 120.0
            self.speed_direction = -1
        elif self.speed <= 0.0:
            self.speed = 0.0
            self.speed_direction = 1
        
        # Add some randomness
        self.speed += random.uniform(-1.0, 1.0)
        self.speed = max(0.0, min(120.0, self.speed))
        
        return round(self.speed, 1)
    
    def simulate_speed_signal(self):
        """Simulate Vehicle.EgoVehicle.Motion.Locomotion.Speed signal."""
        speed_value = self.generate_speed()
        
        signal_data = {
            "Vehicle": {
                "EgoVehicle": {
                    "Motion": {
                        "Locomotion": {
                            "Speed": speed_value
                        }
                    }
                }
            }
        }
        
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] 🚗 Speed: {speed_value} km/h")
        
        result = self.update_signal(signal_data)
        
        if "error" in result:
            print(f"  ❌ Error: {result['error']}")
        elif result.get("code") == "OK":
            print(f"  ✅ Updated successfully")
        else:
            print(f"  ⚠️  Unexpected response: {result}")
    
    def run(self, interval: float = 2.0):
        """
        Run the simulator continuously.
        
        Args:
            interval: Time in seconds between signal updates
        """
        print("=" * 70)
        print("DnDataModel Simulator")
        print("=" * 70)
        print(f"Base URL: {self.base_url}")
        print(f"Update Interval: {interval} seconds")
        print(f"Signal: dndatamodel/Vehicle/EgoVehicle/Motion/Locomotion/Speed")
        print("\nPress Ctrl+C to stop\n")
        
        try:
            while self.running:
                self.simulate_speed_signal()
                time.sleep(interval)
        except KeyboardInterrupt:
            print("\n\n🛑 Simulator stopped")
            sys.exit(0)
        except Exception as e:
            print(f"\n❌ Fatal error: {e}")
            sys.exit(1)


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="DnDataModel Simulator - Generate signals from custom data model"
    )
    parser.add_argument(
        "--url",
        default="http://localhost:50050",
        help="Base URL of Quad REST API (default: http://localhost:50050)"
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=2.0,
        help="Update interval in seconds (default: 2.0)"
    )
    
    args = parser.parse_args()
    
    simulator = DnDataModelSimulator(base_url=args.url)
    simulator.run(interval=args.interval)


if __name__ == "__main__":
    main()
