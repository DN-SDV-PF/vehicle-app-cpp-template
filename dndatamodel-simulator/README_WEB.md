# DnDataModel Simulator with Web UI

Automated signal generator for the custom DN data model defined in `dataModel_basic.json` with a web-based control interface on **port 8081**.

## Overview

This simulator generates realistic vehicle signals and sends them to the Quad/Duo broker via REST API. It includes a modern web UI on port 8081 for monitoring and control.

### Features

- 🚗 **Realistic Speed Generation**: Smooth acceleration/deceleration between 0-120 km/h
- 🌐 **Web UI on Port 8081**: Beautiful dashboard for monitoring and control
- 🎛️ **Interactive Controls**: Start/stop/pause/resume via web interface
- 📊 **Real-time Statistics**: Update counts, errors, current values
- 🎯 **Manual Override**: Set speed manually via web UI
- ⏱️ **Adjustable Interval**: Change update frequency on the fly (0.5-10 seconds)
- 🐳 **Docker Support**: Easy deployment with Docker Compose
- 📡 **REST API Integration**: Direct updates to Quad broker

## Architecture

```
┌─────────────────────────┐
│   Web Browser           │
│   http://localhost:8081 │
└───────────┬─────────────┘
            │ HTTP/REST API
            ↓
┌─────────────────────────┐
│ DnDataModel Simulator   │
│   + Flask Web Server    │
│   (Python 3.9)          │
│   Port 8081             │
└───────────┬─────────────┘
            │ REST API (PATCH)
            │ http://localhost:50050/reports/vss
            ↓
┌─────────────────────────┐
│   Quad/Duo Broker       │
│   (port 50050/50051)    │
└───────────┬─────────────┘
            │ gRPC Subscribe
            ↓
┌─────────────────────────┐
│   SampleApp (C++)       │
│   Vehicle Application   │
└─────────────────────────┘
```

## Quick Start

### Docker Method (Recommended)

1. **Build the Docker image**:
   ```bash
   cd /workspaces/vehicle-app-cpp-template/dndatamodel-simulator
   ./build.sh
   ```

2. **Start the simulator**:
   ```bash
   docker-compose up -d
   ```

3. **Open Web UI**:
   Open your browser to **`http://localhost:8081`**

4. **View logs**:
   ```bash
   docker-compose logs -f dndatamodel-simulator
   ```

5. **Stop the simulator**:
   ```bash
   docker-compose down
   ```

### Direct Python Method

```bash
# Install dependencies
pip install requests flask flask-cors

# Run simulator with web UI
python simulator_web.py --quad-url http://localhost:50050 --port 8081 --interval 2.0 --autostart
```

## Web UI Features

Access the web interface at **`http://localhost:8081`**

### Dashboard Components

1. **Status Panel**: 
   - Real-time simulator state (Running/Paused/Stopped)
   - Color-coded indicators (Green=Running, Orange=Paused, Red=Stopped)
   - Update count and error tracking
   - Last update timestamp

2. **Speed Display**: 
   - Large, easy-to-read current speed value
   - Real-time updates with pulse animation
   - Signal path display

3. **Control Buttons**:
   - ▶️ **Start**: Begin signal generation
   - ⏸️ **Pause**: Temporarily stop without losing state
   - ▶️ **Resume**: Continue from paused state
   - ⏹️ **Stop**: Completely stop simulator

4. **Interval Slider**: 
   - Adjust update frequency from 0.5 to 10 seconds
   - Real-time adjustment without restarting

5. **Manual Speed Control**: 
   - Override automatic speed generation
   - Set specific speed for testing (0-120 km/h)
   - Useful for edge case testing

6. **Activity Log**: 
   - Real-time log of all simulator actions
   - Timestamps for each event
   - Auto-scrolling with 50-entry limit

### REST API Endpoints

The simulator exposes the following API endpoints:

- `GET /` - Web UI dashboard
- `GET /api/status` - Get current simulator status (JSON)
- `POST /api/start` - Start the simulator
- `POST /api/stop` - Stop the simulator
- `POST /api/pause` - Pause signal generation
- `POST /api/resume` - Resume signal generation
- `POST /api/interval` - Set update interval (JSON body: `{"interval": 2.0}`)
- `POST /api/speed` - Manually set speed value (JSON body: `{"speed": 50.0}`)

Example API usage with curl:
```bash
# Get status
curl http://localhost:8081/api/status

# Start simulator
curl -X POST http://localhost:8081/api/start

# Set speed to 75 km/h
curl -X POST http://localhost:8081/api/speed \
     -H "Content-Type: application/json" \
     -d '{"speed": 75.0}'

# Change interval to 1 second
curl -X POST http://localhost:8081/api/interval \
     -H "Content-Type: application/json" \
     -d '{"interval": 1.0}'
```

## Command-Line Options

```bash
python simulator_web.py [OPTIONS]
```

Options:
- `--quad-url URL`: Base URL of Quad REST API (default: `http://localhost:50050`)
- `--port PORT`: Web UI port (default: `8081`)
- `--interval SECONDS`: Update interval in seconds (default: `2.0`)
- `--autostart`: Automatically start simulator on launch

Examples:
```bash
# Start with custom interval and port
python simulator_web.py --port 9000 --interval 1.0

# Start with autostart enabled
python simulator_web.py --autostart

# Connect to remote Quad broker
python simulator_web.py --quad-url http://192.168.1.100:50050
```

## Signal Format

The simulator sends data to the Quad broker in the following JSON format:

```json
{
  "item": {
    "dndatamodel": {
      "Vehicle": {
        "EgoVehicle": {
          "Motion": {
            "Locomotion": {
              "Speed": 65.3
            }
          }
        }
      }
    }
  }
}
```

