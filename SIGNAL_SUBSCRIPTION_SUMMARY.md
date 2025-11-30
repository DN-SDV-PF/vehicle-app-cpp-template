# SampleApp Signal Subscription Summary

## Overview
SampleApp has been updated to subscribe to a comprehensive set of vehicle signals covering Powertrain, HVAC, Cabin, and Infotainment systems.

## Total Signals: 27

### Motion & Speed (1 signal)
- `Vehicle.EgoVehicle.Motion.Locomotion.Speed` → `sampleapp/currentSpeed`

### Powertrain - Battery (4 signals)
- `Vehicle.EgoVehicle.Powertrain.TractionBattery.StateOfCharge.Current` → `sampleapp/batterySOC`
- `Vehicle.EgoVehicle.Powertrain.TractionBattery.CurrentVoltage` → `sampleapp/batteryVoltage`
- `Vehicle.EgoVehicle.Powertrain.TractionBattery.BatteryLevel` → `sampleapp/batteryLevel`
- `Vehicle.EgoVehicle.Powertrain.Range` → `sampleapp/range`

### Powertrain - Transmission & Fuel (2 signals)
- `Vehicle.EgoVehicle.Powertrain.Transmission.CurrentGear` → `sampleapp/currentGear`
- `Vehicle.EgoVehicle.Powertrain.FuelSystem.Level` → `sampleapp/fuelLevel`

### HVAC - Climate Control (13 signals)
- `Vehicle.EgoVehicle.Cabin.HVAC.IsAirConditioningActive` → `sampleapp/hvac/acActive`
- `Vehicle.EgoVehicle.Cabin.HVAC.AirCompressor.IsAirCompressorOn` → `sampleapp/hvac/airCompressor`
- `Vehicle.EgoVehicle.Cabin.HVAC.Sync.IsSyncOn` → `sampleapp/hvac/sync`
- `Vehicle.EgoVehicle.Cabin.HVAC.EcoMode.IsEcoModeOn` → `sampleapp/hvac/ecoMode`
- `Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier1.IsAirPurifier1On` → `sampleapp/hvac/airPurifier1`
- `Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier2.IsAirPurifier2On` → `sampleapp/hvac/airPurifier2`
- `Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier3.IsAirPurifier3On` → `sampleapp/hvac/airPurifier3`
- `Vehicle.EgoVehicle.Cabin.HVAC.PollenRemove.IsPollenRemoveOn` → `sampleapp/hvac/pollenRemove`
- `Vehicle.EgoVehicle.Cabin.HVAC.Swing.IsSwingOn` → `sampleapp/hvac/swing`
- `Vehicle.EgoVehicle.Cabin.HVAC.PmRemove.IsPmRemoveOn` → `sampleapp/hvac/pmRemove`
- `Vehicle.EgoVehicle.Cabin.HVAC.Recirculation.RecirculationState` → `sampleapp/hvac/recirculation`
- `Vehicle.EgoVehicle.Cabin.HVAC.Heater.IsHeaterOn` → `sampleapp/hvac/heater`

### Cabin - Comfort (2 signals)
- `Vehicle.EgoVehicle.Cabin.RearShade.Switch` → `sampleapp/cabin/rearShadeSwitch`
- `Vehicle.EgoVehicle.Cabin.RearShade.Position` → `sampleapp/cabin/rearShadePosition`

### Infotainment (5 signals)
- `Vehicle.EgoVehicle.Cabin.Infotainment.Media.Action` → `sampleapp/infotainment/mediaAction`
- `Vehicle.EgoVehicle.Cabin.Infotainment.Media.Played.Source` → `sampleapp/infotainment/mediaSource`
- `Vehicle.EgoVehicle.Cabin.Infotainment.Media.Volume` → `sampleapp/infotainment/mediaVolume`
- `Vehicle.EgoVehicle.Cabin.Infotainment.Media.IsOn` → `sampleapp/infotainment/mediaOn`
- `Vehicle.EgoVehicle.Cabin.Infotainment.Navigation.IsOn` → `sampleapp/infotainment/navigationOn`

## MQTT Topic Structure

### Legacy Topics (maintained for backward compatibility)
- `sampleapp/getSpeed` (read)
- `sampleapp/getSpeed/response` (write)

### Powertrain Topics
- `sampleapp/currentSpeed`
- `sampleapp/batterySOC`
- `sampleapp/batteryVoltage`
- `sampleapp/batteryLevel`
- `sampleapp/range`
- `sampleapp/currentGear`
- `sampleapp/fuelLevel`

### HVAC Topics
- `sampleapp/hvac/acActive`
- `sampleapp/hvac/airCompressor`
- `sampleapp/hvac/sync`
- `sampleapp/hvac/ecoMode`
- `sampleapp/hvac/airPurifier1`
- `sampleapp/hvac/airPurifier2`
- `sampleapp/hvac/airPurifier3`
- `sampleapp/hvac/pollenRemove`
- `sampleapp/hvac/swing`
- `sampleapp/hvac/pmRemove`
- `sampleapp/hvac/recirculation`
- `sampleapp/hvac/heater`

### Cabin Topics
- `sampleapp/cabin/rearShadeSwitch`
- `sampleapp/cabin/rearShadePosition`

### Infotainment Topics
- `sampleapp/infotainment/mediaAction`
- `sampleapp/infotainment/mediaSource`
- `sampleapp/infotainment/mediaVolume`
- `sampleapp/infotainment/mediaOn`
- `sampleapp/infotainment/navigationOn`

## Files Modified
1. `app/src/SampleApp.cpp` - Added subscriptions and handlers for all signals
2. `app/AppManifest.json` - Declared all required data points and MQTT topics

## Testing
To test the new subscriptions, you can use curl commands to update signals:

```bash
# HVAC Test
curl -X PATCH http://localhost:50050/reports/vss \
  -H "Content-Type: application/json" \
  -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"HVAC":{"IsAirConditioningActive":true}}}}}}}'

# Infotainment Test
curl -X PATCH http://localhost:50050/reports/vss \
  -H "Content-Type: application/json" \
  -d '{"item":{"dndatamodel":{"Vehicle":{"EgoVehicle":{"Cabin":{"Infotainment":{"Media":{"Volume":75}}}}}}}}'
```

Then monitor MQTT topics:
```bash
mosquitto_sub -h localhost -t 'sampleapp/#' -v
```
