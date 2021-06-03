#include "memory/resources/mesh_data.h"

#include <clocale>
#include "glm/gtc/type_ptr.hpp"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

#include "memory/assets/assets.h"
#include "utils/mathutils.h"

// ----------------------------------------------------------------------------

namespace {

// Load a file using an external buffer.
//
// If the buffer is empty or its size is not sufficient it will allocate it.
// Return true when the file has been loaded correctly, false otherwhise.
//
// When true is returned the user is responsible for the buffer deallocation,
// otherwhise it will be freed automatically.
bool LoadFile(std::string_view filename, char **buffer, size_t *buffersize) {
  assert( (buffer != nullptr) && (buffersize != nullptr) );
  FILE *fd = fopen( filename.data(), "r");
  
  if (nullptr == fd) {
    LOG_ERROR( filename, "does not exists.");
    return false;
  }

  // Determine its size.
  fseek(fd, 0, SEEK_END);
  size_t const filesize = static_cast<size_t>(ftell(fd));
  fseek(fd, 0, SEEK_SET);

  // Allocate the buffer.
  if ((*buffer != nullptr) && (*buffersize < filesize)) {
    delete [] *buffer;
    *buffer = nullptr;
  }
  if (*buffer == nullptr) {
    *buffer = new char[filesize]();
    *buffersize = filesize;
  }

  if (*buffer == nullptr) {
    LOG_ERROR( filename, " : buffer allocation failed.");
    fclose(fd);
    return false; 
  }

  // Read it.
  size_t const rbytes = fread(*buffer, 1u, filesize, fd);
  
  bool const succeed = (rbytes == filesize) || (feof(fd) && !ferror(fd));
  fclose(fd);

  if (!succeed) {
    LOG_ERROR( "Failed to load file :", filename);
    delete [] *buffer;
    return false;
  }
  
  return succeed;
}

// This method parses all the geometry and fed 'raw' in one pass.
// It is destructive of the input buffer but should always works,
// note however that the data could be easily restored after each iteration.
void ParseOBJ(char *input, RawMeshFile &meshfile, bool bSeparateObjects) {
  using Vec2f = glm::vec2;
  using Vec3f = glm::vec3;
  using Vec3i = glm::ivec3;

  char namebuffer[128]{'\0'};

  // If no objects are specified, create a default raw geo.
  if (meshfile.meshes.empty()) {
    meshfile.meshes.resize(1);
  } 

  char *s = input;
  for (char *end = strchr(s, '\n'); end != nullptr; s = end+1u, end = strchr(s, '\n'))
  {
    *end = '\0';

    // Handle Objects, Submesh & Material file.
    if (s[0] & 1) {
      // OBJECT

      if (s[0] == '#') {
        continue;
      }

      if ((s[0] == 'o')) {//} || (s[0] == 'g')) {

        // [wip]
        if (bSeparateObjects) {
          // [ Objects are defined by vertices, but maybe the whole OBJ is considered
          // as a unique buffer, so test it ] 
          meshfile.meshes.resize(meshfile.meshes.size() + 1);

          sscanf( s+2, "%128s", namebuffer);
          meshfile.meshes.back().name = std::string(namebuffer);
        }
      } 
      // MATERIAL ID (usemtl)
      else if (s[0] == 'u') {
        auto &raw = meshfile.meshes.back();

        int32_t const last_vertex_index = static_cast<int32_t>(raw.elementsAttribs.size()) - 1;

        // Set last index of current submesh.
        if (!raw.vgroups.empty()) {
          raw.vgroups.back().end_index = last_vertex_index;
        }

        // Add a new submesh.
        raw.vgroups.resize(raw.vgroups.size() + 1);
        auto &vg = raw.vgroups.back();

        // Set name and first vertex index.
        sscanf( s+7, "%128s", namebuffer);
        vg.name = std::string(namebuffer);
        vg.start_index = last_vertex_index + 1;
      } 
      // MATERIAL FILE ID (mtlib)
      else if ((s[0] == 'm') && (s[1] == 't')) {
        sscanf( s+7, "%128s", namebuffer);
        meshfile.material_id = std::string(namebuffer);
      }

      // [the other captured parameters has their first char last bit set to 0, so we can skip here]
      continue;
    }

    // Update the last geometry in the buffer.
    auto &raw = meshfile.meshes.back();

    // VERTEX
    if (s[0] == 'v') {
      if (s[1] == ' ')
      {
        Vec3f v;
        sscanf(s+2, "%f %f %f", &v.x, &v.y, &v.z);
        raw.vertices.push_back(v);
      }
      else if (s[1] == 't')
      {
        Vec2f v;
        sscanf(s+3, "%f %f", &v.x, &v.y);
        v.y = 1.0f - v.y; // (reverse y)
        raw.texcoords.push_back(v);
      }
      else //if (s[1] == 'n')
      {
        Vec3f v;
        sscanf(s+3, "%f %f %f", &v.x, &v.y, &v.z);
        raw.normals.push_back(v);
      }
    }
    // FACE
    else if (s[0] == 'f') {
      // vertex attribute indices, will be set to 0 (-1 after postprocessing)
      // if none exists.
      Vec3i f(0), ft(0), fn(0);

      // This vector is fed by the 4th extra attribute, if any, to perform
      // auto-triangularization for quad faces.
      Vec3i ext(0), neo(0);

      bool const has_texcoords = !raw.texcoords.empty();
      bool const has_normals   = !raw.normals.empty();

      if (has_texcoords && has_normals)
      {
        sscanf(s+2, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", &f.x, &ft.x, &fn.x,
                                                           &f.y, &ft.y, &fn.y,
                                                           &f.z, &ft.z, &fn.z,
                                                           &ext.x, &ext.y, &ext.z);
      }
      else if (has_texcoords)
      {
        sscanf(s+2, "%d/%d %d/%d %d/%d %d/%d", &f.x, &ft.x,
                                               &f.y, &ft.y,
                                               &f.z, &ft.z,
                                               &ext.x, &ext.y);
      }
      else if (has_normals)
      {
        sscanf(s+2, "%d//%d %d//%d %d//%d %d//%d", &f.x, &fn.x,
                                                   &f.y, &fn.y,
                                                   &f.z, &fn.z,
                                                   &ext.x, &ext.z);
      }
      else // only vertex ids
      {
        sscanf(s+2, "%d %d %d %d", &f.x, &f.y, &f.z, &ext.x);
      }

      raw.elementsAttribs.push_back( Vec3i(f.x, ft.x, fn.x) );
      raw.elementsAttribs.push_back( Vec3i(f.y, ft.y, fn.y) );
      raw.elementsAttribs.push_back( Vec3i(f.z, ft.z, fn.z) );
      if (ext.x > 0) {
        raw.elementsAttribs.push_back( Vec3i(f.z, ft.z, fn.z) );
        raw.elementsAttribs.push_back( Vec3i(ext.x, ext.y, ext.z) );
        raw.elementsAttribs.push_back( Vec3i(f.x, ft.x, fn.x) );
      }
    }
  }

  // Post-process mesh with indices.
  for (auto &raw : meshfile.meshes) {
    if (raw.elementsAttribs.empty()) {
      continue;
    }

    // Change range of indices from [1, n] to [0, n-1].
    for (auto &v : raw.elementsAttribs) { 
      v.x -= 1; 
      v.y -= 1; 
      v.z -= 1; 
    }
  
    // Create a default vertex group if none were specified.
    if (!raw.has_vertex_groups()) {
      raw.vgroups.resize(1);
      raw.vgroups[0].name = MeshData::kDefaultGroupName;
    }

    // Update border groups indices.
    auto &vgroups = raw.vgroups;
    vgroups.front().start_index = 0;
    vgroups.back().end_index = static_cast<int32_t>(raw.elementsAttribs.size());
  }
}

void ParseMTL(char *input, MaterialFile &matfile) {
  // Check if a string match a token.
  auto check_token = [](std::string_view s, std::string_view token) {
    return 0 == strncmp( s.data(), token.data(), token.size());
  };

  auto &materials = matfile.infos;
  char namebuffer[128]{'\0'};

  // For MTL, as there is no way to determine proper shading properties, we
  // decide that materials are :
  //  * opaque by default, 
  //  * alpha_tested if they have a map_Kd (with a potential alpha value) or an map_d, 
  //  * transparent if they have an opacity value ('d').
  //  * double sided when certain tokens are found in the material name, &
  //  * unlit if there is no specular / normal map as well.
  //
  // Bump map are bypassed because there are often not used as normal maps.
  //

  char *s = input;
  for (char *end = strchr(s, '\n'); end != nullptr; s=end+1u, end = strchr(s, '\n')) //
  {
    *end = '\0';
    
    // New material.
    if (s[0] == 'n') {
      materials.resize(materials.size() + 1);
      sscanf( s+7, "%128s", namebuffer);
      auto matname = std::string(namebuffer);
      
      auto &mat = materials.back();
      mat.name = matname;

      // Detect with name flat geometries for specific materials.
      // [ should rather detect topology / normal ].
      std::array<std::string, 3> const flatten_tokens{
        "foliage", "leaf", "cross"
      };

      // Use lower case name.
      std::transform( matname.begin(), matname.end(), matname.begin(), ::tolower);
      for (auto const& token : flatten_tokens) {
        if (matname.find(token) != matname.npos) {
          LOG_DEBUG_INFO( "OBJ / MTL : flat material detected for [", mat.name, "] ." );
          mat.bDoubleSided = true;
          mat.bUnlit = true;
        }
      }
      continue;
    }

    auto &mat = materials.back();

    if (!(s[0] & 1)) {
      continue;
    }
    
    if ((s[0] == 'm') && (s[1] == 'a') && (s[2] == 'p'))
    {
      if (check_token(s, "map_Kd")) {
        sscanf( s+7, "%128s", namebuffer);
        mat.diffuse_map = std::string(namebuffer);
        mat.bAlphaTest = true;
      } else if (check_token(s, "map_Ks")) {
        sscanf( s+7, "%128s", namebuffer);
        mat.specular_map = std::string(namebuffer);
        mat.bUnlit = false;
      } else if (check_token(s, "map_Bump -bm")) {
        // float tmp;
        // sscanf( s+13, "%f %128s", &tmp, namebuffer);
        // mat.bump_map = std::string( namebuffer );
        // mat.bUnlit = false;
        // // (sometimes the bump might have a scale factor we bypass)
        // LOG_WARNING("[MTL Loader] Bypassed bump factor", tmp, "for file :\n", namebuffer);
      } else if (check_token(s, "map_Bump")) {
        // sscanf( s+9, "%128s", namebuffer);
        // mat.bump_map = std::string( namebuffer );
        // mat.bUnlit = false;
      } else if (check_token(s, "map_d")) {
        sscanf( s+6, "%128s", namebuffer);
        mat.alpha_map = std::string( namebuffer );
        mat.bAlphaTest = true;
      }
    }
    // Colors.
    else if (s[0] == 'K')
    {
      glm::vec3 v;
      sscanf(s+2, "%f %f %f", &v.x, &v.y, &v.z);

      // Diffuse.
      if (s[1] == 'd') {
        mat.diffuse_color.x = v.x;
        mat.diffuse_color.y = v.y;
        mat.diffuse_color.z = v.z;
      }
      // Specular.
      else if (s[1] == 's') {
        mat.specular_color.x = v.x;
        mat.specular_color.y = v.y;
        mat.specular_color.z = v.z;
      }
    }
    // Alpha value.
    else if (s[0] == 'd') {
      sscanf(s+2, "%f", &mat.diffuse_color.w);
      mat.bBlending = true;
    }
    // Specular exponent.
    else if ((s[0] == 'N') && (s[1] == 's')) {
      sscanf(s+3, "%f", &mat.specular_color.w);
    } 
  }
}

} // namespace

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool MeshDataManager::CheckExtension(std::string_view _ext) {
  std::string ext(_ext);
  std::transform( _ext.cbegin(), _ext.cend(), ext.begin(), ::tolower);
  
  return ("obj" == ext)
      || ("glb" == ext)
      || ("gltf" == ext)
      ;  
}

