#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/Params/Purposes.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Snippet.hpp"

#include "dynasma/standalone.hpp"

#include "glm/gtx/norm.hpp"

namespace VitraePluginShadowFiltering
{
using namespace Vitrae;

inline void setupRenderEdgeSilhouette(ComponentRoot &root)
{
    MethodCollection &methodCollection = root.getComponent<MethodCollection>();

    methodCollection.registerShaderTask(
        root.getComponent<ShaderSnippetKeeper>().new_asset_k<ShaderSnippet::StringParams>({
            .inputSpecs =
                {
                    {"position_view", TYPE_INFO<glm::vec4>},
                    {"viewport_size", TYPE_INFO<glm::uvec2>},
                    {"denormal4view", TYPE_INFO<glm::vec3>},
                    {"eaa_edge_scale", TYPE_INFO<float>, 0.5f},
                },
            .outputSpecs = {{"position_halfnormal_offset4view", TYPE_INFO<glm::vec4>}},
            .snippet = R"glsl(
                vec3 normvertex4view = normalize(denormal4view.xyz);
                position_halfnormal_offset4view = vec4(
                    position_view.xy + normalize(normvertex4view.xy) / vec2(viewport_size) * position_view.w*eaa_edge_scale,
                    position_view.z + normvertex4view.z * position_view.w * 0.005,
                    position_view.w
                );
            )glsl",
        }),
        ShaderStageFlag::Vertex);

    methodCollection.registerComposeTask(
        root.getComponent<ComposeSceneRenderKeeper>().new_asset_k<ComposeSceneRender::SetupParams>({
            .root = root,
            .inputTokenNames = {"scene_silhouette_rendered"},
            .outputTokenNames = {"scene_edge_silhouette_rendered"},

            .rasterizing =
                {
                    .vertexPositionOutputPropertyName = "position_halfnormal_offset4view",
                    .modelFormPurpose = Purposes::silhouetting,
                    .cullingMode = CullingMode::Frontface,
                    .rasterizingMode = RasterizingMode::DerivationalTraceEdges,
                },
            .ordering =
                {
                    .generateFilterAndSort = [](const Scene &scene, const RenderComposeContext &ctx)
                        -> std::pair<ComposeSceneRender::FilterFunc, ComposeSceneRender::SortFunc> {
                        return {
                            [](const ModelProp &prop) { return true; },
                            [](const ModelProp &l, const ModelProp &r) { return &l < &r; },
                        };
                    },
                },
        }));

    methodCollection.registerPropertyOption("scene_rendered", "scene_edge_silhouette_rendered");
}
} // namespace VitraePluginShadowFiltering