# DnDataModel Simulator

A vehicle signal simulator that generates data based on the custom DN data model defined in `dataModel_basic.json`.

## Overview

This simulator generates vehicle signals and sends them to the Quad broker using the REST API. The signals are organized under the `dndatamodel` thing namespace within the `vss` thing.

## Signals Generated

Currently simulates:
- `Vehicle.EgoVehicle.Motion.Locomotion.Speed` - Vehicle speed in km/h with realistic acceleration/deceleration patterns

## Architecture

- **Endpoint**: `http://localhost:50050/reports/vss`
- **Thing Structure**: `vss` thing with `dndatamodel` namespace
- **Format**: 
  ```json
  {
    "item": {
      "dndatamodel": {
        "Vehicle": {
          "EgoVehicle": {
            "Motion": {
              "Locomotion": {
                "Speed": <value>
              }
            }
          }
        }
      }
    }
  }
  ```

## Usage

### Using Docker Compose (Recommended)

1. **Build the simulator:**
   ```bash
   ./build.sh
   ```

2. **Start the simulator:**
   ```bash
   docker-compose up -d
   ```

3. **View logs:**
   ```bash
   docker-compose logs -f dndatamodel-simulator
   ```

4. **Stop the simulator:**
   ```bash
   docker-compose down
   ```

### Running Locally

1. **Install dependencies:**
   ```bash
   pip install requests
   ```

2. **Run the simulator:**
   ```bash
   python simulator.py
   ```

3. **Custom settings:**
   ```bash
   # Change update interval to 5 seconds
   python simulator.py --interval 5.0
   
   # Use different Quad broker URL
   python simulator.py --url http://192.168.1.100:50050
   ```

## Integration with Quad Local

To add this simulator to the Quad local environment:

1. The simulator uses `network_mode: "host"` to access `localhost:50050`
2. Alternatively, modify `docker-compose.yml` to use the quad network:
   ```yaml
   networks:
     - quad_default
   
   networks:
     quad_default:
       external: true
   ```
   And change the URL to `http://quad:50050`

## Extending the Simulator

To add more signals from the data model:

1. Add signal generation methods in `DnDataModelSimulator` class
2. Update the `signal_data` dictionary in the simulation loop
3. Example for adding another signal:
   ```python
   def simulate_humidity_signal(self):
       humidity_value = random.uniform(30.0, 70.0)
       
       signal_data = {
           "Vehicle": {
               "EgoVehicle": {
                   "Cabin": {
                       "Humidity": humidity_value
                   }
               }
           }
       }
       
       self.update_signal(signal_data)
   ```

## Monitoring

The simulator outputs logs showing:
- Timestamp of each update
- Current signal values
- Success/failure status of each update

Example output:
```
[2025-11-22 23:15:30] 🚗 Speed: 45.3 km/h
  ✅ Updated successfully
[2025-11-22 23:15:32] 🚗 Speed: 47.8 km/h
  ✅ Updated successfully
```

## Testing with SampleApp

1. Start the Quad local environment
2. Start this simulator
3. Run your SampleApp - it should receive speed updates automatically
4. Check SampleApp logs for "Speed subscription callback triggered!"

## Port Configuration

The simulator itself doesn't expose any ports. It acts as a client sending data to the Quad broker at port 50050.

If you want to add a web UI for the simulator (port 8081), you would need to:
1. Add a Flask/FastAPI web server to `simulator.py`
2. Expose port 8081 in the Dockerfile and docker-compose.yml
3. Create HTML/JS frontend for controlling simulation parameters
