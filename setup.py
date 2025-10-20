#!/usr/bin/env python3

from typing import List
import urllib.request
import subprocess
import zipfile
from pathlib import Path
import shutil
import sys
import os
import stat

exe_mod = stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IROTH

def cc(*cmd: List[str], src: str, build: str):
  Path(build).mkdir(parents=True, exist_ok=True)
  c_file_paths = [x for x in Path(src).rglob(f"*.c")]
  cpp_file_paths = [x for x in Path(src).rglob(f"*.cpp")]

  for f in c_file_paths:
    c_file = str(f)
    o_file = Path(build) / f.with_suffix(".o").name
    exec(*cmd, "-c", c_file, "-o", o_file)
  for f in cpp_file_paths:
    c_file = str(f)
    o_file = Path(build) / f.with_suffix(".o").name
    exec(*cmd, "-std=c++11", "-c", c_file, "-o", o_file)

def build_repo(repo: str, include="include"):
  name = Path(repo).stem
  if not (Path(f"./deps/macos/include/{name}").is_dir() and Path(f"./deps/macos/lib/lib{name}.a").is_file()):
    if not Path(f"./deps/macos/src/{name}").is_dir():
      exec("git", "clone", "--no-checkout", "--depth=1", "--filter=tree:0", repo, cwd="./deps/macos/src")
      exec("git", "sparse-checkout", "set", "--no-cone", f"/{include}", "/src", cwd=f"./deps/macos/src/{name}")
      exec("git", "checkout", cwd=f"./deps/macos/src/{name}")
      cc("clang", "-fPIC", "-ffast-math", "-O2", f"-I./deps/macos/src/{name}/{include}", src=f"./deps/macos/src/{name}/src", build=f"./deps/macos/src/{name}/build")
      o_files = [str(x.relative_to(f"./deps/macos/src/{name}/build")) for x in Path(f"./deps/macos/src/{name}/build").rglob("*.o")]
      exec("ar", "rcs", f"lib{name}.a", *o_files, cwd=f"./deps/macos/src/{name}/build")
    if not Path(f"./deps/macos/include/{name}").is_dir():
      copy_dir_contents(f"./deps/macos/src/{name}/{include}", "./deps/macos/include")
    if not Path(f"./deps/macos/lib/lib{name}.a").is_file():
      shutil.copyfile(f"./deps/macos/src/{name}/build/lib{name}.a", f"./deps/macos/lib/lib{name}.a")

try:
  i = sys.argv.index("--")
  setup_args = sys.argv[1:i]
  exe_args = sys.argv[i+1:]
except:
  setup_args = sys.argv[1:]
  exe_args = []

def arg(arg: str):
  return arg in setup_args

def exec(*cmd: List[str], cwd: str | None = None):
  res = subprocess.run(cmd, stderr=subprocess.STDOUT, cwd=cwd)
  if res.returncode != 0:
    exit(1)

def copy_dir_contents(src, dest):
  for file in os.listdir(src):
    src_path = str(Path(src) / file)
    dest_path = str(Path(dest) / file)
    if Path(src_path).is_dir():
      shutil.copytree(src_path, dest_path, dirs_exist_ok=True)
    else:
      shutil.copyfile(src_path, dest_path)

clean = arg("clean")
run = arg("run")
build = arg("build")
pull = arg("pull")

if pull:
  exec("git", "pull")

if clean:
  shutil.rmtree("./deps", ignore_errors=True)
  shutil.rmtree("./build", ignore_errors=True)

