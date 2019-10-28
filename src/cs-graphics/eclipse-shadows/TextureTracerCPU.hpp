////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CS_GRAPHICS_TEXTURE_TRACER_CPU_HPP
#define CS_GRAPHICS_TEXTURE_TRACER_CPU_HPP

#include "../../cs-core/Settings.hpp"
#include "TextureTracer.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cs::graphics {
class TextureTracerCPU : public TextureTracer {
 public:
  std::vector<FloatPixel> traceThroughTexture(uint32_t ssboPhotons, size_t numPhotons,
      core::Settings::BodyProperties const& bodyProperties) override;
};
} // namespace cs::graphics

#endif // CS_GRAPHICS_TEXTURE_TRACER_CPU_HPP
