import os
import sys
import urllib.request
import zipfile
import subprocess
import shutil

# Configure headers to prevent HTTP 403 blocks from GitHub/Khronos
HTTP_HEADERS = {
    'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3'
}

def download_file(url, filepath):
    print(f"Downloading {url} to {filepath}...")
    req = urllib.request.Request(url, headers=HTTP_HEADERS)
    # Ensure parent directory exists
    os.makedirs(os.path.dirname(os.path.abspath(filepath)), exist_ok=True)
    with urllib.request.urlopen(req) as response, open(filepath, 'wb') as out_file:
        shutil.copyfileobj(response, out_file)
    print(f"Finished downloading {filepath}.")

def extract_zip_member(zip_ref, member_path, target_path):
    # Ensure target directory exists
    os.makedirs(os.path.dirname(target_path), exist_ok=True)
    with zip_ref.open(member_path) as source, open(target_path, "wb") as target:
        shutil.copyfileobj(source, target)
    print(f"Extracted: {target_path}")

def setup_imgui():
    imgui_zip = "imgui_v1.92.8.zip"
    imgui_url = "https://github.com/ocornut/imgui/archive/refs/tags/v1.92.8.zip"
    
    if not os.path.exists(imgui_zip):
        download_file(imgui_url, imgui_zip)
        
    print("Extracting Dear ImGui...")
    os.makedirs("libs/ImGui/backends", exist_ok=True)
    with zipfile.ZipFile(imgui_zip, 'r') as zip_ref:
        # Maps internal zip path -> local destination path
        files_to_extract = {
            "imgui-1.92.8/imgui.h": "libs/ImGui/imgui.h",
            "imgui-1.92.8/imgui.cpp": "libs/ImGui/imgui.cpp",
            "imgui-1.92.8/imgui_draw.cpp": "libs/ImGui/imgui_draw.cpp",
            "imgui-1.92.8/imgui_widgets.cpp": "libs/ImGui/imgui_widgets.cpp",
            "imgui-1.92.8/imgui_tables.cpp": "libs/ImGui/imgui_tables.cpp",
            "imgui-1.92.8/imgui_demo.cpp": "libs/ImGui/imgui_demo.cpp",
            "imgui-1.92.8/imconfig.h": "libs/ImGui/imconfig.h",
            "imgui-1.92.8/imgui_internal.h": "libs/ImGui/imgui_internal.h",
            "imgui-1.92.8/imstb_rectpack.h": "libs/ImGui/imstb_rectpack.h",
            "imgui-1.92.8/imstb_textedit.h": "libs/ImGui/imstb_textedit.h",
            "imgui-1.92.8/imstb_truetype.h": "libs/ImGui/imstb_truetype.h",
            "imgui-1.92.8/backends/imgui_impl_glfw.h": "libs/ImGui/backends/imgui_impl_glfw.h",
            "imgui-1.92.8/backends/imgui_impl_glfw.cpp": "libs/ImGui/backends/imgui_impl_glfw.cpp",
            "imgui-1.92.8/backends/imgui_impl_opengl3.h": "libs/ImGui/backends/imgui_impl_opengl3.h",
            "imgui-1.92.8/backends/imgui_impl_opengl3.cpp": "libs/ImGui/backends/imgui_impl_opengl3.cpp",
            "imgui-1.92.8/backends/imgui_impl_opengl3_loader.h": "libs/ImGui/backends/imgui_impl_opengl3_loader.h",
        }
        for src, dest in files_to_extract.items():
            extract_zip_member(zip_ref, src, dest)

def setup_nfd():
    nfd_zip = "nfd_v1.2.1.zip"
    nfd_url = "https://github.com/btzy/nativefiledialog-extended/archive/refs/tags/v1.2.1.zip"
    
    if not os.path.exists(nfd_zip):
        download_file(nfd_url, nfd_zip)
        
    print("Extracting Native File Dialog Extended (NFDe)...")
    os.makedirs("libs/include", exist_ok=True)
    with zipfile.ZipFile(nfd_zip, 'r') as zip_ref:
        files_to_extract = {
            "nativefiledialog-extended-1.2.1/src/include/nfd.h": "libs/include/nfd.h",
            "nativefiledialog-extended-1.2.1/src/include/nfd.hpp": "libs/include/nfd.hpp",
            "nativefiledialog-extended-1.2.1/src/nfd_win.cpp": "libs/ImGui/nfd_win.cpp",
            "nativefiledialog-extended-1.2.1/src/nfd_cocoa.m": "libs/ImGui/nfd_cocoa.m",
            "nativefiledialog-extended-1.2.1/src/nfd_gtk.cpp": "libs/ImGui/nfd_gtk.cpp",
        }
        for src, dest in files_to_extract.items():
            extract_zip_member(zip_ref, src, dest)

