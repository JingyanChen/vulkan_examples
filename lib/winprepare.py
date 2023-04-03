import os
import sys
import argparse
import shutil
import io

root_dir = os.getcwd()
sdk_inc_path = '"C:\\VulkanSDK\\1.3.231.1"'
sdk_inc_link = os.path.join(root_dir, 'vulkan_sdk')
sdk_inc_linke_cmd = 'mklink /j %s %s' % (sdk_inc_link, sdk_inc_path) 


if not os.path.exists(sdk_inc_link):
    print(sdk_inc_linke_cmd)
    os.system(sdk_inc_linke_cmd)

print ('[meson build]: link ready!')
