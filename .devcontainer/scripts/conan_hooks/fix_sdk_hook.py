# Conan hook to fix vehicle-app-sdk conanfile.py before build
# Place this in ~/.conan2/extensions/hooks/

import os
import re

def _fix_conanfile(conanfile_path, output):
    """Fix the conanfile.py if it contains the broken build_script code"""
    if not os.path.exists(conanfile_path):
        return False
    
    with open(conanfile_path, 'r') as f:
        content = f.read()
    
    # Check if file needs fixing - look for the broken pattern
    if 'build_script = os.path.abspath' not in content and 'build_script' not in content:
        return False
    
    # Also check if already fixed
    if 'cmake.configure(variables={' in content and 'SDK_BUILD_EXAMPLES' in content:
        return False
    
    output.info("Fixing conanfile.py build() method...")
    
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
    
    output.info("conanfile.py fixed successfully")
    return True

def pre_export(conanfile):
    """Fix vehicle-app-sdk conanfile.py before export"""
    if conanfile.name != "vehicle-app-sdk":
        return
    
    conanfile_path = os.path.join(conanfile.recipe_folder, "conanfile.py")
    _fix_conanfile(conanfile_path, conanfile.output)

def post_source(conanfile):
    """Fix vehicle-app-sdk conanfile.py after source download"""
    if conanfile.name != "vehicle-app-sdk":
        return
    
    conanfile_path = os.path.join(conanfile.source_folder, "conanfile.py")
    _fix_conanfile(conanfile_path, conanfile.output)

def pre_build(conanfile):
    """Fix vehicle-app-sdk conanfile.py before build"""
    if conanfile.name != "vehicle-app-sdk":
        return
    
    conanfile_path = os.path.join(conanfile.source_folder, "conanfile.py")
    _fix_conanfile(conanfile_path, conanfile.output)