def setup_headers():
    print("Downloading required OpenGL and JSON headers...")
    # 1. Download glcorearb.h
    glcorearb_url = "https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h"
    download_file(glcorearb_url, "libs/include/GL/glcorearb.h")
    
    # 2. Download khrplatform.h
    khrplatform_url = "https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h"
    download_file(khrplatform_url, "libs/include/KHR/khrplatform.h")
    
    # 3. Download picojson.h
    picojson_url = "https://raw.githubusercontent.com/kazuho/picojson/master/picojson.h"
    download_file(picojson_url, "libs/include/picojson.h")

def setup_windows():
    print("Setting up Windows-specific dependencies...")
    os.makedirs("libs", exist_ok=True)
    
    # 1. GLFW 3.4 WIN64 Binaries
    glfw_zip = "glfw-3.4.bin.WIN64.zip"
    glfw_url = "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip"
    if not os.path.exists(glfw_zip):
        download_file(glfw_url, glfw_zip)
        
    print("Extracting GLFW binaries...")
    with zipfile.ZipFile(glfw_zip, 'r') as zip_ref:
        for member in zip_ref.namelist():
            if member.startswith("glfw-3.4.bin.WIN64/"):
                # Construct output path
                rel_path = member[len("glfw-3.4.bin.WIN64/"):]
                if not rel_path:
                    continue
                dest_path = os.path.join("libs/glfw-3.4.bin.WIN64", rel_path)
                if member.endswith('/'):
                    os.makedirs(dest_path, exist_ok=True)
                else:
                    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
                    with zip_ref.open(member) as source, open(dest_path, "wb") as target:
                        shutil.copyfileobj(source, target)
                        
    # 2. OpenCV 4.13.0 Windows EXE
    opencv_exe = "opencv-4.13.0-windows.exe"
    opencv_url = "https://github.com/opencv/opencv/releases/download/4.13.0/opencv-4.13.0-windows.exe"
    if not os.path.exists(opencv_exe):
        download_file(opencv_url, opencv_exe)
        
    print("Extracting OpenCV self-extracting archive (this might take a few moments)...")
    try:
        abs_lib_dir = os.path.abspath("libs")
        cmd = [os.path.abspath(opencv_exe), "-y", f"-o{abs_lib_dir}"]
        print(f"Executing: {' '.join(cmd)}")
        result = subprocess.run(cmd, check=True)
        print("OpenCV extraction complete.")
    except Exception as e:
        print(f"Error extracting OpenCV: {e}")
        sys.exit(1)

def setup_macos():
    print("Setting up macOS-specific dependencies...")
    os.makedirs("libs", exist_ok=True)
    
    # 1. GLFW 3.4 Source
    glfw_zip = "glfw-3.4.zip"
    glfw_url = "https://github.com/glfw/glfw/archive/refs/tags/3.4.zip"
    if not os.path.exists(glfw_zip):
        download_file(glfw_url, glfw_zip)
        
    print("Extracting GLFW sources...")
    with zipfile.ZipFile(glfw_zip, 'r') as zip_ref:
        zip_ref.extractall("libs")
        src_dir = "libs/glfw-3.4"
        if not os.path.exists(src_dir):
            extracted_folder = [f for f in os.listdir("libs") if f.startswith("glfw-")][0]
            os.rename(os.path.join("libs", extracted_folder), src_dir)

    # 2. OpenCV 4.13.0 Source
    opencv_zip = "opencv-4.13.0.zip"
    opencv_url = "https://github.com/opencv/opencv/archive/refs/tags/4.13.0.zip"
    if not os.path.exists(opencv_zip):
        download_file(opencv_url, opencv_zip)
        
    print("Extracting OpenCV sources...")
    with zipfile.ZipFile(opencv_zip, 'r') as zip_ref:
        zip_ref.extractall("libs")
        src_dir = "libs/opencv-4.13.0"
        if not os.path.exists(src_dir):
            extracted_folder = [f for f in os.listdir("libs") if f.startswith("opencv-")][0]
            os.rename(os.path.join("libs", extracted_folder), src_dir)

def main():
    setup_imgui()
    setup_nfd()
    setup_headers()
    
    # Platform-specific library setups
    if sys.platform.startswith('win'):
        setup_windows()
    elif sys.platform == 'darwin':
        setup_macos()
    else:
        print("Running on Linux. OpenCV and GLFW will be linked from the system packages.")

    print("\nDependency setup finished successfully!")

if __name__ == "__main__":
    main()
