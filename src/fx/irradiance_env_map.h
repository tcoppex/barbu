#ifndef BARBU_FX_IRRADIANCE_ENV_MAP_H_
#define BARBU_FX_IRRADIANCE_ENV_MAP_H_

#include <array>

#include "glm/gtc/constants.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

#include "memory/resources/resources.h" //

// ----------------------------------------------------------------------------

using IrradianceMatrices_t = std::array<glm::mat4, 3>;

/*
  Implement Ravi Ramamoorthi & Pat Hanrahan's technique as described in their 2001 paper :
    "An Efficient Representation for Irradiance Environment Maps" 
*/
class IrradianceEnvMap {
 public:
  template<typename T>
  using CubemapData_t = std::array<T*, 6>;

  static constexpr float kGammaCorrection = 2.2f; // [to externalize]
  
  /**
   * Computes Spherical Harmonics coefficients for standard unsigned byte 
   * environment map (cf. equation 10 from the paper).
   * 
   * Note : alternatively, retrieving the coefficients can be interesting
   *        (cf. equation 13).
   *
   *  Presuppose gammacorrected image data (uncorrected during the pass). 
   *
   */
  template<typename T>
  static
  void Prefilter(CubemapData_t<T> const& cubemap, float const dColor, int const w, int const h, int const nchannels, IrradianceMatrices_t &M)
  {
    float const texelSize  = 1.0f / static_cast<float>(w);

    SHCoeff_t shCoeff{0};

    float sumWeight = 0.0f;
    for (int texid = 0; texid < 6; ++texid) {
      auto const* pixels = cubemap[texid];

      for (int y = 0; y < h; ++y)
      {
        // map value to [-1, 1]
        float const v = 2.0f * ((y+0.5f) * texelSize) - 1.0f;
        
        for (int x = 0; x < w; ++x)
        {
          float const u = 2.0f * ((x+0.5f) * texelSize) - 1.0f;        
          
          glm::vec4 const attribs = GetTexelAttrib( texid, u, v, texelSize);
          
          sumWeight += attribs.w;
          glm::vec3 const direction = glm::vec3(attribs);
          float const solidAngle = attribs.w;

          for (int k = 0; k < kSHCoeffSize; ++k) {
            float const lambda = powf(pixels[k] * dColor, kGammaCorrection) * solidAngle;
            
            auto &shc = shCoeff[k];
            shc[0] += lambda * Y0( direction );
            shc[1] += lambda * Y1( direction );
            shc[2] += lambda * Y2( direction );
            shc[3] += lambda * Y3( direction );
            shc[4] += lambda * Y4( direction );
            shc[5] += lambda * Y5( direction );
            shc[6] += lambda * Y6( direction );
            shc[7] += lambda * Y7( direction );
            shc[8] += lambda * Y8( direction );
          }
          pixels += nchannels;
        }
      }
    }
    
    // Normalize the sh coefficients.
    float const dnorm = glm::two_pi<float>() / sumWeight;
    for (int i=0; i<kNumSHCoeff; ++i) {
      shCoeff[0][i] *= dnorm;
      shCoeff[1][i] *= dnorm;
      shCoeff[2][i] *= dnorm;
    }
    
    // Create the irradiance matrices from sh coefficients.
    SetIrradianceMatrices(shCoeff, M);
  }

  static
  void PrefilterU8( ResourceInfoList const& resource_infos, IrradianceMatrices_t &M) {
    assert( !resource_infos.empty() );

    CubemapData_t<uint8_t> cubemap{nullptr};
    int w(0), h(0), channels(0);
    int index = -1;

    for (auto res : resource_infos) {
      auto img = Resources::Get<Image>( res.id ).data;
      w = img->width;
      h = img->height;
      channels = img->channels;
      cubemap[++index] = (uint8_t*)img->pixels;
    }
    float constexpr dColor = 1.0f / static_cast<float>(std::numeric_limits<uint8_t>::max()-1);
    Prefilter<uint8_t>( cubemap, dColor, w, h, channels, M);
  }

  // [ nope ]
  // static
  // void PrefilterHDR( ResourceInfo const& resource, glm::mat4 M[3]) {
  //   CubemapData_t<float> cubemap{nullptr};

  //   auto img = Resources::Get<Image>( resource.id ).data;
  //   for (int i=0; i<img->depth; ++i) {
  //     cubemap[i] = ((float*)img->pixels) + img->width * img->height * img->channels;
  //   }

    
  //   int const size = img->width * img->height * img->depth * img->channels; 
  //   float maxf = -std::numeric_limits<float>::max();
  //   float minf = std::numeric_limits<float>::max();
    