// ----------------------------------------------------------------------------

MeshDataManager::Handle MeshDataManager::_load(ResourceId const& id) {
  MeshDataManager::Handle h(id);

  auto const path = id.path;
  auto ext = path.substr(path.find_last_of(".") + 1);
  std::transform( ext.cbegin(), ext.cend(), ext.begin(), ::tolower);

  auto &meshdata = *h.data;

  if ("obj" == ext) {
    load_obj( path, meshdata);
  } else if (("glb" == ext) || ("gltf" == ext)) {
    load_gltf( path, meshdata);
  } else {
    LOG_WARNING( ext, "models are not supported." );
  }

  return h;
}

// ----------------------------------------------------------------------------

bool MeshDataManager::load_obj(std::string_view filename, MeshData &meshdata) {
  char *buffer = nullptr;
  size_t buffersize = 0uL;

  if (!LoadFile(filename, &buffer, &buffersize)) {
    return false;
  }

#ifdef WIN32
  // (to prevent expecting comas instead of points when parsing floating point values)
  char *locale = std::setlocale(LC_ALL, nullptr);
  std::setlocale(LC_ALL, "C");
#endif

  constexpr bool bSplitObjects = false; //

  // Parse the OBJ for raw mesh data.
  RawMeshFile meshfile;
  ParseOBJ(buffer, meshfile, bSplitObjects);

  // Handle MTL file materials if any.
  if (!meshfile.material_id.empty()) {
    constexpr bool debug_log = false;

    // Determine the material file full path.
    std::string fn( filename );
    // OBJ mesh directory name.
    std::string dirname = fn.substr(0, fn.find_last_of('/'));
    // Add the material relative path.
    fn = dirname + "/" + meshfile.material_id;

    // Trim material id.
    auto &mtl = meshdata.material;
    mtl.id = meshfile.material_id;
    mtl.id = mtl.id.substr(0, mtl.id.find_last_of('.'));

    // Load & Parse the material file.
    if (LoadFile(fn, &buffer, &buffersize)) {
      ParseMTL( buffer, mtl);
    }
    meshfile.prefix_material_vg_names(mtl);

    // Transform relative paths to absolute ones.
    for (auto &mat : mtl.infos) {
      if (auto &m = mat.diffuse_map; !m.empty() && m[0] != '/')  m = dirname + "/" + m;
      if (auto &m = mat.specular_map; !m.empty() && m[0] != '/') m = dirname + "/" + m;
      if (auto &m = mat.bump_map; !m.empty() && m[0] != '/')     m = dirname + "/" + m;
      if (auto &m = mat.alpha_map; !m.empty() && m[0] != '/')    m = dirname + "/" + m;
    }

    // Display a debug log.
    if constexpr (debug_log) {
      LOG_INFO( "\nMTL :", mtl.id );
      // for (auto &vg : meshfile.meshes[0].vgroups) {
      //   LOG_INFO( " >", vg.name );
      // }
      for (auto &mat : mtl.infos) {
        LOG_INFO(" >", mat.name);
        LOG_MESSAGE(" * Diffuse color :", mat.diffuse_color.x, mat.diffuse_color.y, mat.diffuse_color.z);
        if (auto &m = mat.diffuse_map; !m.empty())  LOG_MESSAGE(" * Diffuse  :", m);
        if (auto &m = mat.specular_map; !m.empty()) LOG_MESSAGE(" * Specular :", m);
        if (auto &m = mat.bump_map; !m.empty())     LOG_MESSAGE(" * Bump     :", m);
        if (auto &m = mat.alpha_map; !m.empty())    LOG_MESSAGE(" * Alpha    :", m);
      }
    }
  }
  delete [] buffer;

#ifdef WIN32
  // reset to previous locale.
  std::setlocale(LC_ALL, locale);
#endif

  bool const bNeedTangents = false; //!mat.bump_map.empty(); //

  return meshdata.setup( meshfile, bNeedTangents );
}

