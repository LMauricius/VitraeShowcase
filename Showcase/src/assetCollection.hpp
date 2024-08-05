#pragma once

#include <iostream>

#include "Methods/classic.hpp"
#include "shadingModes.hpp"

#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Visuals/Compositor.hpp"

#include <filesystem>
#include <mutex>

using namespace Vitrae;

struct AssetCollection
{
    std::mutex accessMutex;

    bool running;

    ComponentRoot &root;
    Renderer &rend;

    ShadingModeSetter modeSetter;
    MethodsClassic methodsClassic;

    dynasma::FirmPtr<FrameStore> p_windowFrame;
    dynasma::FirmPtr<Scene> p_scene;
    Compositor comp;

    AssetCollection(ComponentRoot &root, Renderer &rend, std::filesystem::path scenePath,
                    float sceneScale);
    ~AssetCollection();

    void render();
};