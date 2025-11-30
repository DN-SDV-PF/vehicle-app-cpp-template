#!/usr/bin/env python3
"""
DnDataModel Simulator with Web UI

Generates vehicle signals based on the custom data model defined in dataModel_basic.json.
Provides a web interface on port 8081 for monitoring and control.
"""

import requests
import json
import time
import random
import sys
import threading
from datetime import datetime
from flask import Flask, render_template, jsonify, request
from flask_cors import CORS


class DnDataModelSimulator:
    """Simulator for DN custom data model signals."""
    
    def __init__(self, base_url: str = "http://localhost:50050"):
        self.base_url = base_url
        self.running = False
        self.paused = False
        self.interval = 2.0
        
        # Initial signal values
        self.speed = 0.0
        self.speed_direction = 1  # 1 for increasing, -1 for decreasing
        
        # Statistics
        self.update_count = 0
        self.error_count = 0
        self.last_update_time = None
        self.last_speed = 0.0
        
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
        
        try:
            response = requests.patch(url, headers=headers, json=payload, timeout=5)
            response.raise_for_status()
            self.update_count += 1
            return response.json()
        except requests.exceptions.RequestException as e:
            self.error_count += 1
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
        self.last_speed = speed_value
        
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
        self.last_update_time = timestamp
        print(f"[{timestamp}] 🚗 Speed: {speed_value} km/h")
        
        result = self.update_signal(signal_data)
        
        if "error" in result:
            print(f"  ❌ Error: {result['error']}")
        elif result.get("code") == "OK":
            print(f"  ✅ Updated successfully")
        else:
            print(f"  ⚠️  Unexpected response: {result}")
    
    def run_loop(self):
        """Main simulation loop."""
        while self.running:
            if not self.paused:
                try:
                    self.simulate_speed_signal()
                except Exception as e:
                    print(f"Error in simulation: {e}")
                    self.error_count += 1
            time.sleep(self.interval)
    
    def start(self):
        """Start the simulator."""
        if not self.running:
            self.running = True
            self.paused = False
            thread = threading.Thread(target=self.run_loop, daemon=True)
            thread.start()
            print("✅ Simulator started")
    
    def stop(self):
        """Stop the simulator."""
        self.running = False
        self.paused = False
        print("🛑 Simulator stopped")
    
    def pause(self):
        """Pause the simulator."""
        self.paused = True
        print("⏸️  Simulator paused")
    
    def resume(self):
        """Resume the simulator."""
        self.paused = False
        print("▶️  Simulator resumed")
    
    def set_speed_manual(self, speed: float):
        """Manually set speed value."""
        self.speed = max(0.0, min(120.0, speed))
        self.last_speed = self.speed
        
        signal_data = {
            "Vehicle": {
                "EgoVehicle": {
                    "Motion": {
                        "Locomotion": {
                            "Speed": self.speed
                        }
                    }
                }
            }
        }
        
        result = self.update_signal(signal_data)
        self.last_update_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        return result
    
    def get_status(self):
        """Get current simulator status."""
        return {
            "running": self.running,
            "paused": self.paused,
            "interval": self.interval,
            "current_speed": self.last_speed,
            "update_count": self.update_count,
            "error_count": self.error_count,
            "last_update_time": self.last_update_time,
            "base_url": self.base_url
        }


# Flask Web Application
app = Flask(__name__)
CORS(app)

# Global simulator instance
simulator = None


@app.route('/')
def index():
    """Serve the main page."""
    return render_template('index.html')


@app.route('/api/status')
def get_status():
    """Get simulator status."""
    return jsonify(simulator.get_status())


@app.route('/api/start', methods=['POST'])
def start_simulator():
    """Start the simulator."""
    simulator.start()
    return jsonify({"status": "started"})


@app.route('/api/stop', methods=['POST'])
def stop_simulator():
    """Stop the simulator."""
    simulator.stop()
    return jsonify({"status": "stopped"})


@app.route('/api/pause', methods=['POST'])
def pause_simulator():
    """Pause the simulator."""
    simulator.pause()
    return jsonify({"status": "paused"})


@app.route('/api/resume', methods=['POST'])
def resume_simulator():
    """Resume the simulator."""
    simulator.resume()
    return jsonify({"status": "resumed"})


@app.route('/api/interval', methods=['POST'])
def set_interval():
    """Set update interval."""
    data = request.json
    interval = float(data.get('interval', 2.0))
    simulator.interval = max(0.1, min(60.0, interval))
    return jsonify({"interval": simulator.interval})


@app.route('/api/speed', methods=['POST'])
def set_speed():
    """Manually set speed value."""
    data = request.json
    speed = float(data.get('speed', 0.0))
    result = simulator.set_speed_manual(speed)
    return jsonify(result)


@app.route('/update_speed', methods=['POST'])
def update_speed():
    """Update speed value (frontend endpoint)."""
    data = request.json
    speed = float(data.get('speed', 0.0))
    result = simulator.set_speed_manual(speed)
    if "error" in result:
        return jsonify({"success": False, "message": str(result.get("error"))})
    else:
        return jsonify({"success": True, "speed": speed})


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="DnDataModel Simulator - Generate signals from custom data model"
    )
    parser.add_argument(
        "--quad-url",
        default="http://localhost:50050",
        help="Base URL of Quad REST API (default: http://localhost:50050)"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=8081,
        help="Web UI port (default: 8081)"
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=2.0,
        help="Update interval in seconds (default: 2.0)"
    )
    parser.add_argument(
        "--autostart",
        action="store_true",
        help="Automatically start simulator on launch"
    )
    
    args = parser.parse_args()
    
    global simulator
    simulator = DnDataModelSimulator(base_url=args.quad_url)
    simulator.interval = args.interval
    
    if args.autostart:
        simulator.start()
    
    print("=" * 70)
    print("DnDataModel Simulator with Web UI")
    print("=" * 70)
    print(f"Quad Broker: {args.quad_url}")
    print(f"Web UI: http://localhost:{args.port}")
    print(f"Update Interval: {args.interval} seconds")
    print("\nPress Ctrl+C to stop\n")
    
    try:
        app.run(host='0.0.0.0', port=args.port, debug=False)
    except KeyboardInterrupt:
        print("\n\n🛑 Shutting down...")
        simulator.stop()
        sys.exit(0)


if __name__ == "__main__":
    main()
