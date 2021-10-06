#include "ui/ui_controller.h"

#include <cstdlib>

#include "core/global_clock.h"
#include "core/graphics.h" //
#include "core/logger.h"
#include "core/window.h"

#include "ui/imgui_wrapper.h"
#include "ui/ui_view.h"

// -----------------------------------------------------------------------------

void UIController::init(/*GLFWwindow* window*/) {
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;


  // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
  io.KeyMap[ImGuiKey_Tab]         = symbols::Keyboard::Tab;
  io.KeyMap[ImGuiKey_LeftArrow]   = symbols::Keyboard::Left;
  io.KeyMap[ImGuiKey_RightArrow]  = symbols::Keyboard::Right;
  io.KeyMap[ImGuiKey_UpArrow]     = symbols::Keyboard::Up;
  io.KeyMap[ImGuiKey_DownArrow]   = symbols::Keyboard::Down;
  io.KeyMap[ImGuiKey_PageUp]      = symbols::Keyboard::PageUp;
  io.KeyMap[ImGuiKey_PageDown]    = symbols::Keyboard::PageDown;
  io.KeyMap[ImGuiKey_Home]        = symbols::Keyboard::Home;
  io.KeyMap[ImGuiKey_End]         = symbols::Keyboard::End;
  io.KeyMap[ImGuiKey_Insert]      = symbols::Keyboard::Insert;
  io.KeyMap[ImGuiKey_Delete]      = symbols::Keyboard::Delete;
  io.KeyMap[ImGuiKey_Backspace]   = symbols::Keyboard::BackSpace;
  io.KeyMap[ImGuiKey_Space]       = symbols::Keyboard::Space;
  io.KeyMap[ImGuiKey_Enter]       = symbols::Keyboard::Return;
  io.KeyMap[ImGuiKey_Escape]      = symbols::Keyboard::Escape;
  io.KeyMap[ImGuiKey_A]           = symbols::Keyboard::A;
  io.KeyMap[ImGuiKey_C]           = symbols::Keyboard::C;
  io.KeyMap[ImGuiKey_V]           = symbols::Keyboard::V;
  io.KeyMap[ImGuiKey_X]           = symbols::Keyboard::X;
  io.KeyMap[ImGuiKey_Y]           = symbols::Keyboard::Y;
  io.KeyMap[ImGuiKey_Z]           = symbols::Keyboard::Z;


  //io.SetClipboardTextFn = cb_SetClipboardText;
  //io.GetClipboardTextFn = cb_GetClipboardText;
  
  // io.ClipboardUserData = window_ptr_;

#ifdef _WIN32
  //io.ImeWindowHandle = glfwGetWin32Window(window_ptr_);
#endif

  // Setup style.
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.Alpha = 0.85f;
}

void UIController::deinit() {
  ImGui::DestroyContext();
}

void UIController::update(std::shared_ptr<AbstractWindow> window) {
  if (!device_.fontTexture) {
    create_device_objects();
  }

  ImGuiIO& io = ImGui::GetIO();

  // Setup display size (every frame to accommodate for window resizing)
  int const w = window->width();
  int const h = window->height();
  int const display_w = window->width(); //
  int const display_h = window->height(); //

  io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
  io.DisplayFramebufferScale = ImVec2((w > 0) ? (static_cast<float>(display_w) / w) : 0.0f,
                                      (h > 0) ? (static_cast<float>(display_h) / h) : 0.0f);

  // Setup time step
  auto& gc = GlobalClock::Get();
  io.DeltaTime = gc.delta_time();

  // [TODO] Setup special keys.
  // io.KeyCtrl  = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
  // io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT]   || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
  // io.KeyAlt   = io.KeysDown[GLFW_KEY_LEFT_ALT]     || io.KeysDown[GLFW_KEY_RIGHT_ALT];
  // io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER]   || io.KeysDown[GLFW_KEY_RIGHT_SUPER];

  // Setup inputs
  // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
  if (window->hasFocus()) {
    if (io.WantSetMousePos) {
      window->setCursorPosition(io.MousePos.x, io.MousePos.y);
    } else {
      int mouse_x, mouse_y;
      window->getCursorPosition(&mouse_x, &mouse_y);
      io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
    }
  } else {
    io.MousePos = ImVec2(-FLT_MAX,-FLT_MAX);
  }

  // Update OS/hardware mouse cursor if imgui isn't drawing a software cursor
  if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0) {
    ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
    bool const bShowCursor = !(io.MouseDrawCursor || cursor == ImGuiMouseCursor_None);
    window->showCursor(bShowCursor);
  }