if build:
  Path("./deps/macos/include").mkdir(parents=True, exist_ok=True)
  Path("./deps/macos/lib").mkdir(parents=True, exist_ok=True)
  Path("./deps/macos/src").mkdir(parents=True, exist_ok=True)
  Path("./deps/macos/bin").mkdir(parents=True, exist_ok=True)
  Path("./deps/tmp").mkdir(parents=True, exist_ok=True)

  # cglm
  if not Path("./deps/macos/include/cglm").is_dir():
    if not Path("./deps/macos/src/cglm").is_dir():
      exec("git", "clone", "--no-checkout", "--depth=1", "--filter=tree:0", "https://github.com/recp/cglm.git", cwd="./deps/macos/src")
      exec("git", "sparse-checkout", "set", "--no-cone", "/include/cglm", cwd="./deps/macos/src/cglm")
      exec("git", "checkout", cwd="./deps/macos/src/cglm")
    copy_dir_contents("./deps/macos/src/cglm/include", "./deps/macos/include")

  # MoltenVK
  if not (Path("./deps/macos/include/vulkan").is_dir() and Path("./deps/macos/lib/libMoltenVK.dylib").is_file() and Path("./deps/macos/bin/glslc").is_file()):
    if not Path("./deps/macos/src/vulkansdk").is_dir():
      if not Path("./deps/tmp/vulkansdk-macOS-1.4.328.1.app").is_dir():
        if not Path("./deps/tmp/vulkansdk.zip").is_file():
          urllib.request.urlretrieve("https://sdk.lunarg.com/sdk/download/1.4.328.1/mac/vulkansdk-macos-1.4.328.1.zip", "./deps/tmp/vulkansdk.zip")
        with zipfile.ZipFile("./deps/tmp/vulkansdk.zip", "r") as archive:
          archive.extractall("./deps/tmp")
          archive.close()
      os.chmod('./deps/tmp/vulkansdk-macOS-1.4.328.1.app/Contents/MacOS/vulkansdk-macOS-1.4.328.1', exe_mod)
      exec(
        './deps/tmp/vulkansdk-macOS-1.4.328.1.app/Contents/MacOS/vulkansdk-macOS-1.4.328.1',
        '--root', f'{Path.cwd() / "deps/macos/src/vulkansdk"}',
        '--accept-licenses',
        '--default-answer',
        '--confirm-command',
        'install',
      )
    if not Path("./deps/macos/include/vulkan").is_dir():
      copy_dir_contents("./deps/macos/src/vulkansdk/macOS/include", "./deps/macos/include")
    if not Path("./deps/macos/lib/libMoltenVK.dylib").is_file():
      shutil.copyfile("./deps/macos/src/vulkansdk/macOS/lib/libMoltenVK.dylib", "./deps/macos/lib/libMoltenVK.dylib")
    if not Path("./deps/macos/bin/glslc").is_file():
      shutil.copyfile("./deps/macos/src/vulkansdk/macOS/bin/glslc", "./deps/macos/bin/glslc")
      os.chmod("./deps/macos/bin/glslc", exe_mod)

  # Box2D
  build_repo("https://github.com/erincatto/box2d.git")

  # polypartition
  build_repo("https://github.com/ivanfratric/polypartition.git", include="src")

  cc("clang", "-g", "-O0", "-c", "-I./deps/macos/include", src="./src", build="./build/macos")
  o_files = [str(x) for x in Path("./build/macos").rglob("*.o")]
  exec(
    "swiftc",
    "-import-objc-header", "src/arch/macos/swift_bridge.h",
    "-I", "deps/macos/include",
    "-L./deps/macos/lib",
    "src/arch/macos/main.swift",
    *o_files,
    "./deps/macos/lib/libMoltenVK.dylib",
    "-lbox2d",
    "-lpolypartition",
    "-lc++",
    "-o", "./build/macos/game",
    "-framework", "AppKit",
    "-framework", "Metal",
    "-framework", "QuartzCore",
    "-Xlinker", "-rpath",
    "-Xlinker", "@executable_path",
  )
  exec("./deps/macos/bin/glslc", "./src/shaders/shader.vert", "-o", "./build/macos/vert.spv")
  exec("./deps/macos/bin/glslc", "./src/shaders/shader.frag", "-o", "./build/macos/frag.spv")

  Path("./build/macos/Game.app/Contents/MacOS").mkdir(parents=True, exist_ok=True)
  Path("./build/macos/Game.app/Contents/Resources").mkdir(parents=True, exist_ok=True)

  shutil.copyfile("./build/macos/game", "./build/macos/Game.app/Contents/MacOS/game")
  shutil.copyfile("./build/macos/frag.spv", "./build/macos/Game.app/Contents/MacOS/frag.spv")
  shutil.copyfile("./build/macos/vert.spv", "./build/macos/Game.app/Contents/MacOS/vert.spv")
  shutil.copyfile("./deps/macos/lib/libMoltenVK.dylib", "./build/macos/Game.app/Contents/MacOS/libMoltenVK.dylib")
  shutil.copyfile("./app_icon.icns", "./build/macos/Game.app/Contents/Resources/app_icon.icns")

  os.chmod("./build/macos/game", exe_mod)
  os.chmod("./build/macos/Game.app/Contents/MacOS/game", exe_mod)

  plist = f"""
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>Game</string>
    <key>CFBundleIdentifier</key>
    <string>com.example.Game</string>
    <key>CFBundleName</key>
    <string>Game</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleIconFile</key>
    <string>app_icon.icns</string>
</dict>
</plist>
"""

  with open("./build/macos/Game.app/Contents/Info.plist", "w") as plist_fd:
    plist_fd.write(plist)
    plist_fd.close()

if run:
  try:
    exec("./build/macos/Game.app/Contents/MacOS/game", *exe_args)
  except KeyboardInterrupt:
    pass
