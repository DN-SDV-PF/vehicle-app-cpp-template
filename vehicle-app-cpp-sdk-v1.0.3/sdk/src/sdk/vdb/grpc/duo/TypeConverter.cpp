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

#include "sdk/vdb/grpc/duo/TypeConverter.h"

#include "sdk/DataPoint.h"
#include "sdk/Exceptions.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace velocitas::duo {

namespace {

using google::protobuf::ListValue;
using google::protobuf::Struct;
using google::protobuf::Value;

constexpr long double kEpsilon = 1e-6L;

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> segments;
    std::size_t              start = 0;

    while (start < path.length()) {
        auto next = path.find('.', start);
        if (next == std::string::npos) {
            segments.emplace_back(path.substr(start));
            break;
        }
        if (next > start) {
            segments.emplace_back(path.substr(start, next - start));
        }
        start = next + 1;
    }
    return segments;
}

const Value* accessField(const Struct& structure, const std::string& key) {
    const auto fieldsEnd = structure.fields().end();
    auto       iter      = structure.fields().find(key);
    if (iter != fieldsEnd) {
        return &iter->second;
    }

    std::string alternative = key;
    std::replace(alternative.begin(), alternative.end(), '.', '/');
    iter = structure.fields().find(alternative);
    if (iter != fieldsEnd) {
        return &iter->second;
    }

    return nullptr;
}

const Value* locateLeaf(const Value& root, const std::vector<std::string>& segments) {
    if (!root.has_struct_value() || segments.empty()) {
        return &root;
    }

    const Value* current = &root;
    for (std::size_t index = 0; index < segments.size(); ++index) {
        if (!current->has_struct_value()) {
            return current;
        }

        const auto& segment = segments[index];
        const auto* next    = accessField(current->struct_value(), segment);
        if (next == nullptr) {
            // Attempt to match the remaining path as a single entry using '/' delimiters.
            std::string combined = segment;
            for (std::size_t remaining = index + 1; remaining < segments.size(); ++remaining) {
                combined.append("/").append(segments[remaining]);
            }
            next = accessField(current->struct_value(), combined);
        }

        if (next == nullptr) {
            return nullptr;
        }
        current = next;
    }
    return current;
}

std::string toLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

double convertToDouble(const Value& value) {
    if (value.has_number_value()) {
        return value.number_value();
    }
    if (value.has_bool_value()) {
        return value.bool_value() ? 1.0 : 0.0;
    }
    if (value.has_string_value()) {
        try {
            std::size_t idx = 0;
            double      dbl = std::stod(value.string_value(), &idx);
            if (idx != value.string_value().length()) {
                throw InvalidValueException("Non numeric characters encountered while parsing "
                                            "floating point value.");
            }
            return dbl;
        } catch (const std::exception&) {
            throw InvalidValueException("Failed to parse floating point value from string.");
        }
    }
    throw InvalidValueException("Unsupported value type for floating point conversion.");
}

std::string convertToString(const Value& value) {
    if (value.has_string_value()) {
        return value.string_value();
    }
    if (value.has_bool_value()) {
        return value.bool_value() ? "true" : "false";
    }
    if (value.has_number_value()) {
        std::ostringstream stream;
        stream << value.number_value();
        return stream.str();
    }
    if (value.has_null_value()) {
        return {};
    }
    throw InvalidValueException("Unsupported value type for string conversion.");
}