#if 0
  // Gamepad navigation mapping [BETA]
  memset(io.NavInputs, 0, sizeof(io.NavInputs));
  if (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) {
      // Update gamepad inputs
      #define MAP_BUTTON(NAV_NO, BUTTON_NO)       { if (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS) io.NavInputs[NAV_NO] = 1.0f; }
      #define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); if (v > 1.0f) v = 1.0f; if (io.NavInputs[NAV_NO] < v) io.NavInputs[NAV_NO] = v; }
      int axes_count = 0, buttons_count = 0;
      const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
      const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
      MAP_BUTTON(ImGuiNavInput_Activate,   0);     // Cross / A
      MAP_BUTTON(ImGuiNavInput_Cancel,     1);     // Circle / B
      MAP_BUTTON(ImGuiNavInput_Menu,       2);     // Square / X
      MAP_BUTTON(ImGuiNavInput_Input,      3);     // Triangle / Y
      MAP_BUTTON(ImGuiNavInput_DpadLeft,   13);    // D-Pad Left
      MAP_BUTTON(ImGuiNavInput_DpadRight,  11);    // D-Pad Right
      MAP_BUTTON(ImGuiNavInput_DpadUp,     10);    // D-Pad Up
      MAP_BUTTON(ImGuiNavInput_DpadDown,   12);    // D-Pad Down
      MAP_BUTTON(ImGuiNavInput_FocusPrev,  4);     // L1 / LB
      MAP_BUTTON(ImGuiNavInput_FocusNext,  5);     // R1 / RB
      MAP_BUTTON(ImGuiNavInput_TweakSlow,  4);     // L1 / LB
      MAP_BUTTON(ImGuiNavInput_TweakFast,  5);     // R1 / RB
      MAP_ANALOG(ImGuiNavInput_LStickLeft, 0,  -0.3f,  -0.9f);
      MAP_ANALOG(ImGuiNavInput_LStickRight,0,  +0.3f,  +0.9f);
      MAP_ANALOG(ImGuiNavInput_LStickUp,   1,  +0.3f,  +0.9f);
      MAP_ANALOG(ImGuiNavInput_LStickDown, 1,  -0.3f,  -0.9f);
      #undef MAP_BUTTON
      #undef MAP_ANALOG
      if (axes_count > 0 && buttons_count > 0) {
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
      } else {
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
      }
  }
#endif

  ImGui::NewFrame();
}

void UIController::render(bool show_ui) {
  if (!mainview_ptr_) {
    return;
  }

  mainview_ptr_->render();
  ImGui::Render();

  if (show_ui) {
    render_frame(ImGui::GetDrawData());
  }

  CHECK_GX_ERROR();
}

