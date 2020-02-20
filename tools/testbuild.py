#!/usr/bin/env python3

from buildMicroblaze import build_microblaze
proj_name = "quickbuild"

dev_path = '../'

xsct = "/opt/Xilinx/SDK/2017.4/bin/xsct"
dev_path_tools = dev_path + "/tools"

device_dir = "/device1"
build_microblaze(xsct, proj_name, dev_path_tools, dev_path, device_dir)
