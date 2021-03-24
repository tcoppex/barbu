#include "memory/resources/shader.h"

#include <array>
#include <regex>
#include "core/graphics.h"

// ----------------------------------------------------------------------------

namespace {

// Detect the type of shader file by comparing the file's basename to a
// prefix or suffix pattern.
ShaderType GetShaderTypeFromName(std::string const& basename) {
  std::array< std::string[2], kNumShaderType> const tokens{{ 
    {"vert", "vs"},
    {"tesc", "tcs"},
    {"tese", "tes"},
    {"geom", "gs"},
    {"frag", "fs"},
    {"comp", "cs"}, 
  }};

  char s[64];
  for (int i = 0; i < kNumShaderType; ++i) {
    auto const& token = tokens[i];
    sprintf(s, "((%s|%s)_.+\\.glsl)|(.+\\.(%s|%s)(\\.glsl)?)", 
      token[0].c_str(), token[1].c_str(), token[0].c_str(), token[1].c_str()
    );

    std::regex re(s);
    if ( std::regex_match(basename, re) ) {
      return ShaderType(i);
    }
  }

  return ShaderType::kNone;
}

// ----------------------------------------------------------------------------

bool ReadFile(const char* filename, const unsigned int maxsize, char out[]) {
  FILE* fd = nullptr;
  size_t nelems = 0;
  size_t nreads = 0;

  if (!(fd = fopen(filename, "r"))) {
    LOG_WARNING( "\"", filename , "\" not found." ); //
    return false;
  }
  memset(out, 0, maxsize);

  fseek(fd, 0, SEEK_END);
  nelems = static_cast<size_t>(ftell(fd));
  nelems = (nelems > maxsize) ? maxsize : nelems;
  fseek(fd, 0, SEEK_SET);

  nreads = fread(out, sizeof(char), nelems, fd);
  fclose(fd);

  return nreads == nelems;
}

/* Return true if the given filename is in the list of special extensions. */
bool IsSpecialFile(char const* fn) {
  const char* exts[]{ ".hpp" };

  const size_t length_fn = strlen(fn);
  for (auto ext : exts) {
    const size_t length_ext = strlen(ext);
    if (0 == strncmp(fn + length_fn-length_ext, ext, length_ext)) {
      return true;
    }
  }

  return false;
}

/* Read the shader and process the #include preprocessors. */
bool ReadShaderFile(char const* filename, unsigned int const maxsize, char out[], int *level) {
  char const * substr = "#include \"";
  size_t const len = strlen(substr);
  char *first = nullptr;
  char *last = nullptr;
  char include_fn[64u]{0};
  char include_path[256u]{0};
  size_t include_len = 0u;

  /* Prevent long recursive includes */
  if (*level <= 0) {
    return false;
  }
  --(*level);

  /* Read the shaders */
  if (!ReadFile(filename, maxsize, out)) {
    return false;
  }

  /* Check for include file an retrieve its name */
  last = out;

  while (nullptr != (first = strstr(last, substr))) {
    /* pass commented include directives */
    if ((first != out) && (*(first-1) != '\n')) {
      last = first + 1;
      continue;
    }
    first += len;

    last = strchr(first, '"');
    if (!last) {
      return false;
    }

    /* Copy the include file name */
    include_len = static_cast<size_t>(last-first);
    strncpy(include_fn, first, include_len);
    include_fn[include_len] = '\0';

    /* Set include global path */
    sprintf(include_path, "%s/%s", SHADERS_DIR, include_fn);

    /* Create memory to hold the include file */
    char *include_file = reinterpret_cast<char*>(calloc(maxsize, sizeof(char)));

    /* Retrieve the include file */
    if (!IsSpecialFile(include_path)) {
      ReadShaderFile(include_path, maxsize, include_file, level);
    }

    /* Add the line directive to the included file */
    sprintf(include_file, "%s\n#line 0", include_file);

    /* Add the second part of the shader */
    last = strchr(last, '\n');
    sprintf(include_file, "%s\n%s", include_file, last);

    /* Copy it back to the shader buffer */
    sprintf(first-len, "%s", include_file);

    /* Free include file data */
    free(include_file);
  }

  return true;
}

bool ReadShaderFile(std::string_view filename, int32_t const maxsize, char out[]) {
  /// Simple way to deal with include recursivity, without reading guards.
  /// Known limitations : do not handle loop well.

  int max_level = 8;
  bool const result = ReadShaderFile(filename.data(), maxsize, out, &max_level);
  if (max_level < 0) {
    LOG_ERROR( filename, ": too many nested includes found.");
  }
  return result;
}


} // namespace

// ----------------------------------------------------------------------------

void Shader::release() {
  if (loaded()) {
    glDeleteShader(id);
    id = 0u;
  }
  CHECK_GX_ERROR();
}

int Shader::target() const {
  std::array<GLenum, ShaderType::kNumShaderType> constexpr targets{
    GL_VERTEX_SHADER, 
    GL_TESS_CONTROL_SHADER, 
    GL_TESS_EVALUATION_SHADER, 
    GL_GEOMETRY_SHADER, 
    GL_FRAGMENT_SHADER, 
    GL_COMPUTE_SHADER,
  };

  return targets[type];
}

// ----------------------------------------------------------------------------

ShaderManager::Handle ShaderManager::_load(ResourceId const& id) {
  ShaderManager::Handle h(id);

  std::string const& filename(id.path);
  if (!ReadShaderFile(filename, kMaxShaderBufferSize, buffer_)) {
    LOG_WARNING( "ShaderManager : unknown file \"", filename, "\"." );
    return h;
  }
  auto shader = h.data;

  // Compile and store the shader.
  shader->type = GetShaderTypeFromName(h.name);
  shader->id = glCreateShader( shader->target() );
  glShaderSource(shader->id, 1, (const GLchar**)&buffer_, nullptr);
  glCompileShader(shader->id);
  
  if (!gx::CheckShaderStatus(shader->id, h.name.c_str())) {
    shader->release();
  }

  return h;
}

// ----------------------------------------------------------------------------