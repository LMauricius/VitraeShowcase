#pragma once

#include <iostream>

#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Assets/Compositor.hpp"

#include <filesystem>
#include <mutex>

using namespace Vitrae;

struct AssetCollection
{
    std::mutex accessMutex;

    bool running;
    bool shouldReloadPipelines;

    ComponentRoot &root;
    Renderer &rend;

    dynasma::FirmPtr<FrameStore> p_windowFrame;
    dynasma::FirmPtr<Scene> p_scene;
    Compositor comp;

    AssetCollection(ComponentRoot &root, Renderer &rend, std::filesystem::path scenePath,
                    float sceneScale);
    ~AssetCollection();

    void render();
};