void UIController::render_frame(ImDrawData* draw_data) {
  ImGuiIO& io = ImGui::GetIO();

  // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
  int fb_width = static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
  int fb_height = static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
  if (fb_width == 0 || fb_height == 0) {
    return;
  }
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);

  // Backup GL state
  GLenum last_active_texture          = gx::Get<uint32_t>(GL_ACTIVE_TEXTURE);
  glActiveTexture(GL_TEXTURE0);
  GLuint last_program                 = gx::Get<uint32_t>(GL_CURRENT_PROGRAM);
  GLuint last_texture                 = gx::Get<uint32_t>(GL_TEXTURE_BINDING_2D);
  GLuint last_sampler                 = gx::Get<uint32_t>(GL_SAMPLER_BINDING);
  GLuint last_array_buffer            = gx::Get<uint32_t>(GL_ARRAY_BUFFER_BINDING);
  GLuint last_element_array_buffer    = gx::Get<uint32_t>(GL_ELEMENT_ARRAY_BUFFER_BINDING);
  GLuint last_vertex_array            = gx::Get<uint32_t>(GL_VERTEX_ARRAY_BINDING);
  GLint last_polygon_mode[2];         glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
  GLint last_viewport[4];             glGetIntegerv(GL_VIEWPORT, last_viewport);
  GLint last_scissor_box[4];          glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
  GLenum last_blend_src_rgb           = gx::Get<uint32_t>(GL_BLEND_SRC_RGB);
  GLenum last_blend_dst_rgb           = gx::Get<uint32_t>(GL_BLEND_DST_RGB);
  GLenum last_blend_src_alpha         = gx::Get<uint32_t>(GL_BLEND_SRC_ALPHA);
  GLenum last_blend_dst_alpha         = gx::Get<uint32_t>(GL_BLEND_DST_ALPHA);
  GLenum last_blend_equation_rgb      = gx::Get<uint32_t>(GL_BLEND_EQUATION_RGB);
  GLenum last_blend_equation_alpha    = gx::Get<uint32_t>(GL_BLEND_EQUATION_ALPHA);
  GLboolean last_enable_blend         = glIsEnabled(GL_BLEND);
  GLboolean last_enable_cull_face     = glIsEnabled(GL_CULL_FACE);
  GLboolean last_enable_depth_test    = glIsEnabled(GL_DEPTH_TEST);
  GLboolean last_enable_scissor_test  = glIsEnabled(GL_SCISSOR_TEST);

  //-------------------------

  // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // Setup viewport, orthographic projection matrix
  glViewport(0, 0, fb_width, fb_height);
  float const ortho_projection[4][4] =
  {
    { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
    { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
    { 0.0f,                  0.0f,                  -1.0f, 0.0f },
    {-1.0f,                  1.0f,                   0.0f, 1.0f },
  };
  glUseProgram(device_.shaderHandle);
  glUniform1i(device_.uTex, 0);
  glUniformMatrix4fv(device_.uProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
  glBindSampler(0, 0); // Rely on combined texture/sampler state.

  // Recreate the VAO every time
  // (This is to easily allow multiple GL contexts. VAO are not shared among GL contexts,
  // and we don't track creation/deletion of windows so we don't have an obvious key
  // to use to cache them.)
  GLuint vao_handle = 0;
  glGenVertexArrays(1, &vao_handle);
  glBindVertexArray(vao_handle);
  glBindBuffer(GL_ARRAY_BUFFER, device_.vboHandle);
  glEnableVertexAttribArray(device_.aPosition);
  glEnableVertexAttribArray(device_.aUV);
  glEnableVertexAttribArray(device_.aColor);
  glVertexAttribPointer(device_.aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, pos));
  glVertexAttribPointer(device_.aUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv));
  glVertexAttribPointer(device_.aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, col));

  // Draw
  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    const ImDrawIdx* idx_buffer_offset = nullptr;

    glBindBuffer(GL_ARRAY_BUFFER, device_.vboHandle);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
                 (const GLvoid*)cmd_list->VtxBuffer.Data,
                 GL_STREAM_DRAW
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device_.elementsHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx)),
                 (const GLvoid*)cmd_list->IdxBuffer.Data,
                 GL_STREAM_DRAW
    );
    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->UserCallback) {
        pcmd->UserCallback(cmd_list, pcmd);
      } else {
        GLuint texid = static_cast<GLuint>(reinterpret_cast<intptr_t>(pcmd->TextureId));

        if (!glIsTexture(texid)) {
          LOG_DEBUG_INFO( "UI Controller : Incorrect texture id.");
          continue;
        }
        glBindTexture(GL_TEXTURE_2D, texid);
        //glBindTextureUnit(0, texid);
        CHECK_GX_ERROR();

        glScissor(static_cast<GLint>(pcmd->ClipRect.x),
                  static_cast<GLint>(fb_height - pcmd->ClipRect.w),
                  static_cast<GLint>(pcmd->ClipRect.z - pcmd->ClipRect.x),
                  static_cast<GLint>(pcmd->ClipRect.w - pcmd->ClipRect.y));
        glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
      }
      idx_buffer_offset += pcmd->ElemCount;
    }
  }
  glDeleteVertexArrays(1, &vao_handle);

  //-------------------------

  // Restore modified GL state
  glUseProgram(last_program);
  glBindTexture(GL_TEXTURE_2D, last_texture);
  glBindSampler(0, last_sampler);
  glActiveTexture(last_active_texture);
  glBindVertexArray(last_vertex_array);
  glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
  glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
  glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
  if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
  if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
  if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
  if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
  glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
  glScissor(last_scissor_box[0], last_scissor_box[1], last_scissor_box[2], last_scissor_box[3]);

  CHECK_GX_ERROR();
}

