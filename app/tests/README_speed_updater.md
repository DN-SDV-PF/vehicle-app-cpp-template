# Speed Updater Tool

Interactive Python tool to send speed updates to the Quad local broker.

## Usage

### Interactive Mode

Run the script without arguments to enter interactive mode:

```bash
cd app/tests
python speed_updater.py
```

Then simply enter speed values when prompted:

```
Enter speed value (km/h): 120.5
📤 Sending speed: 120.5 km/h...
✅ Success! Speed updated to 120.5 km/h
```

### Command-Line Mode

Pass a speed value as an argument for one-time updates:

```bash
python speed_updater.py 150.0
```

### Commands

In interactive mode:
- Enter a number to send that speed value
- Type `help` for usage information
- Type `quit`, `exit`, or `q` to exit
- Press `Ctrl+C` to exit

## What it does

The tool sends a PATCH request to:
- **URL**: `http://localhost:50050/reports/vss`
- **Path**: `dndatamodel/Vehicle/EgoVehicle/Motion/Locomotion/Speed`
- **Format**: 
  ```json
  {
    "item": {
      "dndatamodel": {
        "Vehicle": {
          "EgoVehicle": {
            "Motion": {
              "Locomotion": {
                "Speed": <your_value>
              }
            }
          }
        }
      }
    }
  }
  ```

## Requirements

- Python 3.x
- `requests` library (install with: `pip install requests`)

## Example Session

```
$ python speed_updater.py
============================================================
Interactive Speed Updater for Quad Local Broker
============================================================

This tool sends speed updates to:
  URL: http://localhost:50050/reports/vss
  Path: dndatamodel/Vehicle/EgoVehicle/Motion/Locomotion/Speed

Type 'quit' or 'exit' to exit, 'help' for more info

Enter speed value (km/h): 80
📤 Sending speed: 80.0 km/h...
✅ Success! Speed updated to 80.0 km/h
   Request ID: 5a896f6f-d61e-4249-bcf7-6d8065d963f3
   Message: updated

Enter speed value (km/h): 120.5
📤 Sending speed: 120.5 km/h...
✅ Success! Speed updated to 120.5 km/h
   Request ID: 11cf3691-26ab-42de-8d5b-1461f9621ce9
   Message: updated

Enter speed value (km/h): quit
Goodbye!
```
