#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraePluginShadowFiltering
{
    inline void setupShadowPCF(Vitrae::ComponentRoot &root)
    {
        using namespace Vitrae;
    
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        methodCollection.registerShaderTask(
            root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        {"tex_shadow", TYPE_INFO<dynasma::FirmPtr<Texture>>},
                        {"position_shadow", TYPE_INFO<glm::vec3>},
                    },
                .outputSpecs =
                    {
                        {"light_shadow_factor_PCF", TYPE_INFO<float>},
                    },
                .snippet = R"glsl(
                    const int maskSize = 2;
                    const float blurRadius = 0.8;

                    vec2 shadowSize = textureSize(tex_shadow, 0);

                    vec2 stride = blurRadius / float(maskSize) / shadowSize;

                    int counter = 0;
                    light_shadow_factor_PCF = 0;
                    for (int i = -maskSize; i <= maskSize; ++i) {
                        int shift = int((i % 2) == 0);
                        for (int j = -maskSize+shift; j <= maskSize-shift; j += 2) {
                            if (texture(tex_shadow, position_shadow.xy + vec2(ivec2(i, j)) * stride).r < position_shadow.z + 0.5/shadowSize.x) {
                                light_shadow_factor_PCF += 0.0;
                            }
                            else {
                                light_shadow_factor_PCF += 1.0;
                            }
                            ++counter;
                        }
                    }
                    light_shadow_factor_PCF /= float(counter);
                )glsl",
            }}),
            ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerPropertyOption("light_shadow_factor", "light_shadow_factor_PCF");
    }
}