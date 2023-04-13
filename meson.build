project(
  'vulkan-examples',
  ['c', 'cpp'],
  version : run_command(
    [find_program('python3', 'python'), 'bin/meson_get_version.py'],
    check : true
  ).stdout(),
  license : 'MIT',
  meson_version : '>= 0.53',
  default_options : []
)

glfw_header_dir = join_paths(meson.source_root(),'lib/glfw/include')
glfwLibs=meson.get_compiler('c').find_library('glfw3', 
    dirs : join_paths(meson.source_root(),'lib/glfw'))

vulkan_sdk_header_dir = join_paths(meson.source_root(),'lib/vulkan_sdk/Include')
vulkanLibs=meson.get_compiler('c').find_library('vulkan-1', 
    dirs : join_paths(meson.source_root(),'lib/vulkan_sdk/Lib'))

glm_header_dir = join_paths(meson.source_root(),'lib/glm/include')


spriv_compiler = join_paths(meson.source_root(),'lib/vulkan_sdk/Bin','glslc.exe')

#project list 
executable('demo', ['src/demo.cpp'],
dependencies : [glfwLibs,vulkanLibs], 
include_directories : [glfw_header_dir,vulkan_sdk_header_dir,glm_header_dir],
cpp_args: '/std:c++17')

#first triangle project
projectName = 'firstTriangle'
shaderPath = join_paths(meson.source_root(),'src',projectName,'meta')
vsShader = custom_target('vsShader',
command:[find_program(spriv_compiler),join_paths(shaderPath,'shader.vert') ,'-o',join_paths(shaderPath,'vs.spv')],
capture:true,
output:['vs.spv']
)
psShdaer = custom_target('psShader',
command:[find_program(spriv_compiler),join_paths(shaderPath,'shader.frag') ,'-o',join_paths(shaderPath,'ps.spv')],
capture:true,
output:['ps.spv']
)
executable(projectName, [join_paths('src',projectName,'main.cpp'),vsShader,psShdaer],
dependencies : [glfwLibs,vulkanLibs], 
include_directories : [glfw_header_dir,vulkan_sdk_header_dir,glm_header_dir,join_paths(meson.source_root(),'arch')],
cpp_args: ['/std:c++17','-D_DEBUG_'],
)