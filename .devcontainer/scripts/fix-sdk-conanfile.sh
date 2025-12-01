#!/bin/bash
# Fix SDK conanfile.py after download
# The remote SDK repository has a broken conanfile.py with a syntax error
# This script patches it to use CMake directly instead of build.sh
#
# This file is NOT managed by velocitas - it will persist across rebuilds

set -e

echo "######################################################"
echo "### Fixing SDK conanfile.py for Conan build       ###"
echo "######################################################"

# Install the Conan hook for runtime fixing
echo "Installing Conan pre-build hook..."
mkdir -p ~/.conan2/extensions/hooks
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "$SCRIPT_DIR/conan_hooks/fix_sdk_hook.py" ]; then
    cp "$SCRIPT_DIR/conan_hooks/fix_sdk_hook.py" ~/.conan2/extensions/hooks/
    echo "✓ Conan hook installed to ~/.conan2/extensions/hooks/"
fi

# Function to check if conanfile.py needs fixing
needs_fixing() {
    local file="$1"
    if [ ! -f "$file" ]; then
        return 1
    fi
    
    # Check if already fixed (has CMake-based build)
    if grep -q "cmake.configure(variables=" "$file" 2>/dev/null; then
        return 1
    fi
    
    # Check for broken patterns
    if grep -q "build_script = os.path.abspath\|st = os.stat(build_script)\|subprocess.call.*build\.sh" "$file" 2>/dev/null; then
        return 0
    fi
    
    # Check for garbled Japanese comment
    if grep -q "付丁E\|付与" "$file" 2>/dev/null; then
        return 0
    fi
    
    return 1
}

# Function to write the fixed conanfile.py content
write_fixed_conanfile() {
    local target_file="$1"
    echo "  Writing fixed conanfile.py to: $target_file"
    cat > "$target_file" << 'CONANFILE_CONTENT'
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

import os
import re
import subprocess

from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.files import copy


class VehicleAppCppSdkConan(ConanFile):
    name = "vehicle-app-sdk"
    license = "Apache-2.0"
    url = "https://github.com/DN-SDV-PF/vehicle-app-cpp-sdk"
    description = "The Vehicle App SDK for c++ allows to create Vehicle Apps from the Velocitas development model in the c++ programming language."
    generators = "CMakeDeps", "CMakeToolchain"
    author = "Contributors to the Eclipse Foundation, SDV Working Group"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    exports = "version.txt"

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = ".scripts/common.sh", "build.sh", "install_dependencies.sh", "CMakeLists.txt", "sdk/*", "conanfile.py", ".conan/profiles/*", "version.txt", "LICENSE"

    def __run_cmd(self, args):
        return subprocess.run(args, capture_output=True).stdout.strip().decode("utf-8")

    def set_version(self):
        try:
            tag = self.__run_cmd(["git", "tag", "--points-at", "HEAD"])
            version = ""
            if tag:
                version_tag_pattern = re.compile(r"^v[0-9]+(\.[0-9]+){0,2}")
                if version_tag_pattern.match(tag):
                    tag = tag[1:] # cut off initial v if a semver tag
                version = tag

            # if no tag, use branch name or commit hash
            if not version:
                version = self.__run_cmd(["git", "symbolic-ref", "-q", "--short", "HEAD"])
            if not version:
                version = self.__run_cmd(["git", "rev-parse", "HEAD"])
            
            # / is not allowed in conan version
            self.version = version.replace("/", ".")
            open("./version.txt", mode="w", encoding="utf-8").write(self.version)
        except Exception as exc:
            print(f"Exception catched: {exc}")
            print("--> Maybe not a git repository, reading version from static file...")
            if os.path.isfile("./version.txt"):
                self.version = open("./version.txt", encoding="utf-8").read().strip()
            else:
                raise FileNotFoundError("Missing version.txt!")
        print(f"Determined SDK version: {self.version}")

    def config_options(self):
        if self.settings.get_safe("os") == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def requirements(self):
        self.requires("fmt/11.1.1", transitive_headers=True)
        self.requires("grpc/1.50.1", transitive_headers=True)
        self.requires("nlohmann_json/3.11.3")
        self.requires("paho-mqtt-c/1.3.13")
        self.requires("paho-mqtt-cpp/1.4.0")

    def build_requirements(self):
        # Declare both, grpc and protobuf, here to enable proper x-build (w/o using qemu)
        self.tool_requires("grpc/<host_version>")
        self.tool_requires("protobuf/<host_version>")

    def layout(self):
        os_name = str(self.settings.os).lower()
        arch = str(self.settings.arch).lower()
        if arch == "armv8":
            arch = "aarch64"
        cmake_layout(
            self, 
            src_folder=".",
            build_folder=f"build-{os_name}-{arch}",
        )

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={
            "SDK_BUILD_EXAMPLES": "OFF",
            "SDK_BUILD_TESTS": "OFF"
        })
        cmake.build()

    def package(self):
        copy(
            self,
            "license*",
            src=self.source_folder,
            dst=os.path.join(self.package_folder, "licenses"),
            keep_path=True,
        )
        copy(
            self,
            "*.h",
            src=os.path.join(self.source_folder, "sdk", "include"),
            dst=os.path.join(self.package_folder, "include"),
            keep_path=True,
        )
        copy(
            self,
            "*.a",
            src=self.build_folder,
            dst=os.path.join(self.package_folder, "lib"),
            keep_path=False,
        )

    def package_info(self):
        self.cpp_info.libs = ["vehicle-app-sdk", "vehicle-app-sdk-generated-grpc"]