This matches the path structure:
```
dndatamodel/Vehicle/EgoVehicle/Motion/Locomotion/Speed
```

## Integration with SampleApp

The SampleApp subscribes to the signal path:
```
dndatamodel/Vehicle.EgoVehicle.Motion.Locomotion.Speed
```

When the simulator sends updates, SampleApp's `onSpeedChanged()` callback is triggered automatically.

### Complete Testing Flow

1. **Start Quad local runtime**:
   ```bash
   velocitas exec runtime-local up
   ```

2. **Build and run SampleApp**:
   ```bash
   ./build.sh && velocitas exec runtime-local run-vehicle-app build/bin/app
   ```

3. **Start the simulator**:
   ```bash
   cd dndatamodel-simulator
   docker-compose up -d
   ```

4. **Open web UI**:
   - Navigate to `http://localhost:8081` in your browser
   - Click "Start" button or it auto-starts if `--autostart` was used

5. **Monitor the flow**:
   - Watch web UI for speed updates and statistics
   - Check SampleApp logs for speed change callbacks
   - Verify MQTT messages on `sampleapp/currentSpeed` topic

## Extending with Additional Signals

### 1. Add Signal Generation Method

Edit `simulator_web.py` and add new signal generation:

```python
def generate_humidity(self) -> float:
    """Generate realistic humidity values."""
    return round(random.uniform(30.0, 90.0), 1)

def generate_temperature(self) -> float:
    """Generate realistic temperature values."""
    return round(random.uniform(15.0, 35.0), 1)
```

### 2. Create Update Method

```python
def simulate_humidity_signal(self):
    """Simulate Vehicle.Cabin.Environment.AirQuality.Humidity signal."""
    humidity_value = self.generate_humidity()
    
    signal_data = {
        "Vehicle": {
            "Cabin": {
                "Environment": {
                    "AirQuality": {
                        "Humidity": humidity_value
                    }
                }
            }
        }
    }
    
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] 💧 Humidity: {humidity_value}%")
    
    result = self.update_signal(signal_data)
    if result.get("code") == "OK":
        print(f"  ✅ Updated successfully")
```

### 3. Add to Simulation Loop

```python
def run_loop(self):
    """Main simulation loop."""
    while self.running:
        if not self.paused:
            try:
                self.simulate_speed_signal()
                self.simulate_humidity_signal()      # Add new signal
                self.simulate_temperature_signal()   # Add another signal
            except Exception as e:
                print(f"Error in simulation: {e}")
                self.error_count += 1
        time.sleep(self.interval)
```

### 4. Update Web UI (Optional)

To display new signals in the web UI, edit `templates/index.html` and add new display panels:

```html
<div class="card">
    <h2>Current Humidity</h2>
    <div class="speed-display" id="humidity-display">
        0.0 <span class="speed-unit">%</span>
    </div>
</div>
```

And update the JavaScript to fetch and display the new values.

## Troubleshooting

### Port 8081 already in use
```bash
# Find what's using port 8081
lsof -i :8081

# Kill the process (replace PID)
kill -9 <PID>

# Or use a different port
python simulator_web.py --port 8082
```

### Simulator not connecting to Quad broker
```bash
# Check if Quad broker is running
curl http://localhost:50050

# If not running, start it
velocitas exec runtime-local up

# Check network connectivity
docker network ls
docker inspect dndatamodel-simulator
```

### No updates received by SampleApp
1. Verify signal path matches exactly:
   ```
   dndatamodel/Vehicle.EgoVehicle.Motion.Locomotion.Speed
   ```

2. Check SampleApp subscription in logs:
   ```bash
   # Look for subscription confirmation
   grep "Subscribe" <sampleapp-log>
   ```

3. Test manual update:
   ```bash
   curl -X PATCH http://localhost:50050/reports/vss \
        -H "Content-Type: application/json" \
        -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Motion":{"Locomotion":{"Speed":50.0}}}}}}}'
   ```

### Web UI not accessible
1. Check container is running:
   ```bash
   docker ps | grep dndatamodel-simulator
   ```

2. Verify port mapping:
   ```bash
   docker port dndatamodel-simulator
   # Should show: 8081/tcp -> 0.0.0.0:8081
   ```

3. Check container logs:
   ```bash
   docker-compose logs dndatamodel-simulator
   ```

4. Test from command line:
   ```bash
   curl http://localhost:8081/api/status
   ```

### Firewall issues
If accessing from another machine:
```bash
# Allow port 8081 through firewall (Ubuntu/Debian)
sudo ufw allow 8081/tcp

# Or for CentOS/RHEL
sudo firewall-cmd --add-port=8081/tcp --permanent
sudo firewall-cmd --reload
```

## Performance Notes

- **Update Interval**: Lower intervals (< 1 second) may increase CPU usage
- **Memory Usage**: ~50-80MB for Python + Flask
- **Network**: Minimal bandwidth (~1KB per update)
- **Docker Overhead**: ~20-30MB additional memory

## Security Considerations

- Web UI has no authentication (suitable for local development only)
- For production use, add authentication middleware
- Consider using HTTPS if exposing publicly
- Restrict port 8081 to localhost if not needed externally

## Files Structure

```
dndatamodel-simulator/
├── simulator_web.py        # Main simulator with Flask web server
├── simulator.py            # Original CLI-only simulator (backup)
├── Dockerfile              # Docker image definition
├── docker-compose.yml      # Docker Compose configuration
├── build.sh               # Build script
├── README_WEB.md          # This file
└── templates/
    └── index.html         # Web UI dashboard
```