template <typename T> T convertToIntegral(const Value& value) {
    static_assert(std::is_integral_v<T>, "Integral type expected");

    if (value.has_string_value()) {
        const auto& str = value.string_value();
        std::size_t idx = 0;
        try {
            if constexpr (std::is_unsigned_v<T>) {
                auto parsed = std::stoull(str, &idx, 0);
                if (idx != str.length()) {
                    throw InvalidValueException("Non numeric characters encountered while parsing "
                                                "integer value.");
                }
                if (parsed > static_cast<unsigned long long>(std::numeric_limits<T>::max())) {
                    throw InvalidValueException("Unsigned integer value out of range.");
                }
                return static_cast<T>(parsed);
            } else {
                auto parsed = std::stoll(str, &idx, 0);
                if (idx != str.length()) {
                    throw InvalidValueException("Non numeric characters encountered while parsing "
                                                "integer value.");
                }
                if (parsed < static_cast<long long>(std::numeric_limits<T>::min()) ||
                    parsed > static_cast<long long>(std::numeric_limits<T>::max())) {
                    throw InvalidValueException("Signed integer value out of range.");
                }
                return static_cast<T>(parsed);
            }
        } catch (const std::invalid_argument&) {
            throw InvalidValueException("Failed to parse integer value from string.");
        } catch (const std::out_of_range&) {
            throw InvalidValueException("Integer value out of range.");
        }
    }

    if (value.has_bool_value()) {
        return static_cast<T>(value.bool_value());
    }

    if (value.has_number_value()) {
        const long double number = value.number_value();
        if (!std::isfinite(static_cast<double>(number))) {
            throw InvalidValueException(
                "Non finite number encountered while converting to integer.");
        }
        const long double rounded = std::nearbyintl(number);
        if (rounded < static_cast<long double>(std::numeric_limits<T>::min()) ||
            rounded > static_cast<long double>(std::numeric_limits<T>::max())) {
            throw InvalidValueException("Integer value out of range.");
        }
        return static_cast<T>(rounded);
    }

    throw InvalidValueException("Unsupported value type for integer conversion.");
}

bool convertToBool(const Value& value) {
    if (value.has_bool_value()) {
        return value.bool_value();
    }
    if (value.has_number_value()) {
        return std::fabs(value.number_value()) > kEpsilon;
    }
    if (value.has_string_value()) {
        const auto lowered = toLowerCopy(value.string_value());
        if (lowered == "true" || lowered == "1") {
            return true;
        }
        if (lowered == "false" || lowered == "0") {
            return false;
        }
    }
    throw InvalidValueException("Unsupported value type for boolean conversion.");
}

template <typename T, typename Converter>
std::vector<T> convertList(const Value& value, Converter&& converter) {
    if (!value.has_list_value()) {
        throw InvalidValueException("Expected list value for array conversion.");
    }

    std::vector<T> result;
    result.reserve(static_cast<std::size_t>(value.list_value().values_size()));
    for (const auto& element : value.list_value().values()) {
        result.emplace_back(converter(element));
    }
    return result;
}

template <typename T> const TypedDataPointValue<T>& requireTyped(const DataPointValue& value) {
    const auto* typed = dynamic_cast<const TypedDataPointValue<T>*>(&value);
    if (!typed) {
        throw InvalidTypeException("DataPointValue type mismatch during Duo conversion.");
    }
    return *typed;
}

template <typename T>
std::shared_ptr<DataPointValue> makeTypedValue(const std::string& path, const T& data,
                                               const Timestamp& timestamp) {
    return std::make_shared<TypedDataPointValue<T>>(path, data, timestamp);
}

template <typename T>
std::shared_ptr<DataPointValue> makeFailedValue(const std::string& path, const Timestamp& timestamp,
                                                DataPointValue::Failure failure) {
    return std::make_shared<TypedDataPointValue<T>>(path, failure, timestamp);
}

} // namespace

