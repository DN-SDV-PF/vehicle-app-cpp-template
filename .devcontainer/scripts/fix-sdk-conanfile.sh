#!/bin/bash
# Fix SDK conanfile.py after download
# The remote SDK repository has a broken conanfile.py with a syntax error
# This script patches it to use CMake directly instead of build.sh

echo "######################################################"
echo "### Fixing SDK conanfile.py for Conan build ###"
echo "######################################################"

# Function to write the fixed conanfile.py content
write_fixed_conanfile() {
    local target_file="$1"
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

# Find all locations where SDK conanfile.py might exist

# 1. Velocitas projects directory
SDK_PATH=$(find ~/.velocitas/projects -name "vehicle-app-sdk-cpp" -type d 2>/dev/null | head -1)

# 2. Conan cache - exported recipes
CONAN_EXPORT_PATH=$(find ~/.conan2/p -path "*/vehicle-app-sdk/*" -name "conanfile.py" 2>/dev/null | head -1)
if [ -n "$CONAN_EXPORT_PATH" ]; then
    CONAN_EXPORT_DIR=$(dirname "$CONAN_EXPORT_PATH")
fi

# 3. Conan cache - build directories (where the actual build happens)
CONAN_BUILD_PATHS=$(find ~/.conan2/p/b -name "conanfile.py" 2>/dev/null | xargs grep -l "vehicle-app-sdk" 2>/dev/null)

FIXED_COUNT=0

# Fix SDK in velocitas projects
if [ -n "$SDK_PATH" ] && [ -f "$SDK_PATH/conanfile.py" ]; then
    if grep -q "build_script" "$SDK_PATH/conanfile.py"; then
        echo "Fixing conanfile.py in velocitas projects: $SDK_PATH"
        write_fixed_conanfile "$SDK_PATH/conanfile.py"
        FIXED_COUNT=$((FIXED_COUNT + 1))
    fi
fi

# Fix SDK in Conan export cache
if [ -n "$CONAN_EXPORT_PATH" ] && [ -f "$CONAN_EXPORT_PATH" ]; then
    if grep -q "build_script" "$CONAN_EXPORT_PATH"; then
        echo "Fixing conanfile.py in Conan export cache: $CONAN_EXPORT_PATH"
        write_fixed_conanfile "$CONAN_EXPORT_PATH"
        FIXED_COUNT=$((FIXED_COUNT + 1))
    fi
fi

# Fix SDK in all Conan build directories
for build_conanfile in $CONAN_BUILD_PATHS; do
    if [ -f "$build_conanfile" ] && grep -q "build_script" "$build_conanfile"; then
        echo "Fixing conanfile.py in Conan build cache: $build_conanfile"
        write_fixed_conanfile "$build_conanfile"
        FIXED_COUNT=$((FIXED_COUNT + 1))
    fi
done

# Also search for any conanfile.py with the broken pattern in the entire conan2 directory
echo "Searching for any remaining broken conanfile.py files..."
BROKEN_FILES=$(grep -rl "build_script = os.path.abspath" ~/.conan2 2>/dev/null || true)
for broken_file in $BROKEN_FILES; do
    if [ -f "$broken_file" ] && [[ "$broken_file" == *.py ]]; then
        echo "Fixing additional broken file: $broken_file"
        write_fixed_conanfile "$broken_file"
        FIXED_COUNT=$((FIXED_COUNT + 1))
    fi
done

if [ $FIXED_COUNT -gt 0 ]; then
    echo "✓ Fixed $FIXED_COUNT conanfile.py file(s)"
    
    # Re-export to Conan cache if SDK_PATH exists
    if [ -n "$SDK_PATH" ] && [ -d "$SDK_PATH" ]; then
        echo "Re-exporting SDK to Conan cache..."
        cd "$SDK_PATH" && conan export . -nr
        echo "✓ SDK re-exported to Conan cache"
    fi
    
    # Clear any cached build to force rebuild with fixed conanfile
    echo "Clearing Conan build cache for vehicle-app-sdk..."
    conan cache clean "vehicle-app-sdk*" --build 2>/dev/null || true
    echo "✓ Build cache cleared"
else
    echo "No broken conanfile.py files found or all already fixed"
fi
