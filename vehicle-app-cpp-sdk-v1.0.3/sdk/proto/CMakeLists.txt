# Copyright (c) 2022-2025 Contributors to the Eclipse Foundation
#
# This program and the accompanying materials are made available under the
# terms of the Apache License, Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0.
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0

set(TARGET_NAME "vehicle-app-sdk-generated-grpc")

set(GENERATED_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(DUO_GENERATED_DIR ${GENERATED_ROOT}/duo)
set(GOOGLE_GENERATED_DIR ${GENERATED_ROOT}/google/api)
set(KUKSA_GENERATED_DIR ${GENERATED_ROOT}/kuksa/val/v2)
set(SDV_GENERATED_DIR ${GENERATED_ROOT}/sdv/databroker/v1)

set(GENERATED_SOURCES
    ${DUO_GENERATED_DIR}/duo.pb.cc
    ${DUO_GENERATED_DIR}/duo.grpc.pb.cc
    ${DUO_GENERATED_DIR}/types.pb.cc
    ${DUO_GENERATED_DIR}/types.grpc.pb.cc

    ${GOOGLE_GENERATED_DIR}/annotations.pb.cc
    ${GOOGLE_GENERATED_DIR}/annotations.grpc.pb.cc
    ${GOOGLE_GENERATED_DIR}/field_behavior.pb.cc
    ${GOOGLE_GENERATED_DIR}/field_behavior.grpc.pb.cc
    ${GOOGLE_GENERATED_DIR}/http.pb.cc
    ${GOOGLE_GENERATED_DIR}/http.grpc.pb.cc
    ${GOOGLE_GENERATED_DIR}/httpbody.pb.cc
    ${GOOGLE_GENERATED_DIR}/httpbody.grpc.pb.cc

    ${KUKSA_GENERATED_DIR}/types.pb.cc
    ${KUKSA_GENERATED_DIR}/types.grpc.pb.cc
    ${KUKSA_GENERATED_DIR}/val.pb.cc
    ${KUKSA_GENERATED_DIR}/val.grpc.pb.cc

    ${SDV_GENERATED_DIR}/types.pb.cc
    ${SDV_GENERATED_DIR}/types.grpc.pb.cc
    ${SDV_GENERATED_DIR}/collector.pb.cc
    ${SDV_GENERATED_DIR}/collector.grpc.pb.cc
    ${SDV_GENERATED_DIR}/broker.pb.cc
    ${SDV_GENERATED_DIR}/broker.grpc.pb.cc
)

add_library(${TARGET_NAME} STATIC ${GENERATED_SOURCES})

target_include_directories(${TARGET_NAME}
    PUBLIC
    ${absl_INCLUDE_DIRS}
    ${protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
    ${GENERATED_ROOT}
)

target_link_libraries(${TARGET_NAME}
    gRPC::grpc++
)