google::protobuf::Value DuoTypeConverter::toDuoValue(const DataPointValue& dataPointValue) {
    Value value;

    if (!dataPointValue.isValid()) {
        value.set_null_value(google::protobuf::NullValue::NULL_VALUE);
        return value;
    }

    switch (dataPointValue.getType()) {
    case DataPointValue::Type::BOOL:
        value.set_bool_value(requireTyped<bool>(dataPointValue).value());
        break;
    case DataPointValue::Type::BOOL_ARRAY: {
        auto* list = value.mutable_list_value();
        for (bool entry : requireTyped<std::vector<bool>>(dataPointValue).value()) {
            list->add_values()->set_bool_value(entry);
        }
        break;
    }
    case DataPointValue::Type::INT8:
        value.set_number_value(static_cast<double>(requireTyped<int8_t>(dataPointValue).value()));
        break;
    case DataPointValue::Type::INT8_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto entry : requireTyped<std::vector<int8_t>>(dataPointValue).value()) {
            list->add_values()->set_number_value(static_cast<double>(entry));
        }
        break;
    }
    case DataPointValue::Type::INT16:
        value.set_number_value(static_cast<double>(requireTyped<int16_t>(dataPointValue).value()));
        break;
    case DataPointValue::Type::INT16_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto entry : requireTyped<std::vector<int16_t>>(dataPointValue).value()) {
            list->add_values()->set_number_value(static_cast<double>(entry));
        }
        break;
    }
    case DataPointValue::Type::INT32:
        value.set_number_value(static_cast<double>(requireTyped<int32_t>(dataPointValue).value()));
        break;
    case DataPointValue::Type::INT32_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto entry : requireTyped<std::vector<int32_t>>(dataPointValue).value()) {
            list->add_values()->set_number_value(static_cast<double>(entry));
        }
        break;
    }
    case DataPointValue::Type::INT64:
        value.set_number_value(static_cast<double>(requireTyped<int64_t>(dataPointValue).value()));
        break;
    case DataPointValue::Type::INT64_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto entry : requireTyped<std::vector<int64_t>>(dataPointValue).value()) {
            list->add_values()->set_number_value(static_cast<double>(entry));
        }
        break;
    }
    case DataPointValue::Type::UINT8:
        value.set_number_value(static_cast<double>(requireTyped<uint8_t>(dataPointValue).value()));
        break;
    case DataPointValue::Type::UINT8_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto entry : requireTyped<std::vector<uint8_t>>(dataPointValue).value()) {
            list->add_values()->set_number_value(static_cast<double>(entry));
        }
        break;
    }
    case DataPointValue::Type::UINT16:
        value.set_number_value(static_cast<double>(requireTyped<uint16_t>(dataPointValue).value()));
        break;
    case DataPointValue::Type::UINT16_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto entry : requireTyped<std::vector<uint16_t>>(dataPointValue).value()) {
            list->add_values()->set_number_value(static_cast<double>(entry));
        }
        break;
    }
    case DataPointValue::Type::UINT32:
        value.set_number_value(static_cast<double>(requireTyped<uint32_t>(dataPointValue).value()));
        break;
    case DataPointValue::Type::UINT32_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto entry : requireTyped<std::vector<uint32_t>>(dataPointValue).value()) {
            list->add_values()->set_number_value(static_cast<double>(entry));
        }
        break;
    }
    case DataPointValue::Type::UINT64:
        value.set_number_value(static_cast<double>(requireTyped<uint64_t>(dataPointValue).value()));
        break;
    case DataPointValue::Type::UINT64_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto entry : requireTyped<std::vector<uint64_t>>(dataPointValue).value()) {
            list->add_values()->set_number_value(static_cast<double>(entry));
        }
        break;
    }
    case DataPointValue::Type::FLOAT:
        value.set_number_value(static_cast<double>(requireTyped<float>(dataPointValue).value()));
        break;
    case DataPointValue::Type::FLOAT_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto entry : requireTyped<std::vector<float>>(dataPointValue).value()) {
            list->add_values()->set_number_value(static_cast<double>(entry));
        }
        break;
    }
    case DataPointValue::Type::DOUBLE:
        value.set_number_value(requireTyped<double>(dataPointValue).value());
        break;
    case DataPointValue::Type::DOUBLE_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto entry : requireTyped<std::vector<double>>(dataPointValue).value()) {
            list->add_values()->set_number_value(entry);
        }
        break;
    }
    case DataPointValue::Type::STRING:
        value.set_string_value(requireTyped<std::string>(dataPointValue).value());
        break;
    case DataPointValue::Type::STRING_ARRAY: {
        auto* list = value.mutable_list_value();
        for (const auto& entry : requireTyped<std::vector<std::string>>(dataPointValue).value()) {
            list->add_values()->set_string_value(entry);
        }
        break;
    }
    default:
        throw InvalidTypeException("Unsupported datapoint type for Duo conversion.");
    }

    return value;
}

