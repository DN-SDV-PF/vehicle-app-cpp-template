/**
 * Copyright (c) 2022-2025 Contributors to the Eclipse Foundation
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

#include <memory>

#include "sdk/DataPointBatch.h"
#include "sdk/IPubSubClient.h"
#include "sdk/Logger.h"
#include "sdk/QueryBuilder.h"
#include "sdk/VehicleApp.h"
#include "sdk/vdb/grpc/duo/BrokerClient.h"
#include "vehicle/Vehicle.h"

#include <csignal>
#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace example {

class SetDataPointsApp : public velocitas::VehicleApp {
public:
    SetDataPointsApp()
        : VehicleApp(std::make_shared<velocitas::duo::BrokerClient>("vehicledatabroker")) {}

    void onStart() override {
        // get a single data point
        try {
            velocitas::logger().info("Getting single data point ...");

            auto vel = Vehicle.Speed.get();
            printf("vel:%f\n", vel->await().value());

            velocitas::logger().info("Getting single data point successfully done.");
        } catch (velocitas::AsyncException& e) {
            velocitas::logger().error("Error on getting single data point: {}", e.what());
        }

        // set a single data point
        try {
            velocitas::logger().info("Setting single data point ...");

            // 以下の値を変更することでDuoに送られるSet値が変わる
            Vehicle.Speed.set(129.3F)->await();
            velocitas::logger().info("Setting single data point successfully done.");

        } catch (velocitas::AsyncException& e) {
            velocitas::logger().error("Error on setting single data point: {}", e.what());
        }

        // subscribe data
        try {
            velocitas::logger().info("Subscribing to data ...");

            // subscribeDataPoints(velocitas::QueryBuilder::select(Vehicle.Speed).build())

            subscribeDataPoints(Vehicle.Speed.getPath())
                ->onItem([this](const auto& item) {
                    try {
                        auto ValuePtr =
                            item.template get<decltype(this->Vehicle.Speed)>(this->Vehicle.Speed);
                        if (!ValuePtr) {
                            // do nothing
                            return;
                        }
                        velocitas::logger().info("Received Vehicle.Speed update: {}",
                                                 ValuePtr->value());
                    } catch (const std::exception& exception) {
                        velocitas::logger().warn("Failed to process Vehicle.Speed update: {}",
                                                 exception.what());
                    }
                })
                ->onError([](const auto& status) {
                    velocitas::logger().error("Subscription error: {}", status.errorMessage());
                });

            velocitas::logger().info("Subscribing to data successfully done.");

        } catch (velocitas::AsyncException& e) {
            velocitas::logger().error("Error on subscribing to data: {}", e.what());
        }

        velocitas::logger().info("Done. (Press Ctrl+C to terminate the app.)");
    }

private:
    velocitas::Vehicle Vehicle;
};

} // namespace example

std::unique_ptr<example::SetDataPointsApp> myApp;

void signal_handler(int sig) {
    velocitas::logger().info("App terminating signal received: {}", sig);
    myApp->stop();
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_handler);

    myApp = std::make_unique<example::SetDataPointsApp>();
    myApp->run();
    return 0;
}
