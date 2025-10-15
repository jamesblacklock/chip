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

def arg(arg: str):
  return arg in sys.argv[1:]

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

if clean:
  shutil.rmtree("./deps", ignore_errors=True)
  shutil.rmtree("./build", ignore_errors=True)

if build:
  Path("./deps/macos/include").mkdir(parents=True, exist_ok=True)
  Path("./deps/tmp").mkdir(parents=True, exist_ok=True)
  Path("./build/macos").mkdir(parents=True, exist_ok=True)

  if not Path("./deps/macos/include/cglm/cglm.h").is_file():
    if not Path("./deps/macos/cglm").is_dir():
      exec("git", "clone", "--no-checkout", "--depth=1", "--filter=tree:0", "https://github.com/recp/cglm.git", cwd="./deps/macos")
      exec("git", "sparse-checkout", "set", "--no-cone", "/include/cglm", cwd="./deps/macos/cglm")
      exec("git", "checkout", cwd="./deps/macos/cglm")
    copy_dir_contents("./deps/macos/cglm/include", "./deps/macos/include")

  if not Path("./deps/macos/vulkansdk").is_dir():
    if not Path("./deps/tmp/vulkansdk-macOS-1.4.328.1.app").is_dir():
      if not Path("./deps/tmp/vulkansdk.zip").is_file():
        urllib.request.urlretrieve("https://sdk.lunarg.com/sdk/download/1.4.328.1/mac/vulkansdk-macos-1.4.328.1.zip", "./deps/tmp/vulkansdk.zip")
      with zipfile.ZipFile("./deps/tmp/vulkansdk.zip", "r") as archive:
        archive.extractall("./deps/tmp")
        archive.close()
    os.chmod('./deps/tmp/vulkansdk-macOS-1.4.328.1.app/Contents/MacOS/vulkansdk-macOS-1.4.328.1', exe_mod)
    exec(
      './deps/tmp/vulkansdk-macOS-1.4.328.1.app/Contents/MacOS/vulkansdk-macOS-1.4.328.1',
      '--root', f'{Path.cwd() / "deps/macos/vulkansdk"}',
      '--accept-licenses',
      '--default-answer',
      '--confirm-command',
      'install',
    )
  if not Path("./deps/macos/include/vulkan").is_dir():
    copy_dir_contents("./deps/macos/vulkansdk/macOS/include", "./deps/macos/include")

  exec("clang", "-g", "-O0", "-c", "-Ideps/macos/include", "src/main.c", "-o", "./build/macos/main.o")
  exec(
    "swiftc",
    "-import-objc-header", "src/arch/macos/swift_bridge.h",
    "-I", "deps/macos/include",
    "src/arch/macos/main.swift",
    "./build/macos/main.o",
    "./deps/macos/vulkansdk/macOS/lib/libMoltenVK.dylib",
    "-o", "./build/macos/game",
    "-framework", "AppKit",
    "-framework", "Metal",
    "-framework", "QuartzCore",
    "-Xlinker", "-rpath",
    "-Xlinker", "@executable_path"
  )
  exec("./deps/macos/vulkansdk/macOS/bin/glslc", "./src/shaders/shader.vert", "-o", "./build/macos/vert.spv")
  exec("./deps/macos/vulkansdk/macOS/bin/glslc", "./src/shaders/shader.frag", "-o", "./build/macos/frag.spv")

  Path("./build/macos/Game.app/Contents/MacOS").mkdir(parents=True, exist_ok=True)

  shutil.copyfile("./build/macos/game", "./build/macos/Game.app/Contents/MacOS/game")
  shutil.copyfile("./build/macos/frag.spv", "./build/macos/Game.app/Contents/MacOS/frag.spv")
  shutil.copyfile("./build/macos/vert.spv", "./build/macos/Game.app/Contents/MacOS/vert.spv")
  shutil.copyfile("./deps/macos/vulkansdk/macOS/lib/libMoltenVK.dylib", "./build/macos/Game.app/Contents/MacOS/libMoltenVK.dylib")

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
</dict>
</plist>
"""

  with open("./build/macos/Game.app/Contents/Info.plist", "w") as plist_fd:
    plist_fd.write(plist)
    plist_fd.close()

if run:
  try:
    exec("./build/macos/Game.app/Contents/MacOS/game")
  except KeyboardInterrupt:
    pass