std::shared_ptr<DataPointValue> DuoTypeConverter::fromDuoValue(const std::string&   path,
                                                               DataPointValue::Type expectedType,
                                                               const Value&         value,
                                                               const Timestamp&     timestamp) {
    const auto  segments = splitPath(path);
    const auto* leaf     = locateLeaf(value, segments);
    if (leaf == nullptr) {
        throw InvalidValueException("Requested datapoint path not present in Duo payload: " + path);
    }

    if (leaf->has_null_value()) {
        switch (expectedType) {
        case DataPointValue::Type::BOOL:
            return makeFailedValue<bool>(path, timestamp, DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::BOOL_ARRAY:
            return makeFailedValue<std::vector<bool>>(path, timestamp,
                                                      DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::INT8:
            return makeFailedValue<int8_t>(path, timestamp, DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::INT8_ARRAY:
            return makeFailedValue<std::vector<int8_t>>(path, timestamp,
                                                        DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::INT16:
            return makeFailedValue<int16_t>(path, timestamp,
                                            DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::INT16_ARRAY:
            return makeFailedValue<std::vector<int16_t>>(path, timestamp,
                                                         DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::INT32:
            return makeFailedValue<int32_t>(path, timestamp,
                                            DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::INT32_ARRAY:
            return makeFailedValue<std::vector<int32_t>>(path, timestamp,
                                                         DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::INT64:
            return makeFailedValue<int64_t>(path, timestamp,
                                            DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::INT64_ARRAY:
            return makeFailedValue<std::vector<int64_t>>(path, timestamp,
                                                         DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::UINT8:
            return makeFailedValue<uint8_t>(path, timestamp,
                                            DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::UINT8_ARRAY:
            return makeFailedValue<std::vector<uint8_t>>(path, timestamp,
                                                         DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::UINT16:
            return makeFailedValue<uint16_t>(path, timestamp,
                                             DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::UINT16_ARRAY:
            return makeFailedValue<std::vector<uint16_t>>(path, timestamp,
                                                          DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::UINT32:
            return makeFailedValue<uint32_t>(path, timestamp,
                                             DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::UINT32_ARRAY:
            return makeFailedValue<std::vector<uint32_t>>(path, timestamp,
                                                          DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::UINT64:
            return makeFailedValue<uint64_t>(path, timestamp,
                                             DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::UINT64_ARRAY:
            return makeFailedValue<std::vector<uint64_t>>(path, timestamp,
                                                          DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::FLOAT:
            return makeFailedValue<float>(path, timestamp, DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::FLOAT_ARRAY:
            return makeFailedValue<std::vector<float>>(path, timestamp,
                                                       DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::DOUBLE:
            return makeFailedValue<double>(path, timestamp, DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::DOUBLE_ARRAY:
            return makeFailedValue<std::vector<double>>(path, timestamp,
                                                        DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::STRING:
            return makeFailedValue<std::string>(path, timestamp,
                                                DataPointValue::Failure::NOT_AVAILABLE);
        case DataPointValue::Type::STRING_ARRAY:
            return makeFailedValue<std::vector<std::string>>(
                path, timestamp, DataPointValue::Failure::NOT_AVAILABLE);
        default:
            throw InvalidTypeException("Unsupported datapoint type for Duo conversion.");
        }
    }

    switch (expectedType) {
    case DataPointValue::Type::BOOL:
        return makeTypedValue<bool>(path, convertToBool(*leaf), timestamp);
    case DataPointValue::Type::BOOL_ARRAY:
        return makeTypedValue<std::vector<bool>>(path, convertList<bool>(*leaf, convertToBool),
                                                 timestamp);
    case DataPointValue::Type::INT8:
        return makeTypedValue<int8_t>(path, convertToIntegral<int8_t>(*leaf), timestamp);
    case DataPointValue::Type::INT8_ARRAY:
        return makeTypedValue<std::vector<int8_t>>(
            path, convertList<int8_t>(*leaf, convertToIntegral<int8_t>), timestamp);
    case DataPointValue::Type::INT16:
        return makeTypedValue<int16_t>(path, convertToIntegral<int16_t>(*leaf), timestamp);
    case DataPointValue::Type::INT16_ARRAY:
        return makeTypedValue<std::vector<int16_t>>(
            path, convertList<int16_t>(*leaf, convertToIntegral<int16_t>), timestamp);
    case DataPointValue::Type::INT32:
        return makeTypedValue<int32_t>(path, convertToIntegral<int32_t>(*leaf), timestamp);
    case DataPointValue::Type::INT32_ARRAY:
        return makeTypedValue<std::vector<int32_t>>(
            path, convertList<int32_t>(*leaf, convertToIntegral<int32_t>), timestamp);
    case DataPointValue::Type::INT64:
        return makeTypedValue<int64_t>(path, convertToIntegral<int64_t>(*leaf), timestamp);
    case DataPointValue::Type::INT64_ARRAY:
        return makeTypedValue<std::vector<int64_t>>(
            path, convertList<int64_t>(*leaf, convertToIntegral<int64_t>), timestamp);
    case DataPointValue::Type::UINT8:
        return makeTypedValue<uint8_t>(path, convertToIntegral<uint8_t>(*leaf), timestamp);
    case DataPointValue::Type::UINT8_ARRAY:
        return makeTypedValue<std::vector<uint8_t>>(
            path, convertList<uint8_t>(*leaf, convertToIntegral<uint8_t>), timestamp);
    case DataPointValue::Type::UINT16:
        return makeTypedValue<uint16_t>(path, convertToIntegral<uint16_t>(*leaf), timestamp);
    case DataPointValue::Type::UINT16_ARRAY:
        return makeTypedValue<std::vector<uint16_t>>(
            path, convertList<uint16_t>(*leaf, convertToIntegral<uint16_t>), timestamp);
    case DataPointValue::Type::UINT32:
        return makeTypedValue<uint32_t>(path, convertToIntegral<uint32_t>(*leaf), timestamp);
    case DataPointValue::Type::UINT32_ARRAY:
        return makeTypedValue<std::vector<uint32_t>>(
            path, convertList<uint32_t>(*leaf, convertToIntegral<uint32_t>), timestamp);
    case DataPointValue::Type::UINT64:
        return makeTypedValue<uint64_t>(path, convertToIntegral<uint64_t>(*leaf), timestamp);
    case DataPointValue::Type::UINT64_ARRAY:
        return makeTypedValue<std::vector<uint64_t>>(
            path, convertList<uint64_t>(*leaf, convertToIntegral<uint64_t>), timestamp);
    case DataPointValue::Type::FLOAT:
        return makeTypedValue<float>(path, static_cast<float>(convertToDouble(*leaf)), timestamp);
    case DataPointValue::Type::FLOAT_ARRAY:
        return makeTypedValue<std::vector<float>>(
            path,
            convertList<float>(
                *leaf,
                [](const Value& entry) { return static_cast<float>(convertToDouble(entry)); }),
            timestamp);
    case DataPointValue::Type::DOUBLE:
        return makeTypedValue<double>(path, convertToDouble(*leaf), timestamp);
    case DataPointValue::Type::DOUBLE_ARRAY:
        return makeTypedValue<std::vector<double>>(
            path, convertList<double>(*leaf, convertToDouble), timestamp);
    case DataPointValue::Type::STRING:
        return makeTypedValue<std::string>(path, convertToString(*leaf), timestamp);
    case DataPointValue::Type::STRING_ARRAY:
        return makeTypedValue<std::vector<std::string>>(
            path, convertList<std::string>(*leaf, convertToString), timestamp);
    default:
        throw InvalidTypeException("Unsupported datapoint type for Duo conversion.");
    }
}

std::shared_ptr<DataPointValue> DuoTypeConverter::fromDuoValue(const DataPoint& dataPoint,
                                                               const Value&     value,
                                                               const Timestamp& timestamp) {
    return fromDuoValue(dataPoint.getPath(), dataPoint.getDataType(), value, timestamp);
}

std::string DuoTypeConverter::toDuoPath(const std::string& path) {
    std::string converted = path;
    std::replace(converted.begin(), converted.end(), '.', '/');
    return converted;
}

std::string DuoTypeConverter::toInternalPath(const std::string& path) {
    std::string converted = path;
    std::replace(converted.begin(), converted.end(), '/', '.');
    return converted;
}

} // namespace velocitas::duo
