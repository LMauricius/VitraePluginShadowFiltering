#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"

#include "dynasma/standalone.hpp"

namespace VitraePluginShadowFiltering
{
inline void setupShadowEAA(Vitrae::ComponentRoot &root)
{
    using namespace Vitrae;

    MethodCollection &methodCollection = root.getComponent<MethodCollection>();

    // Composition
    methodCollection.registerComposeTask(
        dynasma::makeStandalone<ComposeFrameToTexture>(ComposeFrameToTexture::SetupParams{
            .root = root,
            .inputTokenNames = {"scene_edge_silhouette_rendered"},
            .outputColorTextureName = "",
            .outputDepthTextureName = "tex_eaa_shadow_adapted",
            .outputs = {{
                .textureName = "tex_eaa_alias_adapted",
                .fragmentSpec = {"alias_data", TYPE_INFO<glm::vec3>},
            }},
            .size{String("ShadowMapSize"), {1024, 1024}},
            .channelType = Texture::ChannelType::VEC3_SNORM8,
            .horWrap = Texture::WrappingType::BORDER_COLOR,
            .verWrap = Texture::WrappingType::BORDER_COLOR,
            .minFilter = Texture::FilterType::NEAREST,
            .magFilter = Texture::FilterType::NEAREST,
            .useMipMaps = false,
            .borderColor = {0.0f, 0.0f, 0.0f, 0.0f},
        }));

    methodCollection.registerComposeTask(
        dynasma::makeStandalone<ComposeAdaptTasks>(ComposeAdaptTasks::SetupParams{
            .root = root,
            .adaptorAliases =
                {
                    {"fs_target", "fs_shadow"},
                    {"position_view", "position_shadow_view"},
                    {"tex_eaa_shadow", "tex_eaa_shadow_adapted"},
                    {"tex_eaa_alias", "tex_eaa_alias_adapted"},
                },
            .desiredOutputs =
                {
                    {"tex_eaa_shadow", TYPE_INFO<dynasma::FirmPtr<Texture>>},
                    {"tex_eaa_alias", TYPE_INFO<dynasma::FirmPtr<Texture>>},
                },
            .friendlyName = "Render shadows & edge data",
        }));

    // Shading
    methodCollection.registerShaderTask(
        root.getComponent<ShaderSnippetKeeper>().new_asset_k<ShaderSnippet::StringParams>({
            .inputSpecs =
                {
                    {"position_shadow", TYPE_INFO<glm::vec3>},
                    {"ShadowMapSize", TYPE_INFO<glm::uvec2>},
                },
            .outputSpecs = {{"texelpos4shadow", TYPE_INFO<glm::vec2>}},
            .snippet = R"glsl(
                texelpos4shadow = position_shadow.xy * vec2(ShadowMapSize);
            )glsl",
        }),
        ShaderStageFlag::Vertex);

    methodCollection.registerShaderTask(
        root.getComponent<ShaderSnippetKeeper>().new_asset_k<ShaderSnippet::StringParams>({
            .inputSpecs =
                {
                    {"normal4view", TYPE_INFO<glm::vec3>},
                    {"texelpos4shadow", TYPE_INFO<glm::vec2>},
                },
            .outputSpecs = {{"alias_data", TYPE_INFO<glm::vec3>}},
            .snippet = R"glsl(
                vec2 normal4view2D = normalize(normal4view.xy);
                vec2 alias = texelpos4shadow - gl_FragCoord.xy;
                float alias_shift = dot(normal4view2D, alias);
                alias_data = vec3(normal4view2D, alias_shift);
            )glsl",
        }),
        ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

    methodCollection.registerShaderTask(
        root.getComponent<ShaderSnippetKeeper>().new_asset_k<ShaderSnippet::StringParams>({
            .inputSpecs =
                {
                    {"tex_eaa_shadow", TYPE_INFO<dynasma::FirmPtr<Texture>>},
                    {"tex_eaa_alias", TYPE_INFO<dynasma::FirmPtr<Texture>>},
                    {"position_shadow", TYPE_INFO<glm::vec3>},
                    {"texelpos4shadow", TYPE_INFO<glm::vec2>},
                    {"ShadowMapSize", TYPE_INFO<glm::uvec2>},
                },
            .outputSpecs =
                {
                    {"light_shadow_factor_EAA", TYPE_INFO<float>},
                },
            .snippet = R"glsl(
                float offset = 0.5 / ShadowMapSize.x;

                vec2 rounded_texelpos4shadow = round(texelpos4shadow);
                ivec2 texelpos4shadowI = ivec2(rounded_texelpos4shadow);

                vec3 shadow_alias_data = texelFetch(tex_eaa_alias, texelpos4shadowI, 0).rgb;
                vec2 shadow_normal4view2D = shadow_alias_data.xy;
                float shadow_alias_shift = shadow_alias_data.z;

                vec2 subtexel_offset = texelpos4shadow - rounded_texelpos4shadow;
                float subtexel_shift = dot(shadow_normal4view2D, subtexel_offset);

                vec2 adjusted_texelpos4shadow = texelpos4shadow;
                if (shadow_alias_shift != 0.0) {
                    if (subtexel_shift < shadow_alias_shift) {
                        adjusted_texelpos4shadow -= shadow_normal4view2D*0.25;
                    } else {
                        adjusted_texelpos4shadow += shadow_normal4view2D;
                    }
                }
                ivec2 final_texelpos4shadowI = ivec2(round(adjusted_texelpos4shadow));

                bool inShadow =
                    (texelFetch(tex_eaa_shadow, final_texelpos4shadowI, 0).r < position_shadow.z + offset);
                light_shadow_factor_EAA = inShadow? 0.0 : 1.0;
            )glsl",
        }),
        ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

    methodCollection.registerPropertyOption("light_shadow_factor", "light_shadow_factor_EAA");
}
} // namespace VitraePluginShadowFiltering