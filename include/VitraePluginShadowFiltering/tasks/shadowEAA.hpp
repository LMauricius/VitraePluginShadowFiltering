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
    methodCollection
        .registerComposeTask(
            dynasma::makeStandalone<ComposeFrameToTexture>(
                ComposeFrameToTexture::SetupParams{
                    .root = root,
                    .inputTokenNames = {"scene_edge_silhouette_rendered"},
                    .outputs =
                        {
                            ComposeFrameToTexture::OutputTextureParamSpec{
                                .textureName = "tex_eaa_shadow_adapted",
                                .shaderComponent = FixedRenderComponent::Depth,
                                .format = BufferFormat::DEPTH_STANDARD,
                                .clearColor = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
                                .filtering =
                                    {
                                        .horWrap = WrappingType::BORDER_COLOR,
                                        .verWrap = WrappingType::BORDER_COLOR,
                                        .minFilter = FilterType::NEAREST,
                                        .magFilter = FilterType::NEAREST,
                                        .useMipMaps = false,
                                        .borderColor = {1.0f, 1.0f, 1.0f, 1.0f},
                                    },
                            },
                            ComposeFrameToTexture::OutputTextureParamSpec{
                                .textureName = "tex_eaa_ext_shadow_adapted",
                                .shaderComponent = ParamSpec{"extended_depth", TYPE_INFO<float>},
                                .format = BufferFormat::SCALAR_FLOAT32,
                                .clearColor = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
                                .filtering =
                                    {
                                        .horWrap = WrappingType::BORDER_COLOR,
                                        .verWrap = WrappingType::BORDER_COLOR,
                                        .minFilter = FilterType::NEAREST,
                                        .magFilter = FilterType::NEAREST,
                                        .useMipMaps = false,
                                        .borderColor = {1.0f, 1.0f, 1.0f, 1.0f},
                                    },
                            },
                            ComposeFrameToTexture::OutputTextureParamSpec{
                                .textureName = "tex_eaa_alias_adapted",
                                .shaderComponent = ParamSpec{"alias_data", TYPE_INFO<glm::vec3>},
                                .format = BufferFormat::VEC3_SNORM8,
                                .clearColor = glm::vec4{0.0f, 0.0f, 0.0f, 0.0f},
                                .filtering =
                                    {
                                        .horWrap = WrappingType::BORDER_COLOR,
                                        .verWrap = WrappingType::BORDER_COLOR,
                                        .minFilter = FilterType::NEAREST,
                                        .magFilter = FilterType::NEAREST,
                                        .useMipMaps = false,
                                        .borderColor = {0.0f, 0.0f, 0.0f, 0.0f},
                                    },
                            },
                        },
                    .size{String("ShadowMapSize"), {1024, 1024}},
                }));

    methodCollection.registerComposeTask(
        dynasma::makeStandalone<ComposeAdaptTasks>(ComposeAdaptTasks::SetupParams{
            .root = root,
            .adaptorAliases =
                {
                    {"fs_target", "fs_shadow"},
                    {"position_view", "position_shadow_view"},
                    {"viewport_size", "ShadowMapSize"},
                    {"mat_view", "mat_shadow_view"},
                    {"mat_proj", "mat_shadow_proj"},
                    {"tex_eaa_shadow", "tex_eaa_shadow_adapted"},
                    {"tex_eaa_alias", "tex_eaa_alias_adapted"},
                    {"tex_eaa_ext_shadow", "tex_eaa_ext_shadow_adapted"},
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
                    {"position_shadow", TYPE_INFO<glm::vec3>},
                    {"texelpos4shadow", TYPE_INFO<glm::vec2>},
                },
            .outputSpecs =
                {
                    {"alias_data", TYPE_INFO<glm::vec3>},
                    {"extended_depth", TYPE_INFO<float>},
                },
            .snippet = R"glsl(
                vec2 normal4view2D = normalize(normal4view.xy);
                vec2 alias = interpolateAtCentroid(texelpos4shadow) - gl_FragCoord.xy;
                float alias_shift = dot(normal4view2D, alias);
                alias_data = vec3(normal4view2D, alias_shift/2);
                extended_depth = position_shadow.z;
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

                vec2 floor_texelpos4shadow = floor(texelpos4shadow);
                vec2 inter_offset = texelpos4shadow - floor_texelpos4shadow;
                ivec2 floor_texelpos4shadowI = ivec2(floor_texelpos4shadow);

                vec2 inter_normal = vec2(0.0, 0.0);
                float inter_shadow_alias_shift = 0.0;
                float inter_subtexel_shift = 0.0;
                float inter_weight = 0.0;

                for (int xoff = 0; xoff <= 1; xoff++) {
                    for (int yoff = 0; yoff <= 1; yoff++) {
                        ivec2 off = ivec2(xoff, yoff);
                        ivec2 texelpos4shadowI = floor_texelpos4shadowI + off;

                        if (texelFetch(tex_eaa_shadow, texelpos4shadowI, 0).r < position_shadow.z - offset) {
                            vec3 shadow_alias_data = texelFetch(tex_eaa_alias, texelpos4shadowI, 0).rgb;
                            float shadow_alias_shift = shadow_alias_data.z*2;
                            
                            if (shadow_alias_shift != 0) {
                                vec2 shadow_normal4view2D = shadow_alias_data.xy;

                                vec2 subtexel_offset = texelpos4shadow - vec2(texelpos4shadowI);
                                float subtexel_shift = dot(shadow_normal4view2D, subtexel_offset);

                                float weight = (abs(float(1 - xoff) - inter_offset.x) * abs(float(1 - yoff) - inter_offset.y));
                                inter_normal += weight * shadow_normal4view2D;
                                inter_shadow_alias_shift += weight * shadow_alias_shift;
                                inter_subtexel_shift += weight * subtexel_shift;
                                inter_weight += weight;
                            }
                        }
                    }
                }

                vec2 adjusted_texelpos4shadow = texelpos4shadow;
                if (inter_weight > 0.0) {
                    if (inter_subtexel_shift < inter_shadow_alias_shift)
                    {
                        light_shadow_factor_EAA = 0.0;
                    }
                    else {
                        adjusted_texelpos4shadow += normalize(inter_normal)*1.5;
                        ivec2 final_texelpos4shadowI = ivec2(round(adjusted_texelpos4shadow));
                        bool inShadow = (texelFetch(tex_eaa_shadow, final_texelpos4shadowI, 0).r < position_shadow.z - offset);
                        light_shadow_factor_EAA = inShadow? 0.0 : 1.0;
                    }
                } else {
                    ivec2 final_texelpos4shadowI = ivec2(round(adjusted_texelpos4shadow));
                    bool inShadow = (texelFetch(tex_eaa_shadow, final_texelpos4shadowI, 0).r < position_shadow.z - offset);
                    light_shadow_factor_EAA = inShadow? 0.0 : 1.0;
                }
            )glsl",
        }),
        ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

    methodCollection.registerShaderTask(
        root.getComponent<ShaderSnippetKeeper>().new_asset_k<ShaderSnippet::StringParams>({
            .inputSpecs =
                {
                    {"tex_eaa_shadow", TYPE_INFO<dynasma::FirmPtr<Texture>>},
                    {"tex_eaa_ext_shadow", TYPE_INFO<dynasma::FirmPtr<Texture>>},
                    {"tex_eaa_alias", TYPE_INFO<dynasma::FirmPtr<Texture>>},
                    {"position_shadow", TYPE_INFO<glm::vec3>},
                    {"texelpos4shadow", TYPE_INFO<glm::vec2>},
                    {"ShadowMapSize", TYPE_INFO<glm::uvec2>},
                    {"eaa_fade_length", TYPE_INFO<float>, 1.0f},
                },
            .outputSpecs =
                {
                    {"light_shadow_factor_EAA_LF", TYPE_INFO<float>},
                },
            .snippet = R"glsl(
                float offset = 0.5 / ShadowMapSize.x;

                vec2 floor_texelpos4shadow = floor(texelpos4shadow);
                vec2 inter_offset = texelpos4shadow - floor_texelpos4shadow;
                ivec2 floor_texelpos4shadowI = ivec2(floor_texelpos4shadow);

                vec2 inter_normal = vec2(0.0, 0.0);
                float inter_shadow_alias_shift = 0.0;
                float inter_subtexel_shift = 0.0;
                float inter_weight = 0.0;

                for (int xoff = 0; xoff <= 1; xoff++) {
                    for (int yoff = 0; yoff <= 1; yoff++) {
                        ivec2 off = ivec2(xoff, yoff);
                        ivec2 texelpos4shadowI = floor_texelpos4shadowI + off;

                        if (texelFetch(tex_eaa_ext_shadow, texelpos4shadowI, 0).r < position_shadow.z - offset) {
                            vec3 shadow_alias_data = texelFetch(tex_eaa_alias, texelpos4shadowI, 0).rgb;
                            float shadow_alias_shift = shadow_alias_data.z*2;
                            
                            if (shadow_alias_shift != 0) {
                                vec2 shadow_normal4view2D = shadow_alias_data.xy;

                                vec2 subtexel_offset = texelpos4shadow - vec2(texelpos4shadowI);
                                float subtexel_shift = dot(shadow_normal4view2D, subtexel_offset);

                                float weight = (abs(float(1 - xoff) - inter_offset.x) * abs(float(1 - yoff) - inter_offset.y));
                                inter_normal += weight * shadow_normal4view2D;
                                inter_shadow_alias_shift += weight * shadow_alias_shift;
                                inter_subtexel_shift += weight * subtexel_shift;
                                inter_weight += weight;
                            }
                        }
                    }
                }

                vec2 adjusted_texelpos4shadow = texelpos4shadow;
                bool inShadow;
                if (inter_weight > 0.0) {
                    adjusted_texelpos4shadow += normalize(inter_normal)*1.5;
                    ivec2 final_texelpos4shadowI = ivec2(round(adjusted_texelpos4shadow));
                    bool inShadow = (texelFetch(tex_eaa_ext_shadow, final_texelpos4shadowI, 0).r < position_shadow.z - offset);
                    
                    light_shadow_factor_EAA_LF = inShadow? 0.0 : clamp(
                        ((inter_subtexel_shift - inter_shadow_alias_shift ) / eaa_fade_length / inter_weight + 0.5),
                        0.0, 1.0
                    );
                } else {
                    ivec2 final_texelpos4shadowI = ivec2(round(adjusted_texelpos4shadow));
                    bool inShadow = (texelFetch(tex_eaa_ext_shadow, final_texelpos4shadowI, 0).r < position_shadow.z - offset);
                    light_shadow_factor_EAA_LF = inShadow? 0.0 : 1.0;
                }
            )glsl",
        }),
        ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

    methodCollection.registerPropertyOption("light_shadow_factor", "light_shadow_factor_EAA");
    methodCollection.registerPropertyOption("light_shadow_factor", "light_shadow_factor_EAA_LF");
}
} // namespace VitraePluginShadowFiltering