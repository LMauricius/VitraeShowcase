#include <QtWidgets/QApplication>
#include <iostream>
#include <thread>

#include "ProfilerWindow.h"
#include "SettingsWindow.h"
#include "Status.hpp"
#include "assetCollection.hpp"

#include "Vitrae/Renderers/OpenGL.hpp"

#include "MMeter.h"

using namespace Vitrae;

int main(int argc, char **argv)
{
    std::cout << "Hello, world!\n";

    String path = argv[1];
    float sceneScale = 1.0f;
    if (argc > 2) {
        sceneScale = std::stof(argv[2]);
    }

    /*
    Setup the system
    */

    ComponentRoot root;
    Renderer *p_rend = new OpenGLRenderer();
    root.setComponent<Renderer>(p_rend);
    {
        p_rend->mainThreadSetup(root);

        /*
        Assets
        */
        AssetCollection collection(root, *p_rend, path, sceneScale);
        Status status;

        /*
        GUI setup
        */
        QApplication app(argc, argv);
        SettingsWindow settingsWindow(collection, status);
        settingsWindow.show();
        ProfilerWindow profilerWindow(collection, status);
        profilerWindow.show();

        /*
        Render loop!
        */
        p_rend->anyThreadDisable();
        std::thread renderThread([&]() {
            p_rend->anyThreadEnable();
            while (collection.running) {
                {
                    std::unique_lock lock1(collection.accessMutex);

                    auto startTime = std::chrono::high_resolution_clock::now();
                    {
                        MMETER_SCOPE_PROFILER("Render iteration");

                        collection.render();
                    }
                    auto endTime = std::chrono::high_resolution_clock::now();

                    status.update(endTime - startTime);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
            p_rend->anyThreadDisable();
        });

        /*
        GUI loop!
        */
        while (collection.running) {
            app.processEvents();
            p_rend->mainThreadUpdate();
            settingsWindow.updateValues();
            profilerWindow.updateValues();
        }

        renderThread.join();
        p_rend->anyThreadEnable();

        /*
        Free resources
        */
        p_rend->mainThreadFree();
    }

    return 0;
}