/// --------------------------------------------------------------------------
///
/// What follow is basically the ImGui example reworked.
/// [ TODO : integrate more intuitively with the app ]
///
/// --------------------------------------------------------------------------

void UIController::create_device_objects() {
  // Backup GL state
  GLint last_texture, last_array_buffer, last_vertex_array;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

  const GLchar* vertex_shader =
    "uniform mat4 ProjMtx;\n"
    "in vec2 Position;\n"
    "in vec2 UV;\n"
    "in vec4 Color;\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
      "	Frag_UV = UV;\n"
      "	Frag_Color = Color;\n"
      "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
    "}\n";

  const GLchar* fragment_shader =
    "uniform sampler2D Texture;\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "out vec4 Out_Color;\n"
    "void main()\n"
    "{\n"
      " Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
    "}\n";

  const char glslVersion[32] = "#version 150\n";
  const GLchar* vertex_shader_with_version[2] = { glslVersion, vertex_shader };
  const GLchar* fragment_shader_with_version[2] = { glslVersion, fragment_shader };

  device_.shaderHandle = glCreateProgram();
  device_.vertHandle = glCreateShader(GL_VERTEX_SHADER);
  device_.fragHandle = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(device_.vertHandle, 2, vertex_shader_with_version, nullptr);
  glShaderSource(device_.fragHandle, 2, fragment_shader_with_version, nullptr);
  glCompileShader(device_.vertHandle);
  glCompileShader(device_.fragHandle);
  glAttachShader(device_.shaderHandle, device_.vertHandle);
  glAttachShader(device_.shaderHandle, device_.fragHandle);
  glLinkProgram(device_.shaderHandle);

  if (!gx::CheckProgramStatus(device_.shaderHandle, "ImGui font rendering shader.")) {
    exit(EXIT_FAILURE);
  }

  device_.uTex      = gx::UniformLocation(device_.shaderHandle, "Texture");
  device_.uProjMtx  = gx::UniformLocation(device_.shaderHandle, "ProjMtx");
  device_.aPosition = gx::AttribLocation(device_.shaderHandle, "Position");
  device_.aUV       = gx::AttribLocation(device_.shaderHandle, "UV");
  device_.aColor    = gx::AttribLocation(device_.shaderHandle, "Color");

  glGenBuffers(1, &device_.vboHandle);
  glGenBuffers(1, &device_.elementsHandle);

  create_font_texture();

  // Restore modified GL state
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(last_texture));
  glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(last_array_buffer));
  glBindVertexArray(static_cast<GLuint>(last_vertex_array));

  CHECK_GX_ERROR();
}

void UIController::create_font_texture() {
  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();
  unsigned char* pixels;
  int width, height;

  // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small)
  // because it is more likely to be compatible with user's existing shaders.
  // If your ImTextureId represent a higher-level concept than just a GL texture id,
  // consider calling GetTexDataAsAlpha8() instead to save on GPU memory.
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  // Upload texture to graphics system
  GLint last_texture;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  glGenTextures(1, &device_.fontTexture);
  glBindTexture(GL_TEXTURE_2D, device_.fontTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Store our identifier
  io.Fonts->TexID = (void *)(intptr_t)device_.fontTexture;

  // Restore state
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(last_texture));
}

// -----------------------------------------------------------------------------
