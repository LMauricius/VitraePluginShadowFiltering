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
    inline void setupShadowRough(Vitrae::ComponentRoot &root)
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
                        {"light_shadow_factor_rough", TYPE_INFO<float>},
                    },
                .snippet = R"glsl(
                    vec2 shadowSize = textureSize(tex_shadow, 0);
                    ivec2 texelPosI = ivec2(round(position_shadow.xy * shadowSize));
                    float offset = 0.5 / shadowSize.x;
                    if (texelFetch(tex_shadow, texelPosI, 0).r < position_shadow.z + offset) {
                        light_shadow_factor_rough = 0.0;
                    }
                    else {
                        light_shadow_factor_rough = 1.0;
                    }
                )glsl"}}),
            ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerPropertyOption("light_shadow_factor", "light_shadow_factor_rough");
    }
}