// ----------------------------------------------------------------------------

namespace {

std::string SetupTextureGLTF(cgltf_texture *tex, std::string const& dirname, std::string const &default_name) {
  std::string texname;

  if (!tex) {
    return texname;
  }

  auto img = tex->image;
 
  if (img->uri) {
    // GLTF file with external data.
    
    if (img->uri[0] != '/') {
      texname = dirname + "/" + std::string(img->uri);
    } else {
      texname = std::string(img->uri);
    }
  } else {
    // GLB / GLTF file with internal data.

    texname = img->name ? img->name : default_name; //
    if (img->buffer_view) {
      auto buffer_view = img->buffer_view;

      // Create the resource internally.
      Resources::LoadInternal<Image>( 
        ResourceId(texname), 
        buffer_view->size, 
        ((uint8_t*)buffer_view->buffer->data) + buffer_view->offset,
        img->mime_type
      ); 
      // [optional] Create the texture directly.
      //TEXTURE_ASSETS.create2d(AssetId(texname)); 
    }
  }

  return texname;
}

void LoadAnimationGLTF(std::string const& basename, cgltf_data const* data, MeshData &meshdata) {
  std::vector<float> inputs;
  std::vector<float> outputs;
  char tmpname[256]{};

  auto skl = meshdata.skeleton;
  if (nullptr == skl) {
    LOG_ERROR( "GLTF : non skeletal animation are not supported yet." );
    return;
  }

  int32_t const njoints{ skl->njoints() };
  LOG_INFO( "> ", basename, ":", njoints, "joint(s),", data->animations_count, "animation(s).");

  skl->clips.resize( data->animations_count );
  
  for (cgltf_size i = 0; i < data->animations_count; ++i) {
    auto const& anim = data->animations[i];
    auto &clip = skl->clips[i];
    
    assert(anim.samplers_count == anim.channels_count);
    
    // Determine animation clip name.
    sprintf(tmpname, "%s::anim_%02d", basename.c_str(), int(i)); //
    auto const clipname = anim.name ? anim.name : tmpname;
    LOG_DEBUG_INFO( ">", clipname, "[", anim.channels_count, "channels ]");

    // Parse each channels (eg. translation, rotation, scale, weights, ..).
    cgltf_accessor *last_input_accessor = nullptr;
    for (cgltf_size channel_id = 0; channel_id < anim.channels_count; ++channel_id) {
      auto const& channel = anim.channels[channel_id];
      auto const& node_name = channel.target_node->name ? channel.target_node->name : "[no_name]";
      auto const target_path = channel.target_path;
      auto const sampler = channel.sampler;
      auto const out_type = sampler->output->type;
      auto const out_size = out_type * sampler->output->count;

      if (sampler->input->type != cgltf_type_scalar) {
        LOG_ERROR( "GLTF : non compatible data sampling input type." );
        break;
      }

      // Access input / output buffer data.
      if (sampler->input != last_input_accessor) {
        inputs.clear();
        inputs.resize(sampler->input->count);
        cgltf_accessor_unpack_floats( sampler->input, inputs.data(), sampler->input->count); //
        last_input_accessor = sampler->input;
      }
      outputs.clear();
      outputs.resize(out_size);
      cgltf_accessor_unpack_floats( sampler->output, outputs.data(), out_size); //

      auto const nsamples = static_cast<int32_t>(inputs.size());

      // Initialize the clip memory.
      if (channel_id == 0) {
        auto const clip_duration = glm::max( inputs.back(), glm::epsilon<float>());
        //LOG_INFO( __FUNCTION__, "(init clip)", nsamples, clip_duration, njoints, "(out_size", out_size, ")");

        clip = AnimationClip_t(clipname, nsamples, clip_duration);
        for (auto &sample : clip.samples) {
          sample.joints.resize( njoints );
        }
      }

      // ---------------------------

      LOG_CHECK( skl->index_map.find(node_name) != skl->index_map.end() );
      int32_t const joint_id = skl->index_map[node_name]; 

      // 
      if ((target_path == cgltf_animation_path_type_translation) && (out_type == cgltf_type_vec3)) {

        for (int sample_id = 0; sample_id < nsamples; ++sample_id) {
          auto &sample = clip.samples[sample_id];
          auto &joint = sample.joints[joint_id];
          float *v = &outputs[sample_id * out_type];

          joint.vTranslation = glm::vec3(v[0], v[1], v[2]);
        }
      } else if ((target_path == cgltf_animation_path_type_rotation) && (out_type == cgltf_type_vec4)) {

        for (int sample_id = 0; sample_id < nsamples; ++sample_id) {
          auto &sample = clip.samples[sample_id];
          auto &joint = sample.joints[joint_id];
          float *v = &outputs[sample_id * out_type];
          joint.qRotation = glm::quat(v[3], v[0], v[1], v[2]);
        }
      } else if ((target_path == cgltf_animation_path_type_scale) && (out_type == cgltf_type_vec3)) {

        for (int sample_id = 0; sample_id < nsamples; ++sample_id) {
          auto &sample = clip.samples[sample_id];
          auto &joint = sample.joints[joint_id];
          float *v = &outputs[sample_id * out_type];

          float const eps{ 1.e-4f };
          if (almost_equal(v[0], v[1], eps) && almost_equal(v[0], v[2], eps)) {
            joint.fScale = v[0]; 
          } else {
            LOG_WARNING( "GLTF : non uniform scale are not supported for skin animation.", v[0], v[1], v[0] );
          }
        }

      } else if ((target_path == cgltf_animation_path_type_weights) && (out_type == cgltf_type_scalar)) {
        LOG_WARNING( "GLTF : BlendShape animation are not supported." );
      } else {
        LOG_WARNING( "GLTF : unknown animation format requested." );
      }
    }
  }
}

} // namespace

