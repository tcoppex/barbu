# -----------------------------------------------------------------------------
# Sources.
# -----------------------------------------------------------------------------

set(TARGET_NAME "${CMAKE_PROJECT_NAME}")

list(APPEND Sources
  main.cc
  Application.cc

  # animation/blend_tree.cc
  animation/skeleton.cc
  animation/skeleton_controller.cc

  core/app.cc
  core/events.cc
  core/global_clock.cc
  core/graphics.cc
  core/renderer.cc

  ecs/scene_hierarchy.cc
  ecs/components/skin.cc

  fx/gpu_particle.cc
  fx/grid.cc
  fx/hair.cc
  fx/marschner.cc
  fx/probe.cc
  fx/skybox.cc
  fx/postprocess/hbao.cc
  fx/postprocess/postprocess.cc

  memory/assets/assets.cc
  memory/assets/material_asset.cc
  memory/assets/mesh.cc
  memory/assets/program.cc
  memory/assets/texture.cc
  memory/resources/image.cc
  memory/resources/mesh_data.cc
  memory/resources/mesh_data_manager.cc
  memory/resources/resources.cc
  memory/resources/shader.cc
  memory/pingpong_buffer.cc
  memory/random_buffer.cc

  utils/gizmo.cc
  utils/raw_mesh_file.cc

  ui/ui_controller.cc

  ui/views/Main.cc
  ui/views/RendererView.cc
  ui/views/fx/SparkleView.cc
)

list(APPEND Headers
  Application.h

  # animation/blend_tree.h
  # animation/blend_node.h
  animation/skeleton.h
  animation/skeleton_controller.h
  animation/common.h

  core/app.h
  core/events.h
  core/global_clock.h
  core/graphics.h
  core/renderer.h

  ecs/ecs.h
  ecs/entity.h
  ecs/component.h
  ecs/scene_hierarchy.h
  ecs/components/skin.h
  ecs/components/transform.h
  ecs/components/visual.h
  
  fx/gpu_particle.h
  fx/grid.h
  fx/hair.h
  fx/irradiance.h
  fx/marschner.h
  fx/skybox.h
  fx/postprocess/postprocess.h
  fx/postprocess/hbao.h

  memory/asset_factory.h
  memory/assets/assets.h
  memory/assets/mesh.h
  memory/assets/texture.h
  memory/assets/program.h
  memory/assets/material_asset.h
  memory/resources/image.h
  memory/resources/mesh_data.h
  memory/resource_info_list.h
  memory/resource_manager.h
  memory/resources/resources.h
  memory/resources/shader.cc
  memory/hash_id.h
  memory/null_vector.h
  memory/pingpong_buffer.h
  memory/random_buffer.h
  memory/enum_array.h

  utils/arcball_controller.h
  utils/cpu_particle.h
  utils/gizmo.h
  utils/mathutils.h
  utils/raw_mesh_file.h
  utils/singleton.h

  ui/imgui_wrapper.h
  ui/ui_controller.h
  ui/ui_view.h

  ui/views/Main.h
  ui/views/RendererView.cc
  ui/views/views.h
  ui/views/fx/HairView.h
  ui/views/fx/MarschnerView.h
  ui/views/fx/SparkleView.h
)

file(GLOB_RECURSE Miscs 
  ${BARBU_SHADERS_DIR}/*.glsl 
  ${BARBU_SHADERS_DIR}/hair/interop.h
)

# -----------------------------------------------------------------------------
# Target properties and build parameters.
# -----------------------------------------------------------------------------

# All dependencies files.
set(Dependencies 
  ${Sources}
  ${Headers}
  ${Miscs}
  ${Thirdparties}
)

add_executable(${TARGET_NAME} ${Dependencies})

# Set targets properties.
list(APPEND Targets ${TARGET_NAME})

foreach(target ${Targets})
  target_compile_options(
    ${target}
    PRIVATE
      "${CXX_FLAGS}"
      "$<$<CONFIG:Debug>:${CXX_FLAGS_DEBUG}>"
      "$<$<CONFIG:Release>:${CXX_FLAGS_RELEASE}>"
      "$<$<CONFIG:DebugWithRelInfo>:${CXX_FLAGS_RELWITHDEBINFO}>"
  )
  target_compile_definitions(${target} PRIVATE ${CustomDefinitions})
  target_include_directories(${target} PRIVATE ${CustomIncludeDirs}
#    INTERFACE
#      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
#      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(${target} ${CustomLibs})

  if (NOT ("${CustomLinkFlags}" MATCHES ""))
    set_target_properties(${target} PROPERTIES LINK_FLAGS ${CustomLinkFlags})
  endif()

  # Force Debug / Release output in the same directory (for MSVC).
  helpers_setTargetOutputDirectory(${target} ${BARBU_BINARY_DIR})
endforeach()

# -----------------------------------------------------------------------------