  //   for (int i=0; i<size; i++) {
  //     maxf = std::max( ((float*)img->pixels)[i], maxf);
  //     minf = std::min( ((float*)img->pixels)[i], minf);
  //   }


  //   float dColor = 10.0f / maxf; //

  //   Prefilter<float>( cubemap, dColor, img->width, img->height, img->channels, M);
  // }

 private:
  static constexpr int32_t kNumSHCoeff  = 9;
  static constexpr int32_t kSHCoeffSize = 3;
  using SHCoeff_t = std::array<float[kNumSHCoeff], kSHCoeffSize>;

  constexpr static float Y0(glm::vec3 const& n)  { return 0.282095f; }                               /* L_00 */
  constexpr static float Y1(glm::vec3 const& n)  { return 0.488603f * n.y; }                         /* L_1-1 */
  constexpr static float Y2(glm::vec3 const& n)  { return 0.488603f * n.z; }                         /* L_10 */
  constexpr static float Y3(glm::vec3 const& n)  { return 0.488603f * n.x; }                         /* L_11 */
  constexpr static float Y4(glm::vec3 const& n)  { return 1.092548f * n.x*n.y; }                     /* L_2-2 */
  constexpr static float Y5(glm::vec3 const& n)  { return 1.092548f * n.y*n.z; }                     /* L_2-1 */
  constexpr static float Y6(glm::vec3 const& n)  { return 0.315392f * (3.0f*n.z*n.z - 1.0f); }       /* L_20 */
  constexpr static float Y7(glm::vec3 const& n)  { return 1.092548f * n.x*n.z; }                     /* L_21 */
  constexpr static float Y8(glm::vec3 const& n)  { return 0.546274f * (n.x*n.x - n.y*n.y); }         /* L_22 */

  /// Coefficients from the paper to test the implementation.
  constexpr static SHCoeff_t sTestCoeffs{
    .79, .39, -.34, -.29, -.11, -.26, -.16, .56, .21,
    .44, .35, -.18, -.06, -.05,-.22, -.09, .21, -.05,
    .54, .60, -.27, .01, -.12, -.47, -.15, .14,- .30
  };   

 
  static
  glm::vec4 GetTexelAttrib(int texid, float u, float v, float texelSize) {
    std::array<glm::vec3, 6> const texeldirs{
      glm::vec3( +1.0f, -v, -u),  // +X
      glm::vec3( -1.0f, -v, +u),  // -X
      glm::vec3( +u, +1.0f, +v),  // +Y
      glm::vec3( +u, -1.0f, -v),  // -Y
      glm::vec3( +u, -v, +1.0f),  // +Z
      glm::vec3( -u, -v, -1.0f),  // -Z
    };
    glm::vec3 const direction = glm::normalize( texeldirs[texid] );
          
    /// Compute the solid angle.      
    auto const area = [](float x, float y) { 
      return atan2f(x * y, sqrtf(x * x + y * y + 1.0f)); 
    };
    float const x0 = u - texelSize;
    float const y0 = v - texelSize;
    float const x1 = u + texelSize;
    float const y1 = v + texelSize;
    float const solidAngle = (area(x0,y0) + area(x1,y1)) 
                           - (area(x0,y1) + area(x1,y0))
                           ;
    return glm::vec4( direction, solidAngle);
  }


  static
  void SetIrradianceMatrices( SHCoeff_t const& shCoeff, IrradianceMatrices_t &M) {
    /** cf equation 12 */
    float constexpr c1 = 0.429043f;
    float constexpr c2 = 0.511664f;
    float constexpr c3 = 0.743125f;
    float constexpr c4 = 0.886227f;
    float constexpr c5 = 0.247708f;
    
    for (int i = 0; i < 3; ++i) {
      auto const& shc = shCoeff[i];
      auto& m = M[i];

      m[0][0] = c1 * shc[8];
      m[0][1] = c1 * shc[4];
      m[0][2] = c1 * shc[7];
      m[0][3] = c2 * shc[3];
      
      m[1][0] = c1 * shc[4];
      m[1][1] =-c1 * shc[8];
      m[1][2] = c1 * shc[5];
      m[1][3] = c2 * shc[1];
      
      m[2][0] = c1 * shc[7];
      m[2][1] = c1 * shc[5];
      m[2][2] = c3 * shc[6];
      m[2][3] = c2 * shc[2];
      
      m[3][0] = c2 * shc[3];
      m[3][1] = c2 * shc[1];
      m[3][2] = c2 * shc[2];
      m[3][3] = c4 * shc[0] - c5 * shc[6];
    }
  }

 private:
  IrradianceEnvMap(IrradianceEnvMap const&) = delete;
  IrradianceEnvMap(IrradianceEnvMap&&) = delete;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_IRRADIANCE_ENV_MAP_H_