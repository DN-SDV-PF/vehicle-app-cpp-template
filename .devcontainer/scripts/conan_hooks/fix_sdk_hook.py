# Conan hook to fix vehicle-app-sdk conanfile.py before build
# Place this in ~/.conan2/extensions/hooks/

import os
import re

def pre_build(conanfile):
    """Fix vehicle-app-sdk conanfile.py before build"""
    if conanfile.name != "vehicle-app-sdk":
        return
    
    conanfile_path = os.path.join(conanfile.source_folder, "conanfile.py")
    if not os.path.exists(conanfile_path):
        return
    
    with open(conanfile_path, 'r') as f:
        content = f.read()
    
    # Check if file needs fixing
    if 'build_script = os.path.abspath' not in content:
        return
    
    conanfile.output.info("Fixing conanfile.py build() method...")
    
    # Replace the broken build() method with CMake-based build
    fixed_build_method = '''    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={
            "SDK_BUILD_EXAMPLES": "OFF",
            "SDK_BUILD_TESTS": "OFF"
        })
        cmake.build()'''
    
    # Pattern to match the entire build method
    pattern = r'    def build\(self\):.*?(?=\n    def |\nclass |\Z)'
    
    new_content = re.sub(pattern, fixed_build_method + '\n\n', content, flags=re.DOTALL)
    
    with open(conanfile_path, 'w') as f:
        f.write(new_content)
    
    conanfile.output.info("conanfile.py fixed successfully")
