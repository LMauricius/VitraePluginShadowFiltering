#include "VitraePluginShadowFiltering/Setup.hpp"

#include "VitraePluginShadowFiltering/tasks/renderEdgeSilhouette.hpp"
#include "VitraePluginShadowFiltering/tasks/shadowBiLin.hpp"
#include "VitraePluginShadowFiltering/tasks/shadowEAA.hpp"
#include "VitraePluginShadowFiltering/tasks/shadowPCF.hpp"
#include "VitraePluginShadowFiltering/tasks/shadowRough.hpp"

namespace VitraePluginShadowFiltering
{

void setup(Vitrae::ComponentRoot &root)
{
    setupShadowRough(root);
    setupShadowBiLin(root);
    setupShadowPCF(root);
    setupShadowEAA(root);
    setupRenderEdgeSilhouette(root);
}

} // namespace VitraePluginShadowFiltering