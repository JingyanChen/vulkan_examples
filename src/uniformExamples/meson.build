projectName = meson.current_source_dir().split('/')[1]
shaderPath = join_paths(meson.current_source_dir(),'meta')

vsShader = custom_target(projectName + '_vsShader',
depend_files:[join_paths(shaderPath,'shader.vert')],
command:[find_program(spriv_compiler),join_paths(shaderPath,'shader.vert') ,'-o',join_paths(shaderPath,'vs.spv')],
capture:true,
output:[projectName +'_vs.spv']
)

psShader = custom_target(projectName +'_psShader',
depend_files:[join_paths(shaderPath,'shader.frag')],
command:[find_program(spriv_compiler),join_paths(shaderPath,'shader.frag') ,'-o',join_paths(shaderPath,'ps.spv')],
capture:true,
output:[projectName +'_ps.spv']
)

commonLibSource = [
    join_paths(meson.source_root(),'lib/vulkanHelper/src','buffer.cpp'),
]
commonLibInc = join_paths(meson.source_root(),'lib/vulkanHelper/inc')

libCommon = static_library(
  'libCommon',commonLibSource,
  dependencies : [glfwLibs,vulkanLibs], 
  include_directories : [commonLibInc,glfw_header_dir,vulkan_sdk_header_dir,join_paths(meson.source_root(),'arch')],
  cpp_args: ['/std:c++17'],
  )


executable(projectName, ['main.cpp',vsShader,psShader],
install : true,
install_dir : meson.build_root(),
dependencies : [glfwLibs,vulkanLibs], 
include_directories : [glfw_header_dir,vulkan_sdk_header_dir,glm_header_dir,commonLibInc,join_paths(meson.source_root(),'arch')],
cpp_args: ['/std:c++17','-D_DEBUG_'],
link_with: libCommon
)