/**
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#ifndef VEHICLE_APP_SDK_VDB_GRPC_DUO_TYPECONVERTER_H
#define VEHICLE_APP_SDK_VDB_GRPC_DUO_TYPECONVERTER_H

#include "sdk/DataPointValue.h"

#include <google/protobuf/struct.pb.h>

#include <memory>
#include <string>

namespace velocitas {

class DataPoint;

namespace duo {

/**
 * @brief Helper responsible for translating between Velocitas DataPoint values and
 * Duo gRPC representations.
 */
class DuoTypeConverter {
public:
    static google::protobuf::Value toDuoValue(const DataPointValue& dataPointValue);

    static std::shared_ptr<DataPointValue> fromDuoValue(const std::string&             path,
                                                        DataPointValue::Type           expectedType,
                                                        const google::protobuf::Value& value,
                                                        const Timestamp& timestamp = Timestamp{});

    static std::shared_ptr<DataPointValue> fromDuoValue(const DataPoint&               dataPoint,
                                                        const google::protobuf::Value& value,
                                                        const Timestamp& timestamp = Timestamp{});

    static std::string toDuoPath(const std::string& path);
    static std::string toInternalPath(const std::string& path);
};

} // namespace duo

} // namespace velocitas

#endif // VEHICLE_APP_SDK_VDB_GRPC_DUO_TYPECONVERTER_H
