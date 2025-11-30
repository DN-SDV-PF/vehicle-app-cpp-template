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

#include "BrokerClient.h"

#include "sdk/DataPointValue.h"
#include "sdk/Exceptions.h"
#include "sdk/Logger.h"
#include "sdk/vdb/grpc/duo/TypeConverter.h"

#include "sdk/middleware/Middleware.h"
#include "sdk/vdb/grpc/common/ChannelConfiguration.h"
#include "sdk/vdb/grpc/duo/BrokerAsyncGrpcFacade.h"

#include <fmt/core.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "duo/duo.grpc.pb.h"
#include <google/protobuf/struct.pb.h>
#include <thread>
#include <utility>

namespace {

std::string joinPathSegments(const std::vector<std::string>& segments) {
    std::string path;
    for (const auto& segment : segments) {
        if (segment.empty()) {
            continue;
        }
        if (!path.empty()) {
            path += '/';
        }
        path += segment;
    }
    return path;
}

std::string valueToString(const google::protobuf::Value& value) {
    if (value.has_string_value()) {
        return value.string_value();
    }
    if (value.has_number_value()) {
        return fmt::format("{}", value.number_value());
    }
    if (value.has_bool_value()) {
        return value.bool_value() ? "true" : "false";
    }
    if (value.has_null_value()) {
        return "null";
    }
    return "";
}

// Duo応答からDataPointValueに変換するヘルパー関数
std::shared_ptr<velocitas::DataPointValue>
convertDuoResponseToDataPointValue(const std::string& path, const google::protobuf::Value& value) {
    using namespace velocitas;

    // 応答の型を推定して変換
    if (value.has_string_value()) {
        return std::make_shared<TypedDataPointValue<std::string>>(path, value.string_value());
    }
    if (value.has_bool_value()) {
        return std::make_shared<TypedDataPointValue<bool>>(path, value.bool_value());
    }
    if (value.has_number_value()) {
        // 数値型はdoubleとして扱う（必要に応じて他の型に対応）
        return std::make_shared<TypedDataPointValue<double>>(path, value.number_value());
    } else if (value.has_null_value()) {
        // null値の場合は空のdouble型で返す（エラーケース）
        return std::make_shared<TypedDataPointValue<double>>(
            path, DataPointValue::Failure::NOT_AVAILABLE);
    }

    throw InvalidValueException("Unsupported value type in Duo response");
}

void collectValuePaths(const google::protobuf::Value& value, std::vector<std::string>& currentPath,
                       std::vector<std::string>&                                     pathList,
                       std::vector<std::pair<std::string, google::protobuf::Value>>& leafValues) {
    if (value.has_struct_value()) {
        for (const auto& [key, nestedValue] : value.struct_value().fields()) {
            currentPath.push_back(key);
            collectValuePaths(nestedValue, currentPath, pathList, leafValues);
            currentPath.pop_back();
        }
        return;
    }

    if (!currentPath.empty()) {
        const auto path = joinPathSegments(currentPath);
        pathList.push_back(path);
        leafValues.emplace_back(path, value);
    }
}

} // namespace

