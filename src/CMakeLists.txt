# -----------------------------------------------------------------------------
# Sources.
# -----------------------------------------------------------------------------

set(TARGET_NAME "${CMAKE_PROJECT_NAME}_demo")

list(APPEND Sources
  main.cc
  scene.cc

  core/app.cc
  core/events.cc
  core/graphics.cc

  ecs/scene_hierarchy.cc

  fx/gpu_particle.cc
  fx/grid.cc
  fx/hair.cc
  fx/marschner.cc
  fx/skybox.cc
  fx/postprocess/postprocess.cc
  fx/postprocess/hbao.cc

  memory/assets/assets.cc
  memory/assets/mesh.cc
  memory/assets/texture.cc
  memory/assets/program.cc
  memory/assets/material_asset.cc
  memory/resources/image.cc
  memory/resources/mesh_data.cc
  memory/resources/resources.cc
  memory/resources/shader.cc
  memory/pingpong_buffer.cc
  memory/random_buffer.cc

  utils/gizmo.cc

  ui/ui_controller.cc

  ui/views/SceneView.cc
  ui/views/Main.cc
  ui/views/fx/SparkleView.cc
)

list(APPEND Headers
  scene.h
  
  core/app.h
  core/events.h
  core/graphics.h

  ecs/ecs.h
  ecs/entity.h
  ecs/component.h
  ecs/scene_hierarchy.h
  
  fx/gpu_particle.h
  fx/grid.h
  fx/hair.h
  fx/irradiance_env_map.h
  fx/marschner.h
  fx/skybox.h
  fx/postprocess/postprocess.h
  fx/postprocess/hbao.h

  memory/assets/asset_factory.h
  memory/assets/assets.h
  memory/assets/mesh.h
  memory/assets/texture.h
  memory/assets/program.h
  memory/assets/material_asset.h
  memory/resources/image.h
  memory/resources/mesh_data.h
  memory/resources/resource_info_list.h
  memory/resources/resource_manager.h
  memory/resources/resources.h
  memory/resources/shader.cc
  memory/hash_id.h
  memory/null_vector.h
  memory/pingpong_buffer.h
  memory/random_buffer.h

  utils/arcball_controller.h
  utils/cpu_particle.h
  utils/gizmo.h

  ui/imgui_wrapper.h
  ui/ui_controller.h
  ui/ui_view.h

  ui/views/SceneView.h
  ui/views/fx/HairView.h
  ui/views/fx/MarschnerView.h
  ui/views/fx/SparkleView.h
  ui/views/Main.h
  ui/views/views.h
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
  target_include_directories(${target} 
    PRIVATE 
      ${CustomIncludeDirs}
      
#    INTERFACE
#      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
#      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(${target} PRIVATE ${CustomLibs})
  set_target_properties(${target} PROPERTIES LINK_FLAGS ${CustomLinkFlags})

  # Force Debug / Release output in the same directory (for MSVC).
  helpers_setTargetOutputDirectory(${target} ${BARBU_BINARY_DIR})
endforeach()

# -----------------------------------------------------------------------------