CONANFILE_CONTENT
}

# Find SDK in velocitas projects
SDK_PATH=""
for project_dir in ~/.velocitas/projects/*/; do
    if [ -d "${project_dir}vehicle-app-sdk-cpp" ]; then
        SDK_PATH="${project_dir}vehicle-app-sdk-cpp"
        break
    fi
done

if [ -z "$SDK_PATH" ]; then
    SDK_PATH=$(find ~/.velocitas/projects -name "vehicle-app-sdk-cpp" -type d 2>/dev/null | head -1)
fi

FIXED_COUNT=0
NEED_REEXPORT=false

# Fix SDK in velocitas projects
if [ -n "$SDK_PATH" ] && [ -f "$SDK_PATH/conanfile.py" ]; then
    echo "SDK_PATH: $SDK_PATH"
    if needs_fixing "$SDK_PATH/conanfile.py"; then
        echo "✗ SDK conanfile.py is broken, fixing..."
        write_fixed_conanfile "$SDK_PATH/conanfile.py"
        FIXED_COUNT=$((FIXED_COUNT + 1))
        NEED_REEXPORT=true
    else
        echo "✓ SDK conanfile.py is OK"
    fi
else
    echo "WARNING: SDK_PATH not found"
fi

# Fix any conanfile.py in Conan cache
echo ""
echo "Checking Conan cache for broken files..."
for conanfile in $(find ~/.conan2/p -name "conanfile.py" 2>/dev/null); do
    # Skip non-SDK files
    if ! grep -q "vehicle-app-sdk\|VehicleAppCppSdkConan" "$conanfile" 2>/dev/null; then
        continue
    fi
    
    if needs_fixing "$conanfile"; then
        echo "✗ Found broken file: $conanfile"
        write_fixed_conanfile "$conanfile"
        FIXED_COUNT=$((FIXED_COUNT + 1))
        NEED_REEXPORT=true
    fi
done

# Always clear and re-export if any fixes were made
if [ "$NEED_REEXPORT" = true ] || [ $FIXED_COUNT -gt 0 ]; then
    echo ""
    echo "Clearing Conan cache and re-exporting SDK..."
    
    # Remove SDK from Conan cache
    conan remove "vehicle-app-sdk*" -c 2>/dev/null || true
    
    # Re-export SDK
    if [ -n "$SDK_PATH" ] && [ -d "$SDK_PATH" ]; then
        echo "Re-exporting SDK from: $SDK_PATH"
        cd "$SDK_PATH"
        conan export . --name vehicle-app-sdk --version 1.0.3
        echo "✓ SDK re-exported to Conan cache"
    fi
    
    # Also re-export vehicle-model if it exists
    VEHICLE_MODEL_PATH=""
    for project_dir in ~/.velocitas/projects/*/; do
        if [ -d "${project_dir}vehicle_model" ]; then
            VEHICLE_MODEL_PATH="${project_dir}vehicle_model"
            break
        fi
    done
    
    if [ -n "$VEHICLE_MODEL_PATH" ] && [ -d "$VEHICLE_MODEL_PATH" ]; then
        echo "Re-exporting vehicle-model from: $VEHICLE_MODEL_PATH"
        conan remove "vehicle-model*" -c 2>/dev/null || true
        cd "$VEHICLE_MODEL_PATH"
        conan export .
        echo "✓ vehicle-model re-exported to Conan cache"
    fi
    
    echo ""
    echo "✓ Fixed $FIXED_COUNT file(s) and re-exported to Conan cache"
else
    echo ""
    echo "✓ All SDK files are OK, no fixes needed"
    
    # Ensure SDK is in cache
    if ! conan list "vehicle-app-sdk*" 2>/dev/null | grep -q "vehicle-app-sdk"; then
        echo "SDK not in Conan cache, exporting..."
        if [ -n "$SDK_PATH" ] && [ -d "$SDK_PATH" ]; then
            cd "$SDK_PATH"
            conan export . --name vehicle-app-sdk --version 1.0.3
            echo "✓ SDK exported to Conan cache"
        fi
    fi
    
    if ! conan list "vehicle-model*" 2>/dev/null | grep -q "vehicle-model"; then
        VEHICLE_MODEL_PATH=""
        for project_dir in ~/.velocitas/projects/*/; do
            if [ -d "${project_dir}vehicle_model" ]; then
                VEHICLE_MODEL_PATH="${project_dir}vehicle_model"
                break
            fi
        done
        
        if [ -n "$VEHICLE_MODEL_PATH" ] && [ -d "$VEHICLE_MODEL_PATH" ]; then
            echo "vehicle-model not in Conan cache, exporting..."
            cd "$VEHICLE_MODEL_PATH"
            conan export .
            echo "✓ vehicle-model exported to Conan cache"
        fi
    fi
fi

echo ""
echo "######################################################"
echo "### SDK fix complete                               ###"
echo "######################################################"
