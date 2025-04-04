#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/Params/Purposes.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"

#include "dynasma/standalone.hpp"

#include "glm/gtx/norm.hpp"

namespace VitraePluginShadowFiltering
{
using namespace Vitrae;

inline void setupRenderEdgeSilhouette(ComponentRoot &root)
{
    MethodCollection &methodCollection = root.getComponent<MethodCollection>();

    methodCollection.registerComposeTask(
        root.getComponent<ComposeSceneRenderKeeper>().new_asset_k<ComposeSceneRender::SetupParams>({
            .root = root,
            .inputTokenNames = {"frame_cleared"},
            .outputTokenNames = {"scene_edge_silhouette_rendered"},

            .rasterizing =
                {
                    .vertexPositionOutputPropertyName = "position_view",
                    .modelFormPurpose = Purposes::silhouetting,
                    .cullingMode = CullingMode::Frontface,
                    .rasterizingMode = RasterizingMode::DerivationalFillEdges,
                },
            .ordering =
                {
                    .generateFilterAndSort = [](const Scene &scene, const RenderComposeContext &ctx)
                        -> std::pair<ComposeSceneRender::FilterFunc, ComposeSceneRender::SortFunc> {
                        glm::vec3 camPos = scene.camera.position;

                        return {
                            [](const ModelProp &prop) { return true; },
                            [camPos](const ModelProp &l, const ModelProp &r) {
                                // closest first
                                glm::vec3 lpos = l.transform.position;
                                glm::vec3 rpos = r.transform.position;
                                return glm::length2(camPos - rpos) < glm::length2(camPos - lpos);
                            },
                        };
                    },
                },
        }));

    methodCollection.registerPropertyOption("scene_rendered", "scene_edge_silhouette_rendered");
}
} // namespace VitraePluginShadowFiltering