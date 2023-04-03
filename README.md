Vulkan Examples 
=========================

This is a Vulkan's examples repository , can be compiled and run at any os.

This project is structed by meson tools.[Meson](http://mesonbuild.com),

This porject is rely on [vulkan_sdk]([Meson](http://mesonbuild.com)) , an you should download vulkan sdk
and modify lib/winprepare.py sdk_inc_path variable

then run

python lib/winprepare.py

to make effect.

Build And Run
=========================
meson setup build
cd build
ninja

or

meson setup build_vs ---backend=vs
cd build
use visual studio to open project & run

