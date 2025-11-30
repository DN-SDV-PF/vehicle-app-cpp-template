/**
 * Copyright (c) 2023-2025 Contributors to the Eclipse Foundation
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SampleApp.h"
#include "sdk/IPubSubClient.h"
#include "sdk/Logger.h"
#include "sdk/QueryBuilder.h"
#include "sdk/vdb/IVehicleDataBrokerClient.h"

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <utility>

namespace example {

const auto GET_SPEED_REQUEST_TOPIC       = "sampleapp/getSpeed";
const auto GET_SPEED_RESPONSE_TOPIC      = "sampleapp/getSpeed/response";
const auto DATABROKER_SUBSCRIPTION_TOPIC = "sampleapp/currentSpeed";

SampleApp::SampleApp()
    : VehicleApp(velocitas::IVehicleDataBrokerClient::createInstance("quadgrpc"),
                 velocitas::IPubSubClient::createInstance("SampleApp")) {}

void SampleApp::onStart() {
    // This method will be called by the SDK when the connection to the
    // Vehicle DataBroker is ready.
    // Here you can subscribe for the Vehicle Signals update.

    velocitas::logger().info("Subscribing to key vehicle data points...");

    // Define key sensors to subscribe to
    std::vector<std::string> keySignals = {
        // Motion & Speed
        "dndatamodel/Vehicle.EgoVehicle.Motion.Locomotion.Speed",

        // Powertrain - Battery
        "dndatamodel/Vehicle.EgoVehicle.Powertrain.TractionBattery.StateOfCharge.Current",
        "dndatamodel/Vehicle.EgoVehicle.Powertrain.TractionBattery.CurrentVoltage",
        "dndatamodel/Vehicle.EgoVehicle.Powertrain.TractionBattery.BatteryLevel",
        "dndatamodel/Vehicle.EgoVehicle.Powertrain.Range",

        // Powertrain - Transmission & Fuel
        "dndatamodel/Vehicle.EgoVehicle.Powertrain.Transmission.CurrentGear",
        "dndatamodel/Vehicle.EgoVehicle.Powertrain.FuelSystem.Level",

        // HVAC - Climate Control
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.IsAirConditioningActive",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.AirCompressor.IsAirCompressorOn",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.Sync.IsSyncOn",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.EcoMode.IsEcoModeOn",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier1.IsAirPurifier1On",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier2.IsAirPurifier2On",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier3.IsAirPurifier3On",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.PollenRemove.IsPollenRemoveOn",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.Swing.IsSwingOn",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.PmRemove.IsPmRemoveOn",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.Recirculation.RecirculationState",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.HVAC.Heater.IsHeaterOn",

        // Cabin - Shade & Comfort
        "dndatamodel/Vehicle.EgoVehicle.Cabin.RearShade.Switch",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.RearShade.Position",

        // Infotainment - Media
        "dndatamodel/Vehicle.EgoVehicle.Cabin.Infotainment.Media.Action",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.Infotainment.Media.Played.Source",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.Infotainment.Media.Volume",
        "dndatamodel/Vehicle.EgoVehicle.Cabin.Infotainment.Media.IsOn",

        // Infotainment - Navigation
        "dndatamodel/Vehicle.EgoVehicle.Cabin.Infotainment.Navigation.IsOn"};

    // Subscribe to each key signal
    for (const auto& signalPath : keySignals) {
        velocitas::logger().info("Subscribing to: {}", signalPath);

        subscribeDataPoints(signalPath)
            ->onItem([this, signalPath](const auto& item) {
                try {
                    onDataPointChanged(signalPath, item);

                    // Special handling for Speed (backward compatibility)
                    if (signalPath.find("Speed") != std::string::npos) {
                        onSpeedChanged(item);
                    }
                } catch (const std::exception& exception) {
                    velocitas::logger().error("Failed to process {} update: {}", signalPath,
                                              exception.what());
                }
            })
            ->onError([this, signalPath](auto&& status) {
                velocitas::logger().error("{} subscription error: {}", signalPath,
                                          status.errorMessage());
                onError(std::forward<decltype(status)>(status));
            });
    }

    velocitas::logger().info("All subscription requests sent");

    // ... and, unlike Python, you have to manually subscribe to pub/sub topics
    subscribeToTopic(GET_SPEED_REQUEST_TOPIC)
        ->onItem(
            [this](auto&& data) { onGetSpeedRequestReceived(std::forward<decltype(data)>(data)); })
        ->onError([this](auto&& status) { onError(std::forward<decltype(status)>(status)); });
}
void SampleApp::onDataPointChanged(const std::string&               signalPath,
                                   const velocitas::DataPointReply& reply) {
    // Generic handler for all data point changes
    velocitas::logger().info("=== Data Point Change Detected ===");
    velocitas::logger().info("Signal Path: {}", signalPath);

    // Extract value and publish to MQTT based on signal type
    try {
        nlohmann::json json;
        std::string    topicName;

        // Motion & Speed
        if (signalPath.find("Speed") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Motion.Locomotion.Speed)->value();
            json       = {{"speed", value}};
            topicName  = "sampleapp/currentSpeed";
        }
        // Battery signals
        else if (signalPath.find("StateOfCharge.Current") != std::string::npos) {
            auto value =
                reply.get(Vehicle.EgoVehicle.Powertrain.TractionBattery.StateOfCharge.Current)
                    ->value();
            json      = {{"battery_soc", value}};
            topicName = "sampleapp/batterySOC";
        } else if (signalPath.find("CurrentVoltage") != std::string::npos) {
            auto value =
                reply.get(Vehicle.EgoVehicle.Powertrain.TractionBattery.CurrentVoltage)->value();
            json      = {{"battery_voltage", value}};
            topicName = "sampleapp/batteryVoltage";
        } else if (signalPath.find("BatteryLevel") != std::string::npos) {
            auto value =
                reply.get(Vehicle.EgoVehicle.Powertrain.TractionBattery.BatteryLevel)->value();
            json      = {{"battery_level", value}};
            topicName = "sampleapp/batteryLevel";
        } else if (signalPath.find("Powertrain.Range") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Powertrain.Range)->value();
            json       = {{"range", value}};
            topicName  = "sampleapp/range";
        }
        // Transmission & Fuel
        else if (signalPath.find("CurrentGear") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Powertrain.Transmission.CurrentGear)->value();
            json       = {{"current_gear", value}};
            topicName  = "sampleapp/currentGear";
        } else if (signalPath.find("FuelSystem.Level") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Powertrain.FuelSystem.Level)->value();
            json       = {{"fuel_level", value}};
            topicName  = "sampleapp/fuelLevel";
        }
        // HVAC signals
        else if (signalPath.find("HVAC.IsAirConditioningActive") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.HVAC.IsAirConditioningActive)->value();
            json       = {{"ac_active", value}};
            topicName  = "sampleapp/hvac/acActive";
        } else if (signalPath.find("IsAirCompressorOn") != std::string::npos) {
            auto value =
                reply.get(Vehicle.EgoVehicle.Cabin.HVAC.AirCompressor.IsAirCompressorOn)->value();
            json      = {{"air_compressor", value}};
            topicName = "sampleapp/hvac/airCompressor";
        } else if (signalPath.find("Sync.IsSyncOn") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.HVAC.Sync.IsSyncOn)->value();
            json       = {{"sync", value}};
            topicName  = "sampleapp/hvac/sync";
        } else if (signalPath.find("EcoMode.IsEcoModeOn") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.HVAC.EcoMode.IsEcoModeOn)->value();
            json       = {{"eco_mode", value}};
            topicName  = "sampleapp/hvac/ecoMode";
        } else if (signalPath.find("AirPurifier1.IsAirPurifier1On") != std::string::npos) {
            auto value =
                reply.get(Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier1.IsAirPurifier1On)->value();
            json      = {{"air_purifier1", value}};
            topicName = "sampleapp/hvac/airPurifier1";
        } else if (signalPath.find("AirPurifier2.IsAirPurifier2On") != std::string::npos) {
            auto value =
                reply.get(Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier2.IsAirPurifier2On)->value();
            json      = {{"air_purifier2", value}};
            topicName = "sampleapp/hvac/airPurifier2";
        } else if (signalPath.find("AirPurifier3.IsAirPurifier3On") != std::string::npos) {
            auto value =
                reply.get(Vehicle.EgoVehicle.Cabin.HVAC.AirPurifier3.IsAirPurifier3On)->value();
            json      = {{"air_purifier3", value}};
            topicName = "sampleapp/hvac/airPurifier3";
        } else if (signalPath.find("PollenRemove.IsPollenRemoveOn") != std::string::npos) {
            auto value =
                reply.get(Vehicle.EgoVehicle.Cabin.HVAC.PollenRemove.IsPollenRemoveOn)->value();
            json      = {{"pollen_remove", value}};
            topicName = "sampleapp/hvac/pollenRemove";
        } else if (signalPath.find("Swing.IsSwingOn") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.HVAC.Swing.IsSwingOn)->value();
            json       = {{"swing", value}};
            topicName  = "sampleapp/hvac/swing";
        } else if (signalPath.find("PmRemove.IsPmRemoveOn") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.HVAC.PmRemove.IsPmRemoveOn)->value();
            json       = {{"pm_remove", value}};
            topicName  = "sampleapp/hvac/pmRemove";
        } else if (signalPath.find("Recirculation.RecirculationState") != std::string::npos) {
            auto value =
                reply.get(Vehicle.EgoVehicle.Cabin.HVAC.Recirculation.RecirculationState)->value();
            json      = {{"recirculation", value}};
            topicName = "sampleapp/hvac/recirculation";
        } else if (signalPath.find("Heater.IsHeaterOn") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.HVAC.Heater.IsHeaterOn)->value();
            json       = {{"heater", value}};
            topicName  = "sampleapp/hvac/heater";
        }
        // Cabin signals
        else if (signalPath.find("RearShade.Switch") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.RearShade.Switch)->value();
            json       = {{"rear_shade_switch", value}};
            topicName  = "sampleapp/cabin/rearShadeSwitch";
        } else if (signalPath.find("RearShade.Position") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.RearShade.Position)->value();
            json       = {{"rear_shade_position", value}};
            topicName  = "sampleapp/cabin/rearShadePosition";
        }
        // Infotainment - Media
        else if (signalPath.find("Media.Action") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.Infotainment.Media.Action)->value();
            json       = {{"media_action", value}};
            topicName  = "sampleapp/infotainment/mediaAction";
        } else if (signalPath.find("Media.Played.Source") != std::string::npos) {
            auto value =
                reply.get(Vehicle.EgoVehicle.Cabin.Infotainment.Media.Played.Source)->value();
            json      = {{"media_source", value}};
            topicName = "sampleapp/infotainment/mediaSource";
        } else if (signalPath.find("Media.Volume") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.Infotainment.Media.Volume)->value();
            json       = {{"media_volume", value}};
            topicName  = "sampleapp/infotainment/mediaVolume";
        } else if (signalPath.find("Media.IsOn") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.Infotainment.Media.IsOn)->value();
            json       = {{"media_on", value}};
            topicName  = "sampleapp/infotainment/mediaOn";
        } else if (signalPath.find("Navigation.IsOn") != std::string::npos) {
            auto value = reply.get(Vehicle.EgoVehicle.Cabin.Infotainment.Navigation.IsOn)->value();
            json       = {{"navigation_on", value}};
            topicName  = "sampleapp/infotainment/navigationOn";
        }

        if (!topicName.empty()) {
            auto jsonString = json.dump();
            velocitas::logger().debug(R"(Publish on topic "{}": "{}")", topicName, jsonString);
            publishToTopic(topicName, jsonString);
            velocitas::logger().info("Data point {} has been updated", signalPath);
        }
    } catch (const std::exception& e) {
        velocitas::logger().warn("Could not extract value from {}: {}", signalPath, e.what());
    }
}

void SampleApp::onSpeedChanged(const velocitas::DataPointReply& reply) {
    // Get the current vehicle speed value from the received DatapointReply.
    // The DatapointReply containes the values of all subscribed DataPoints of
    // the same callback.
    auto vehicleSpeed = reply.get(Vehicle.EgoVehicle.Motion.Locomotion.Speed)->value();

    // Log when speed changes
    velocitas::logger().info("*** Speed changed detected! New speed: {} km/h ***", vehicleSpeed);

    // Do anything with the received value.
    // Example:
    // - Publish the current speed to MQTT Topic (i.e. DATABROKER_SUBSCRIPTION_TOPIC).
    nlohmann::json json({{"speed", vehicleSpeed}});
    publishToTopic(DATABROKER_SUBSCRIPTION_TOPIC, json.dump());
}

void SampleApp::onGetSpeedRequestReceived(const std::string& data) {
    // The subscribe_topic annotation is used to subscribe for incoming
    // PubSub events, e.g. MQTT event for GET_SPEED_REQUEST_TOPIC.

    // Use the logger with the preferred log level (e.g. debug, info, error, etc)
    velocitas::logger().debug("PubSub event for the Topic: {} -> is received with the data: {}",
                              GET_SPEED_REQUEST_TOPIC, data);

    // Getting current speed from VehicleDataBroker using the DataPoint getter.
    auto vehicleSpeed = Vehicle.EgoVehicle.Motion.Locomotion.Speed.get()->await().value();

    // Do anything with the speed value.
    // Example:
    // - Publish the vehicle speed to MQTT topic (i.e. GET_SPEED_RESPONSE_TOPIC).
    nlohmann::json response(
        {{"result",
          {{"status", 0}, {"message", fmt::format("Current Speed = {}", vehicleSpeed)}}}});
    publishToTopic(GET_SPEED_RESPONSE_TOPIC, response.dump());
}

void SampleApp::onError(const velocitas::Status& status) {
    velocitas::logger().error("Error occurred during async invocation: {}", status.errorMessage());
}

} // namespace example
