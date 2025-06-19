#include "engine.h"
#include "random.h"
#include "Updatable.h"
#include "audio/source.h"
#include "io/keybinding.h"
#include "soundManager.h"
#include "windowManager.h"
#include "multiplayer_server.h"
#include "ecs/entity.h"
#include "systems/collision.h"

#include <thread>
#include <SDL.h>

#ifdef STEAMSDK
#include "steam/steam_api.h"
#include "steam/steam_api_flat.h"
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <mach-o/dyld.h>
#include <libgen.h>
#endif

#ifdef DEBUG
#include <typeinfo>
int DEBUG_PobjCount;
PObject* DEBUG_PobjListStart;
#endif

Engine* engine;

Engine::Engine()
{
    engine = this;

#ifdef __APPLE__
    // TODO: Find a proper solution.
    // Seems to be non-NULL even outside of a proper bundle.
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (bundle)
    {
        char bundle_path[PATH_MAX], exe_path[PATH_MAX];

        CFURLRef bundleURL = CFBundleCopyBundleURL(bundle);
        CFURLGetFileSystemRepresentation(bundleURL, true, (unsigned char*)bundle_path, PATH_MAX);
        CFRelease(bundleURL);

        uint32_t size = sizeof(exe_path);
        if (_NSGetExecutablePath(exe_path, &size) != 0)
        {
          fprintf(stderr, "Failed to get executable path.\n");
        }

        char *exe_realpath = realpath(exe_path, NULL);
        char *exe_dir      = dirname(exe_realpath);

        if (strcmp(exe_dir, bundle_path))
        {
          char resources_path[PATH_MAX];

          CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(bundle);
          CFURLGetFileSystemRepresentation(resourcesURL, true, (unsigned char*)resources_path, PATH_MAX);
          CFRelease(resourcesURL);

          chdir(resources_path);
        }
        else
        {
          chdir(exe_dir);
        }

        free(exe_realpath);
        free(exe_dir);
    }
#endif

#ifdef STEAMSDK
    if (SteamAPI_RestartAppIfNecessary(1907040))
        exit(1);
    if (!SteamAPI_Init())
    {
        LOG(Error, "Failed to initialize steam API.");
        exit(1);
    }
    SteamNetworkingUtils()->InitRelayNetworkAccess();
    LOG(Debug, "SteamID:", SteamAPI_ISteamUser_GetSteamID(SteamAPI_SteamUser()));
#endif

#ifdef WIN32
    // Setup crash reporter (Dr. MinGW) if available.
    exchndl = DynamicLibrary::open("exchndl.dll");

    if (exchndl)
    {
        auto pfnExcHndlInit = exchndl->getFunction<void(*)(void)>("ExcHndlInit");

        if (pfnExcHndlInit)
        {
            pfnExcHndlInit();
            LOG(INFO) << "Crash Reporter ON";
        }
        else
        {
            exchndl.reset();
        }
    } 
#endif // WIN32

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1"); // Have clicking on a window to get focus generate mouse events. For multimonitor support.
#ifdef SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH
    SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#elif defined(SDL_HINT_MOUSE_TOUCH_EVENTS)
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#endif
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_ShowCursor(false);
    SDL_StopTextInput();

    atexit(SDL_Quit);

    initRandom();
    gameSpeed = 1.0f;
    running = true;
    elapsedTime = 0.0f;
    soundManager = new SoundManager();
}

Engine::~Engine()
{
    Window::all_windows.clear();
    updatableList.clear();
    delete soundManager;
    soundManager = nullptr;
}

void Engine::registerObject(string name, P<PObject> obj)
{
    objectMap[name] = obj;
}

P<PObject> Engine::getObject(string name)
{
    if (!objectMap[name])
        return NULL;
    return objectMap[name];
}

