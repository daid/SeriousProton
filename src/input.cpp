#include "input.h"
#include "engine.h"

P<Window> InputHandler::window;
bool InputHandler::touch_screen = false;
PVector<InputEventHandler> InputHandler::input_event_handlers;
PVector<JoystickEventHandler> InputHandler::joystick_event_handlers;

//#warning TODO SDL2 this no longer works, SDLK_Keycode will be out of range. Port SP2 keybindings.
bool InputHandler::keyboard_button_down[256];
bool InputHandler::keyboard_button_pressed[256];
bool InputHandler::keyboard_button_released[256];
SDL_KeyboardEvent InputHandler::last_key_press;

float InputHandler::joystick_axis_pos[4][4];
float InputHandler::joystick_axis_changed[4][4];
bool InputHandler::joystick_button_down[4][4];
bool InputHandler::joystick_button_changed[4][4];

InputEventHandler::InputEventHandler()
{
    InputHandler::input_event_handlers.push_back(this);
}

InputEventHandler::~InputEventHandler()
{
}

JoystickEventHandler::JoystickEventHandler()
{
    InputHandler::joystick_event_handlers.push_back(this);
}

JoystickEventHandler::~JoystickEventHandler()
{
}

void InputHandler::initialize()
{
    memset(keyboard_button_down, 0, sizeof(keyboard_button_down));
    memset(joystick_axis_pos, 0, sizeof(joystick_axis_pos));
#ifdef __ANDROID__
    touch_screen = true;
#endif
    last_key_press.keysym.sym = SDLK_UNKNOWN;
}

void InputHandler::preEventsUpdate()
{
    if (!window)
        window = engine->getObject("window");

    for(unsigned int n=0; n<256; n++)
    {
        if (keyboard_button_pressed[n])
            keyboard_button_pressed[n] = false;
        else
            keyboard_button_released[n] = false;
    }
    for(unsigned int i=0; i<4; i++)
    {
        for(unsigned int n=0; n<4; n++)
        {
            joystick_axis_changed[i][n] = false;
        }
        for(unsigned int n=0; n<4; n++)
        {
            joystick_button_changed[i][n] = false;
        }
    }
}

void InputHandler::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.sym > -1 && event.key.keysym.sym < 256)
        {
            keyboard_button_down[event.key.keysym.sym] = true;
            keyboard_button_pressed[event.key.keysym.sym] = true;
        }
        last_key_press = event.key;
    }
	else if (event.type == SDL_KEYUP)
	{
        if (event.key.keysym.sym > -1 && event.key.keysym.sym < 256)
        {
            keyboard_button_down[event.key.keysym.sym] = false;
            keyboard_button_released[event.key.keysym.sym] = true;
        }
	}
    else if (event.type == SDL_TEXTINPUT && event.text.text[0] > 31)
    {
        if (last_key_press.keysym.sym != SDLK_UNKNOWN)
        {
            fireKeyEvent(last_key_press, event.text.text[0]);
            last_key_press.keysym.sym = SDLK_UNKNOWN;
        }
    }
    /*
    else if (event.type == SDL_JOYAXISMOTION)
    {
        static constexpr float scale_factor = 100.f / (100.f - joystick_axis_snap_to_0_range);

        float axis_pos = 0.f;
        
        if (event.jaxis.value > joystick_axis_snap_to_0_range) {
            axis_pos = (event.jaxis.value - joystick_axis_snap_to_0_range) * scale_factor;
        } else if (event.jaxis.value < -joystick_axis_snap_to_0_range) {
            axis_pos = (event.jaxis.value + joystick_axis_snap_to_0_range) * scale_factor;
        }

        // Clamp axis_pos within SFML range.
        axis_pos = std::min(std::max(-100.f, axis_pos), 100.f);

        if (joystick_axis_pos[event.jaxis.which][event.jaxis.axis] != axis_pos){
            joystick_axis_changed[event.jaxis.which][event.jaxis.axis] = true;
        }
        joystick_axis_pos[event.joystickMove.joystickId][event.joystickMove.axis] = axis_pos;
    }
    else if (event.type == sf::Event::JoystickButtonPressed)
    {
        joystick_button_down[event.joystickMove.joystickId][event.joystickButton.button] = true;
        joystick_button_changed[event.joystickMove.joystickId][event.joystickButton.button] = true;
    }
    else if (event.type == sf::Event::JoystickButtonReleased)
    {
        joystick_button_down[event.joystickMove.joystickId][event.joystickButton.button] = false;
        joystick_button_changed[event.joystickMove.joystickId][event.joystickButton.button] = true;
    }
    else if (event.type == sf::Event::LostFocus)
    {
        for(unsigned int n=0; n<sf::Keyboard::KeyCount; n++)
        {
            keyboard_button_down[n] = false;
        }
    }
    */
}

void InputHandler::postEventsUpdate()
{
    input_event_handlers.update();
    joystick_event_handlers.update();

    if (last_key_press.keysym.sym != SDLK_UNKNOWN)
    {
        InputHandler::fireKeyEvent(last_key_press, -1);
        last_key_press.keysym.sym = SDLK_UNKNOWN;
    }

    for(unsigned int i=0; i<4; i++)
    {
        for(unsigned int n=0; n<4; n++)
        {
            if(joystick_axis_changed[i][n])
            {
                foreach(JoystickEventHandler, e, joystick_event_handlers)
                {
                    e->handleJoystickAxis(i, n, joystick_axis_pos[i][n]);
                }
            }
        }
        for(unsigned int n=0; n<4; n++)
        {
            if(joystick_button_changed[i][n])
            {
                foreach(JoystickEventHandler, e, joystick_event_handlers)
                {
                    e->handleJoystickButton(i, n, joystick_button_down[i][n]);
                }
            }
        }
    }
}

void InputHandler::fireKeyEvent(const SDL_KeyboardEvent& key, int unicode)
{
    foreach(InputEventHandler, e, input_event_handlers)
    {
        e->handleKeyPress(key, unicode);
    }
}
