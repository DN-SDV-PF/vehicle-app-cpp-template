# Conan hook to fix vehicle-app-sdk conanfile.py before build
# Place this in ~/.conan2/extensions/hooks/

import os
import re

FIXED_BUILD_METHOD = '''    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={
            "SDK_BUILD_EXAMPLES": "OFF",
            "SDK_BUILD_TESTS": "OFF"
        })
        cmake.build()'''

def _fix_conanfile(conanfile_path, output):
    """Fix the conanfile.py if it contains the broken build_script code"""
    if not os.path.exists(conanfile_path):
        return False
    
    with open(conanfile_path, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    
    # Check if already fixed with CMake approach
    if 'cmake.configure(variables={' in content and 'SDK_BUILD_EXAMPLES' in content:
        return False
    
    # Check for broken patterns:
    # 1. build_script without proper definition (the bug)
    # 2. os.stat(build_script) which is the error line
    # 3. Garbled Japanese comment that breaks the line
    needs_fix = False
    if 'st = os.stat(build_script)' in content:
        needs_fix = True
    if 'build_script = os.path.abspath' in content:
        needs_fix = True
    # Check for the garbled comment pattern
    if '付丁E' in content or '付与' in content:
        needs_fix = True
    # Check if build() method uses subprocess and build.sh
    if 'subprocess.call' in content and 'build.sh' in content:
        needs_fix = True
    
    if not needs_fix:
        return False
    
    output.info("Fixing conanfile.py build() method...")
    
    # Replace the entire build() method with CMake-based build
    pattern = r'    def build\(self\):.*?(?=\n    def |\nclass |\Z)'
    
    new_content = re.sub(pattern, FIXED_BUILD_METHOD + '\n\n', content, flags=re.DOTALL)
    
    with open(conanfile_path, 'w', encoding='utf-8') as f:
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