void Engine::runMainLoop()
{
    if (Window::all_windows.size() == 0)
    {
        sp::SystemStopwatch frame_timer;
#ifdef DEBUG
        sp::SystemTimer debug_output_timer;
        debug_output_timer.repeat(5);
#endif

        while(running)
        {
            // Handle SDL_QUIT event
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    running = false;
                }
            }
#ifdef DEBUG
            if (debug_output_timer.isExpired())
                LOG(DEBUG) << "Object count: " << DEBUG_PobjCount << " " << updatableList.size();
#endif

            auto realtime_delta = frame_timer.restart();
            auto update_delta = realtime_delta;
            if (update_delta > 0.5f)
                update_delta = 0.5f;
            if (update_delta < 0.001f)
                update_delta = 0.001f;
            update_delta *= gameSpeed;

            foreach(Updatable, u, updatableList)
                u->update(update_delta);
            for(auto system : systems)
                system->update(update_delta);
            sp::CollisionSystem::update(update_delta);
            elapsedTime += update_delta;
            soundManager->updateTick();
#ifdef STEAMSDK
            SteamAPI_RunCallbacks();
#endif
            std::this_thread::sleep_for(std::chrono::duration<float>(1.f/60.f - realtime_delta));
        }
    }else{
        sp::audio::Source::startAudioSystem();
        sp::SystemStopwatch frame_timer;
#ifdef DEBUG
        sp::SystemTimer debug_output_timer;
        debug_output_timer.repeat(5);
#endif
        while(running)
        {
            // Handle events
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                handleEvent(event);
            }

#ifdef DEBUG
            if (debug_output_timer.isExpired())
                LOG(DEBUG) << "Object count: " << DEBUG_PobjCount << " " << updatableList.size();
#endif

            float delta = frame_timer.restart();
            if (delta > 0.5f)
                delta = 0.5f;
            if (delta < 0.001f)
                delta = 0.001f;
            delta *= gameSpeed;
            EngineTiming engine_timing;
            
            sp::SystemStopwatch engine_timing_stopwatch;
            foreach(Updatable, u, updatableList) {
                auto name = string(typeid(**u).name());
                u->update(delta);
                engine_timing["update:" + name] = engine_timing_stopwatch.restart();
            }
            for(auto system : systems) {
                system->update(delta);
                engine_timing[typeid(*system).name()] = engine_timing_stopwatch.restart();
            }
            elapsedTime += delta;
            sp::CollisionSystem::update(delta);
            engine_timing["collision"] = engine_timing_stopwatch.restart();
            soundManager->updateTick();
#ifdef STEAMSDK
            SteamAPI_RunCallbacks();
#endif

            // Clear the window
            for(auto window : Window::all_windows)
                window->render();
            engine_timing["rendering"] = engine_timing_stopwatch.restart();
            engine_timing["server_update"] = 0.0f;
            if (game_server)
                engine_timing["server_update"] = game_server->getUpdateTime();
            
            last_engine_timing = engine_timing;

            sp::io::Keybinding::allPostUpdate();
        }
        soundManager->stopMusic();
        sp::audio::Source::stopAudioSystem();
    }
}

void Engine::handleEvent(SDL_Event& event)
{
    if (event.type == SDL_QUIT)
        running = false;
#ifdef DEBUG
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
        running = false;
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_l && !SDL_IsTextInputActive())
    {
        int n = 0;
        printf("------------------------\n");
        std::unordered_map<string,int> totals;
        for(PObject* obj = DEBUG_PobjListStart; obj; obj = obj->DEBUG_PobjListNext)
        {
            printf("%c%4d: %4d: %s\n", obj->isDestroyed() ? '>' : ' ', n++, obj->getRefCount(), typeid(*obj).name());
            if (!obj->isDestroyed())
            {
                totals[typeid(*obj).name()]=totals[typeid(*obj).name()]+1;
            }
        }
        printf("--non-destroyed totals--\n");
        int grand_total=0;
        for (auto entry : totals)
        {
            printf("%4d %s\n", entry.second, entry.first.c_str());
            grand_total+=entry.second;
        }
        printf("%4d %s\n",grand_total,"All PObjects");
        printf("------------------------\n");

        sp::ecs::Entity::dumpDebugInfo();
        sp::ecs::ComponentStorageBase::dumpDebugInfo();
    }
#endif

    unsigned int window_id = 0;
    switch(event.type)
    {
    case SDL_KEYDOWN:
#ifdef __EMSCRIPTEN__
        if (!audio_started)
        {
            audio::AudioSource::startAudioSystem();
            audio_started = true;
        }
#endif
    case SDL_KEYUP:
        window_id = event.key.windowID;
        break;
    case SDL_MOUSEMOTION:
        window_id = event.motion.windowID;
        break;
    case SDL_MOUSEBUTTONDOWN:
#ifdef __EMSCRIPTEN__
        if (!audio_started)
        {
            audio::AudioSource::startAudioSystem();
            audio_started = true;
        }
#endif
    case SDL_MOUSEBUTTONUP:
        window_id = event.button.windowID;
        break;
    case SDL_MOUSEWHEEL:
        window_id = event.wheel.windowID;
        break;
    case SDL_WINDOWEVENT:
        window_id = event.window.windowID;
        break;
    case SDL_FINGERDOWN:
    case SDL_FINGERUP:
    case SDL_FINGERMOTION:
#if SDL_VERSION_ATLEAST(2, 0, 12)
        window_id = event.tfinger.windowID;
#else
        window_id = SDL_GetWindowID(SDL_GetMouseFocus());
#endif
        break;
    case SDL_TEXTEDITING:
        window_id = event.edit.windowID;
        break;
    case SDL_TEXTINPUT:
        window_id = event.text.windowID;
        break;
    }
    if (window_id != 0)
    {
        foreach(Window, window, Window::all_windows)
            if (window->window && SDL_GetWindowID(static_cast<SDL_Window*>(window->window)) == window_id)
                window->handleEvent(event);
    }
    sp::io::Keybinding::handleEvent(event);
}

void Engine::setGameSpeed(float speed)
{
    gameSpeed = speed;
}

float Engine::getGameSpeed()
{
    return gameSpeed;
}

float Engine::getElapsedTime()
{
    return elapsedTime;
}

Engine::EngineTiming Engine::getEngineTiming()
{
    return last_engine_timing;
}

void Engine::shutdown()
{
    running = false;
}