namespace velocitas::duo {

BrokerClient::BrokerClient(const std::string& vdbAddress, const std::string& vdbServiceName) {
    logger().info("Connecting to data broker service '{}' via '{}'", vdbServiceName, vdbAddress);
    m_asyncBrokerFacade = std::make_shared<BrokerAsyncGrpcFacade>(grpc::CreateCustomChannel(
        vdbAddress, grpc::InsecureChannelCredentials(), getChannelArguments()));
    Middleware::Metadata metadata = Middleware::getInstance().getMetadata(vdbServiceName);
    m_asyncBrokerFacade->setContextModifier([metadata](auto& context) {
        for (auto metadatum : metadata) {
            context.AddMetadata(metadatum.first, metadatum.second);
        }
    });
}

BrokerClient::BrokerClient(const std::string& vdbServiceName)
    : BrokerClient(Middleware::getInstance().getServiceLocation(vdbServiceName), vdbServiceName) {}

BrokerClient::~BrokerClient() {}

AsyncResultPtr_t<DataPointReply>
BrokerClient::getDatapoints(const std::vector<std::string>& datapoints) {
    auto result = std::make_shared<AsyncResult<DataPointReply>>();
    m_asyncBrokerFacade->GetDatapoints(
        datapoints,
        [result, datapoints](auto& reply) {
            // get対象データパス群の1つ目だけを扱う(複数データ一括getの対応は残件)
            auto path = datapoints[0];

            velocitas::logger().info(reply.DebugString());
            std::string str_json = reply.DebugString();
            std::string find_tgt = "item ";
            str_json             = str_json.substr(str_json.find(find_tgt) + find_tgt.length());

            DataPointMap_t dataPointsMap;

            // gRPCのprotobuf形式スキーマで定義した
            // google::protobuf::Value型のitemデータが
            // Duoからの応答データに含まれている？
            if (reply.has_item()) {
                // Duo応答をDataPointValueに変換してmapに格納
                try {
                    auto convertedValue = convertDuoResponseToDataPointValue(path, reply.item());
                    dataPointsMap[path] = convertedValue;
                } catch (const std::exception& e) {
                    velocitas::logger().warn("Failed to convert Duo response for path {}: {}", path,
                                             e.what());
                }
            }

            DataPointReply rep(std::move(dataPointsMap));
            // 互換性のため、Duo応答も保持
            rep.setDuoGetResponse(reply);

            // 作成したDataPointReply型のデータを非同期読取I/FであるAsyncResult型データに移動する。
            result->insertResult(std::move(rep));
        },
        [result](auto status) {
            result->insertError(
                Status(fmt::format("RPC 'GetDatapoints' failed: {}", status.error_message())));
        });
    return result;
}

AsyncResultPtr_t<IVehicleDataBrokerClient::SetErrorMap_t>
BrokerClient::setDatapoints(const std::vector<std::unique_ptr<DataPointValue>>& datapoints) {
    auto result = std::make_shared<AsyncResult<SetErrorMap_t>>();

    std::map<std::string, google::protobuf::Value> datapoints_map;
    for (const auto& datapoint : datapoints) {
        std::string path  = datapoint->getPath();
        auto        value = DuoTypeConverter::toDuoValue(*datapoint);
        datapoints_map.emplace(std::move(path), std::move(value));
    }

    m_asyncBrokerFacade->SetDatapoints(
        datapoints_map,
        [result](auto& reply) {
            velocitas::logger().info(reply.DebugString());
            std::string str_json = reply.DebugString();
            std::string find_tgt = "item ";
            str_json             = str_json.substr(str_json.find(find_tgt) + find_tgt.length());

            result->insertResult(SetErrorMap_t{});
        },
        [result](auto status) {
            result->insertError(
                Status(fmt::format("RPC 'SetDatapoints' failed: {}", status.error_message())));
        });
    return result;
}

AsyncSubscriptionPtr_t<DataPointReply> BrokerClient::subscribe(const std::string& query) {
    auto subscription = std::make_shared<AsyncSubscription<DataPointReply>>();

    // duo用パスに変換してsubscribe

    std::vector<std::string> targets;
    targets.emplace_back(DuoTypeConverter::toDuoPath(query));

    m_asyncBrokerFacade->Subscribe(
        targets,
        [subscription, query](const auto& item) {
            std::vector<std::string>                                     pathList;
            std::vector<std::pair<std::string, google::protobuf::Value>> pathValueList;

            const google::protobuf::ListValue& listValue = item.items();

            for (const google::protobuf::Value& value : listValue.values()) {
                if (value.has_number_value()) {
                    std::cout << "Number: " << value.number_value() << std::endl; // DEBUG
                    pathValueList.emplace_back(query, value);
                } else if (value.has_string_value()) {
                    std::cout << "String: " << value.string_value() << std::endl; // DEBUG
                    pathValueList.emplace_back(query, value);
                } else if (value.has_struct_value()) {
                    std::vector<std::string>                                     currentPath;
                    std::vector<std::pair<std::string, google::protobuf::Value>> leafValues;
                    collectValuePaths(value, currentPath, pathList, leafValues);

                    for (const auto& [path, leaf] : leafValues) {
                        velocitas::logger().info("Received update. path: {} value: {}", path,
                                                 valueToString(leaf)); // DEBUG

                        pathValueList.emplace_back(path, leaf);
                    }
                }
            }

            for (const auto& [path, leaf] : pathValueList) {
                DataPointReply reply;
                auto           response = ::duo::GetResponse{};
                response.mutable_item()->CopyFrom(leaf);
                reply.setDuoGetResponse(std::move(response));
                subscription->insertNewItem(std::move(reply));
            }
        },
        [subscription](const auto& status) {
            subscription->insertError(
                Status(fmt::format("RPC 'Subscribe' failed: {}", status.error_message())));
        });

    return subscription;
}
} // namespace velocitas::duo