// -----------------------------------------------------------------------------

bool MeshDataManager::load_gltf(std::string_view filename, MeshData &meshdata) {
  cgltf_options options{};
  cgltf_data* data = nullptr;
  cgltf_result result = cgltf_parse_file(&options, filename.data(), &data);
  
  if (result != cgltf_result_success) {
    LOG_WARNING( "GLTF : failed to parse :", filename );
    return false;
  }

  // Load buffers data.
  result = cgltf_load_buffers(&options, data, filename.data());

  if (result != cgltf_result_success) {
    LOG_WARNING( "GLTF : failed to load buffers :", filename );
    return false;
  }

  std::string basename(filename);
  basename = Logger::TrimFilename(basename);
  basename = basename.substr(0, basename.find_last_of('.'));

  LOG_DEBUG_INFO( "Loading mesh", basename );

  /* ------------ */

  bool bNeedTangents = false;

  RawMeshFile meshfile;
  {
    // We will join all meshes as a single one.
    if (meshfile.meshes.empty()) {
      meshfile.meshes.resize(1);
    }

    char tmpname[256]{};

    // Map to solve name for unknown materials. Key is material's pointer in datastructure.
    std::unordered_map< cgltf_material const*, std::string > material_names( data->materials_count );
    for (cgltf_size i = 0; i < data->materials_count; ++i) {
      cgltf_material const& mat = data->materials[i];
      sprintf(tmpname, "%s::material_%02d", basename.c_str(), int(i));
      material_names[ &mat ] = std::string( mat.name ? mat.name : tmpname );
    }

    // -- MESH ATTRIBUTES & INDICES.
    cgltf_size last_vertex_index = 0;
    for (cgltf_size i=0; i < data->nodes_count; ++i) {
      auto node = data->nodes[i];
      auto *mesh = node.mesh;

      if (!mesh) {
        continue;
      }

      // Node's transform.
      glm::mat4 world_matrix;
      cgltf_node_transform_world( &node, glm::value_ptr(world_matrix));

      // RawMeshData to update.
      auto &raw = meshfile.meshes.back();
      sprintf(tmpname, "%s::mesh_%02d", basename.c_str(), int(i));
      raw.name = std::string(mesh->name ? mesh->name : node.name ? node.name : tmpname); //

      // When meshes are not joined, we should probably reset this to 0.
      //last_vertex_index = 0;

      // Morph Targets.
      if (auto ntargets = mesh->target_names_count; ntargets) {
        LOG_DEBUG_INFO( basename, "has", ntargets, "targets :");
        for (cgltf_size j = 0; j < ntargets; ++j) {
          LOG_DEBUG_INFO( j, mesh->target_names[j]);
        }
      }

      // Primitives / submeshes.
      for (cgltf_size j = 0; j < mesh->primitives_count; ++j) {
        auto const& prim = mesh->primitives[j];

        if (prim.has_draco_mesh_compression) {
          LOG_WARNING( "GLTF : Draco mesh compression is not supported." );
          continue;
        }

        if (prim.type != cgltf_primitive_type_triangles) {
          LOG_WARNING( "GLTF : non TRIANGLES primitives are not implemented :", raw.name );
        }

        /*
        if (prim.targets_count > 0) {
          LOG_WARNING( "GLTF : morph targets are not supported.");
          LOG_MESSAGE(j, prim.targets_count, "morph targets available.");
        }

        for (cgltf_size target_index = 0; target_index < prim.targets_count; ++target_index) {
          auto target = prim.targets[target_index];
          // LOG_MESSAGE( target_index, target.attributes_count);

          for (cgltf_size attrib_index = 0; attrib_index < target.attributes_count; ++attrib_index) {
            auto attrib = target.attributes[attrib_index];
            // LOG_MESSAGE( "  ", attrib.name ? attrib.name : "", attrib.type, attrib.index, attrib.data);
            LOG_MESSAGE( attrib.data->buffer_view->name ? attrib.data->buffer_view->name : "-" );
          }    
        }
        */

        // Attributes.
        for (cgltf_size attrib_index = 0; attrib_index < prim.attributes_count; ++attrib_index) {
          auto const& attrib = prim.attributes[attrib_index];

          // Check accessors layout.
          if (attrib.data->is_sparse) {
            LOG_WARNING( "GLTF : sparse attributes are not supported." );
            continue;
          }
          //LOG_MESSAGE("Accessor type :", attrib.data->component_type);

          // Used to reserve memory.
          auto const attribsize = attrib.data->count;

          // Positions.
          if (attrib.type == cgltf_attribute_type_position) {
            LOG_DEBUG( "> loading positions." );
            LOG_CHECK(attrib.data->type == cgltf_type_vec3);
            raw.vertices.reserve(attribsize);
            
            glm::vec3 vertex;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_float( attrib.data, index, glm::value_ptr(vertex), 3);
              vertex = glm::vec3(world_matrix * glm::vec4(vertex, 1.0f));
              raw.vertices.push_back( vertex );
            }
          }
          // Normals.
          else if (attrib.type == cgltf_attribute_type_normal) {
            LOG_DEBUG( "> loading normals." );
            LOG_CHECK(attrib.data->type == cgltf_type_vec3);
            raw.normals.reserve(attribsize);
            
            glm::vec3 normal;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_float( attrib.data, index, glm::value_ptr(normal), 3);
              normal = glm::vec3(world_matrix * glm::vec4(normal, 0.0f));
              raw.normals.push_back( normal );
            }
          }
          // Tangents
          else if (attrib.type == cgltf_attribute_type_tangent) {
            LOG_DEBUG( "> loading tangents." );
            LOG_CHECK(attrib.data->type == cgltf_type_vec4);
            raw.tangents.reserve(attribsize);
            
            glm::vec4 tangent;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_float( attrib.data, index, glm::value_ptr(tangent), 4);
              glm::vec3 t3 = glm::vec3(tangent);
                        t3 = glm::vec3(world_matrix * glm::vec4(t3, 0.0f));
              tangent = glm::vec4(t3, tangent.w);
              raw.tangents.push_back( tangent );
            }
          }
          // Texcoords. [check which index is used !]
          else if (attrib.type == cgltf_attribute_type_texcoord) {
            LOG_CHECK(attrib.data->type == cgltf_type_vec2);
            LOG_CHECK(attrib.index == 0);

            if (attrib.index > 0) {
              LOG_WARNING( "Multitexturing is not supported yet." );
              continue;
            }
            LOG_DEBUG( "> loading texture coordinates." );

            raw.texcoords.reserve(attribsize);

            glm::vec2 texcoord;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_float( attrib.data, index, glm::value_ptr(texcoord), 2);
              raw.texcoords.push_back( texcoord );
            }
          }
          // Joints.
          else if (attrib.type == cgltf_attribute_type_joints) {
            LOG_DEBUG( "> loading joint indices." );
            LOG_CHECK(attrib.data->type == cgltf_type_vec4);
            raw.joints.reserve(attribsize);
            
            glm::uvec4 joints;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_uint( attrib.data, index, glm::value_ptr(joints), 4);
              raw.joints.push_back( joints );
              //LOG_MESSAGE("* joint (", index, ") :", joints.x, joints.y, joints.z, joints.w);
            }
          }
          // Weights.
          else if (attrib.type == cgltf_attribute_type_weights) {
            LOG_DEBUG( "> loading joint weights." );
            LOG_CHECK(attrib.data->type == cgltf_type_vec4);
            raw.weights.reserve(attribsize);

            glm::vec4 weights;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_float( attrib.data, index, glm::value_ptr(weights), 4);
              raw.weights.push_back( weights );
              // LOG_MESSAGE("* weights (", index, ") :", weights.x, weights.y, weights.z, weights.w);
            }
          }
        }
        
        // Indices.
        if (prim.indices) {
          if (prim.indices->is_sparse) {
            LOG_WARNING( "GLTF sparse indexing is not implemented." );
          } else {
            for (cgltf_size index = 0; index < prim.indices->count; ++index) {
              auto const vid = cgltf_accessor_read_index(prim.indices, index);
              raw.elementsAttribs.push_back(glm::ivec3( last_vertex_index + vid ));
            }
          }

          // Vertex Group.
          if (auto mat = prim.material; mat) {
            VertexGroup vg;

            vg.name = material_names[mat];
            vg.start_index = raw.elementsAttribs.size() - prim.indices->count; // 
            vg.end_index   = raw.elementsAttribs.size(); //
            raw.vgroups.push_back(vg);
          }
        } else {
          LOG_WARNING( "GLTF : No indices are associated with file", filename );
        }

        last_vertex_index = raw.vertices.size(); //
      }

      // Skeleton datas.
      if (auto *skin = node.skin; skin) {
        auto const njoints = skin->joints_count;

        meshdata.skeleton = std::make_shared<Skeleton>( njoints );
        auto skl = meshdata.skeleton;

        // Map to find (parent) nodes index.
        std::unordered_map< cgltf_node*, int32_t> joint_indices( njoints );
        for (cgltf_size index = 0; index < njoints; ++index) {
          auto *joint = skin->joints[index];
          joint_indices[ joint ] = index;
        }
        
        // Fill skeleton data.
        for (cgltf_size index = 0; index < njoints; ++index) {
          auto *joint = skin->joints[index];

          // Joint name.
          sprintf(tmpname, "%s::joint_%02d", basename.c_str(), int(index)); //
          auto const joint_name = (joint->name) ? joint->name : tmpname;

          // Joint parent index.
          auto const& it = joint_indices.find( joint->parent );
          auto const parent_index = (it != joint_indices.end()) ? it->second : -1;

          skl->names.push_back( joint_name );
          skl->parents.push_back( parent_index );
          skl->index_map[joint_name] = static_cast<int32_t>(index);
        }

        // Inverse the global transform.
        auto const inverse_world_matrix{ glm::inverse(world_matrix) };
        
        // Retrieve the global inverse bind matrices.
        auto &matrices = skl->inverse_bind_matrices; 
        matrices.resize(njoints); 
        cgltf_accessor_unpack_floats( skin->inverse_bind_matrices, glm::value_ptr(*matrices.data()), 16 * njoints );

        // Transform them to global space.
        skl->transform_inverse_bind_matrices(inverse_world_matrix);
      }
    }

    // -- MATERIALS.
    {
      auto &mtl = meshdata.material;
      std::string const fn( filename );
      std::string const dirname = fn.substr(0, fn.find_last_of('/'));

      for (cgltf_size i = 0; i < data->materials_count; ++i) {
        auto const& mat = data->materials[i];
        
        MaterialInfo info;
        info.name = material_names[ &mat ];

        // PBR Metal Roughness.
        if (mat.has_pbr_metallic_roughness) {
          auto const& pmr = mat.pbr_metallic_roughness;

          auto const& rgba = pmr.base_color_factor;
          info.diffuse_color      = glm::vec4(rgba[0], rgba[1], rgba[2], rgba[3]);        
          info.metallic           = pmr.metallic_factor;
          info.roughness          = pmr.roughness_factor;

          info.diffuse_map        = SetupTextureGLTF( pmr.base_color_texture.texture,         dirname, info.name + "_diffuse");
          info.metallic_rough_map = SetupTextureGLTF( pmr.metallic_roughness_texture.texture, dirname, info.name + "_metallic_rough");
        }

        // Alpha Test / Blend.
        switch (mat.alpha_mode) {
          case cgltf_alpha_mode_blend:
            info.bBlending = true;
          break;
          case cgltf_alpha_mode_mask:
            info.bAlphaTest = true;
          break;
          case cgltf_alpha_mode_opaque:
          default:
          break;
        }

        // Miscs.
        info.bump_map     = SetupTextureGLTF( mat.normal_texture.texture,    dirname, info.name + "_normal");
        info.ao_map       = SetupTextureGLTF( mat.occlusion_texture.texture, dirname, info.name + "_occlusion");
        info.emissive_map = SetupTextureGLTF( mat.emissive_texture.texture,  dirname, info.name + "_emissive");
        info.alpha_cutoff = mat.alpha_cutoff;
        info.bDoubleSided = mat.double_sided;
        info.bUnlit       = mat.unlit;

        auto const emf = mat.emissive_factor;
        info.emissive_factor = glm::vec3( emf[0], emf[1], emf[2]);

        if (!info.bump_map.empty()) {
          bNeedTangents = true; //
        }

        mtl.infos.push_back(info);
      }

      // Rename materials and vertex group ids.
      meshfile.material_id = fn; 
      mtl.id = fn.substr(fn.find_last_of('/') + 1);
      mtl.id = mtl.id.substr(0, mtl.id.find_last_of('.'));
      meshfile.prefix_material_vg_names(mtl);
    }

    // -- ANIMATION DATAS.
    if (data->animations_count) {
      LoadAnimationGLTF( basename, data, meshdata );
    }
  }

  /* ------------ */

  cgltf_free(data);

  return meshdata.setup( meshfile, bNeedTangents ); //
}

// ----------------------------------------------------------------------------
