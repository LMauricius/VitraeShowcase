#include "assetCollection.hpp"

#include "converters/aiPBSConvert.hpp"
#include "converters/aiPhongConvert.hpp"
#include "generators/meshNormals.hpp"
#include "generators/meshTangents.hpp"
#include "generators/modelSilhouette.hpp"
#include "methods/displayCamera.hpp"
#include "methods/displayNormals.hpp"
#include "methods/effectNormalMaps.hpp"
#include "methods/renderForward.hpp"
#include "methods/renderSilhouette.hpp"
#include "methods/shadingPBS.hpp"
#include "methods/shadingPhong.hpp"
#include "methods/shadingTransform.hpp"
#include "methods/shadowBiLin.hpp"
#include "methods/shadowCommon.hpp"
#include "methods/shadowPCF.hpp"
#include "methods/shadowRough.hpp"

#include "Vitrae/Data/LevelOfDetail.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Constant.hpp"
#include "Vitrae/Pipelines/Shading/Snippet.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Assets/Compositor.hpp"
#include "Vitrae/Assets/Scene.hpp"

#include "dynasma/keepers/naive.hpp"
#include "dynasma/standalone.hpp"

#include "MMeter.h"

#include "glm/gtx/vector_angle.hpp"

AssetCollection::AssetCollection(ComponentRoot &root, Renderer &rend,
                                 std::filesystem::path scenePath, float sceneScale)
    : root(root), rend(rend), running(true), shouldReloadPipelines(true),
      comp(root)
{
    /*
    Shading setup
    */
    VitraeCommon::setupRenderForward(root);
    VitraeCommon::setupRenderSilhouette(root);
    VitraeCommon::setupDisplayCamera(root);
    VitraeCommon::setupSshadingTransform(root);
    VitraeCommon::setupShadingPhong(root);
    VitraeCommon::setupShadingPBS(root);
    VitraeCommon::setupShadowCommon(root);
    VitraeCommon::setupShadowRough(root);
    VitraeCommon::setupShadowBiLin(root);
    VitraeCommon::setupShadowPCF(root);
    VitraeCommon::setupEffectNormalMaps(root);
    VitraeCommon::setupDisplayNormals(root);

    VitraeCommon::setupAssimpPhongConvert(root);
    VitraeCommon::setupAssimpPBSConvert(root);

    VitraeCommon::setupMeshTangentGenerator(root);
    VitraeCommon::setupMeshNormalGenerator(root);
    VitraeCommon::setupModelSilhouetteGenerator(root);

    /*
    Setup window
    */
    running = true;

    p_windowFrame =
        root.getComponent<FrameStoreManager>()
            .register_asset(FrameStoreSeed{FrameStore::WindowDisplayParams{
                .root = root,
                .width = 800,
                .height = 600,
                .title = "Vitrae Showcase",
                .isFullscreen = false,
                .onClose = [&]() { running = false; },
                .onDrag =
                    [&](glm::vec2 motion, bool bLeft, bool bRight, bool bMiddle) {
                        // Camera rotation
                        if (bRight) {
                            glm::vec3 dirVec =
                                p_scene->camera.rotation * glm::vec3{0.0f, 0.0f, 1.0f};
                            float yaw = glm::orientedAngle(
                                glm::normalize(glm::vec2{dirVec.x, dirVec.z}), {0.0, 1.0});
                            float pitch = glm::orientedAngle(
                                glm::normalize(
                                    glm::vec2{glm::sqrt(dirVec.x * dirVec.x + dirVec.z * dirVec.z),
                                              dirVec.y}),
                                {1.0, 0.0});
                            yaw += 0.15f * glm::radians(motion.x);
                            pitch += 0.15f * glm::radians(motion.y);
                            p_scene->camera.rotation = glm::quat(glm::vec3(pitch, yaw, 0.0f));
                        }

                        // Camera movement
                        if (bLeft) {
                            p_scene->camera.move(p_scene->camera.rotation *
                                                 (0.02f * glm::vec3{-motion.x, 0.0, -motion.y}));
                        }

                        // Scene scaling (in case of wrong scale)
                        if (bMiddle) {
                            p_scene->camera.move(p_scene->camera.rotation *
                                                 (0.02f * glm::vec3{-motion.x, motion.y, 0.0}));
                        }
                    }}})
            .getLoaded();

    /*
    Setup assets
    */

    p_scene =
        dynasma::makeStandalone<Scene>(Scene::FileLoadParams{.root = root, .filepath = scenePath});
    p_scene->camera.position = glm::vec3(-15.0, 10.0, 1.3);
    p_scene->camera.scaling = glm::vec3(1, 1, 1);
    p_scene->camera.zNear = 0.05f;
    p_scene->camera.zFar = 1000.0f;
    p_scene->camera.rotation = glm::quatLookAt(glm::vec3(0.8, -0.5, 0), glm::vec3(0, 1, 0));
    for (auto &prop : p_scene->modelProps)
    {
        prop.transform.position = prop.transform.position * sceneScale;
        prop.transform.scale(glm::vec3(sceneScale));
    }

    /*
    Compositor
    */
    comp.parameters.set("scene", p_scene);
    comp.parameters.set("fs_display", p_windowFrame);
    comp.parameters.set("vsync", false);
    comp.parameters.set(StandardParam::LoDParams.name, LoDSelectionParams{
                                                           .method = LoDSelectionMethod::FirstBelowThreshold,
                                                           .threshold = {.minElementSize = 2},
                                                       });
}

AssetCollection::~AssetCollection() {}

void AssetCollection::render()
{
    comp.compose();

    if (shouldReloadPipelines)
    {
        shouldReloadPipelines = false;
        root.cleanMemoryPools(std::numeric_limits<std::size_t>::max()); // free all possible memory
    }
}
