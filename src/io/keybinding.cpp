#include <io/keybinding.h>
#include <logging.h>
#include <SDL_assert.h>
#include <engine.h>
#include <json11/json11.hpp>
#include <fstream>
#include <unordered_set>
#include <SDL_events.h>


namespace sp {
namespace io {

Keybinding* Keybinding::keybindings = nullptr;
Keybinding* Keybinding::rebinding_key = nullptr;
Keybinding::Type Keybinding::rebinding_type;

Keybinding::Keybinding(const string& name)
: name(name), label(name.substr(0, 1).upper() + name.substr(1).lower())
{
    value = 0.0;
    down_event = false;
    up_event = false;

    for(auto other = keybindings; other; other = other->next)
        SDL_assert(other->name != name);//"Duplicate keybinding name"

    auto ptr = &keybindings;
    while(*ptr)
        ptr = &((*ptr)->next);
    *ptr = this;
}

Keybinding::Keybinding(const string& name, const string& default_key)
: Keybinding(name)
{
    addKey(default_key);
}

Keybinding::Keybinding(const string& name, const std::initializer_list<const string>& default_keys)
: Keybinding(name)
{
    for(const string& key : default_keys)
        addKey(key);
}

Keybinding::~Keybinding()
{
    for(auto ptr = &keybindings; *ptr; ptr = &((*ptr)->next))
    {
        if (*ptr == this) {
            *ptr = next;
            break;
        }
    }
}

void Keybinding::setKey(const string& key)
{
    bindings.clear();
    addKey(key);
}

void Keybinding::setKeys(const std::initializer_list<const string>& keys)
{
    bindings.clear();
    for(const string& key : keys)
        addKey(key);
}

void Keybinding::addKey(const string& key, bool inverted)
{
    if (key.startswith("-"))
    {
        return addKey(key.substr(1), !inverted);
    }
    //Format for joystick keys:
    //joy:[joystick_id]:axis:[axis_id]
    //joy:[joystick_id]:button:[button_id]
    if (key.startswith("joy:"))
    {
        std::vector<string> parts = key.split(":");
        if (parts.size() == 4)
        {
            int joystick_id = parts[1].toInt();
            int axis_button_id = parts[3].toInt();
            if (parts[2] == "axis")
                addBinding(int(axis_button_id) | int(joystick_id) << 8 | joystick_axis_mask, inverted);
            else if (parts[2] == "button")
                addBinding(int(axis_button_id) | int(joystick_id) << 8 | joystick_button_mask, inverted);
            else
                LOG(Warning, "Unknown joystick binding:", key);
        }
        return;
    }
    //Format for gamecontroller keys:
    //gamecontroller:[joystick_id]:axis:[axis_name]
    //gamecontroller:[joystick_id]:button:[button_name]
    if (key.startswith("gamecontroller:"))
    {
        std::vector<string> parts = key.split(":");
        if (parts.size() == 4)
        {
            int controller_id = parts[1].toInt();
            if (parts[2] == "axis")
            {
                int axis = SDL_GameControllerGetAxisFromString(parts[3].c_str());
                if (axis < 0)
                {
                    LOG(Warning, "Unknown axis in game controller binding:", key);
                    return;
                }
                addBinding(axis | int(controller_id) << 8 | game_controller_axis_mask, inverted);
            }
            else if (parts[2] == "button")
            {
                int button = SDL_GameControllerGetButtonFromString(parts[3].c_str());
                if (button < 0)
                {
                    LOG(Warning, "Unknown button in game controller binding:", key);
                    return;
                }
                addBinding(button | int(controller_id) << 8 | game_controller_button_mask, inverted);
            }
            else
            {
                LOG(Warning, "Unknown game controller binding:", key);
            }
        }
        return;
    }
    if (key.startswith("pointer:"))
    {
        addBinding(pointer_mask | key.substr(8).toInt(), inverted);
        return;
    }
    if (key.startswith("mouse:"))
    {
        if (key == "mouse:x") addBinding(mouse_movement_mask | 0, inverted);
        else if (key == "mouse:y") addBinding(mouse_movement_mask | 1, inverted);
        else LOG(Warning, "Unknown mouse movement binding:", key);
        return;
    }
    if (key.startswith("wheel:"))
    {
        if (key == "wheel:x") addBinding(mouse_wheel_mask | 0, inverted);
        else if (key == "wheel:y") addBinding(mouse_wheel_mask | 1, inverted);
        else LOG(Warning, "Unknown mouse wheel binding:", key);
        return;
    }
    if (key.startswith("virtual:"))
    {
        int index = key.substr(8).toInt();
        addBinding(virtual_mask | index, inverted);
        return;
    }

    SDL_Keycode code = SDL_GetKeyFromName(key.c_str());
    if (code != SDLK_UNKNOWN)
        addBinding(code | keyboard_mask, inverted);
    else
        LOG(Warning, "Unknown key binding:", key);
}

void Keybinding::removeKey(int index)
{
    if (index < 0 || index >= int(bindings.size()))
        return;
    bindings.erase(bindings.begin() + index);
}

void Keybinding::clearKeys()
{
    bindings.clear();
}

bool Keybinding::isBound() const
{
    return bindings.size() > 0;
}

string Keybinding::getKey(int index) const
{
    if (index >= 0 && index < int(bindings.size()))
    {
        int key = bindings[index].key;
        switch(key & type_mask)
        {
        case keyboard_mask:
            return SDL_GetKeyName(key & ~type_mask);
        case pointer_mask:
            return "pointer:" + string(key & ~type_mask);
        case joystick_axis_mask:
            return "joy:" + string((key >> 8) & 0xff) + ":axis:" + string(key & 0xff);
        case joystick_button_mask:
            return "joy:" + string((key >> 8) & 0xff) + ":button:" + string(key & 0xff);
        case mouse_movement_mask:
            switch(key & ~type_mask)
            {
            case 0: return "mouse:x";
            case 1: return "mouse:y";
            }
            break;
        case mouse_wheel_mask:
            switch(key & ~type_mask)
            {
            case 0: return "wheel:x";
            case 1: return "wheel:y";
            }
            break;
        case game_controller_button_mask:
            return "gamecontroller:" + string((key >> 8) & 0xff) + ":button:" + string(SDL_GameControllerGetStringForButton(SDL_GameControllerButton(key & 0xff)));
        case game_controller_axis_mask:
            return "gamecontroller:" + string((key >> 8) & 0xff) + ":axis:" + string(SDL_GameControllerGetStringForAxis(SDL_GameControllerAxis(key & 0xff)));
        case virtual_mask:
            return "virtual:" + string(key & 0xff);
        }
        return "unknown";
    }
    return "";
}

Keybinding::Type Keybinding::getKeyType(int index) const
{
    if (index < 0 || index >= int(bindings.size()))
        return Type::None;
    return static_cast<Type>((bindings[index].key & type_mask) >> 16);
}

string Keybinding::getHumanReadableKeyName(int index) const
{
    if (index >= 0 && index < int(bindings.size()))
    {
        int key = bindings[index].key;
        int data = key & ~type_mask;
        switch(key & type_mask)
        {
        case keyboard_mask:
            return SDL_GetKeyName(data);
        case pointer_mask:
            switch(Pointer::Button(data))
            {
            case Pointer::Button::Touch: return "Touch Screen";
            case Pointer::Button::Left: return "Left Button";
            case Pointer::Button::Middle: return "Middle Button";
            case Pointer::Button::Right: return "Right Button";
            case Pointer::Button::X1: return "X1 Button";
            case Pointer::Button::X2: return "X2 Button";
            default: break;
            }
            break;
        case joystick_axis_mask:
            return "Axis: " + string(data & 0xff);
        case joystick_button_mask:
            return "Button: " + string(data & 0xff);
        case mouse_movement_mask:
            switch(data)
            {
            case 0: return "X";
            case 1: return "Y";
            }
            break;
        case mouse_wheel_mask:
            switch(data)
            {
            case 0: return "Wheel Sideways";
            case 1: return "Wheel";
            }
            break;
        case game_controller_button_mask:
            switch(data & 0xff)
            {
            case SDL_CONTROLLER_BUTTON_A: return "A";
            case SDL_CONTROLLER_BUTTON_B: return "B";
            case SDL_CONTROLLER_BUTTON_X: return "X";
            case SDL_CONTROLLER_BUTTON_Y: return "Y";
            case SDL_CONTROLLER_BUTTON_BACK: return "Back";
            case SDL_CONTROLLER_BUTTON_GUIDE: return "Guide";
            case SDL_CONTROLLER_BUTTON_START: return "Start";
            case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "LeftStick";
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "RightStick";
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "LeftShoulder";
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "RightShoulder";
            case SDL_CONTROLLER_BUTTON_DPAD_UP: return "Up";
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "Down";
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "Left";
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "Right";
            }
            break;
        case game_controller_axis_mask:
            switch(data & 0xff)
            {
            case SDL_CONTROLLER_AXIS_LEFTX: return "X Axis";
            case SDL_CONTROLLER_AXIS_LEFTY: return "Y Axis";
            case SDL_CONTROLLER_AXIS_RIGHTX: return "X Axis Right";
            case SDL_CONTROLLER_AXIS_RIGHTY: return "Y Axis Right";
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return "Trigger Axis Left";
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return "Trigger Axis Right";
            }
            break;
        case virtual_mask:
            return "Virtual-" + string(data & 0xff);
        }
        return "Unknown";
    }
    return "";
}

bool Keybinding::get() const
{
    return value > 0.5f || down_event;
}

bool Keybinding::getDown() const
{
    return down_event;
}

bool Keybinding::getUp() const
{
    return !down_event && up_event;
}

float Keybinding::getValue() const
{
    return value;
}

void Keybinding::startUserRebind(Type bind_type)
{
    rebinding_key = this;
    rebinding_type = bind_type;
}

bool Keybinding::isUserRebinding() const
{
    return rebinding_key == this;
}

int Keybinding::joystickCount()
{
    return SDL_NumJoysticks() - gamepadCount();
}

int Keybinding::gamepadCount()
{
    int count = 0;
    for(int n=0; n<SDL_NumJoysticks(); n++)
        if (SDL_IsGameController(n))
            count += 1;
    return count;
}

void Keybinding::loadKeybindings(const string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        return;
    std::stringstream data;
    data << file.rdbuf();
    string err;

    json11::Json json = json11::Json::parse(data.str(), err);
    if (err != "")
    {
        LOG(Warning, "Failed to load keybindings from", filename, ":", err);
        return;
    }

    for(Keybinding* keybinding = keybindings; keybinding; keybinding=keybinding->next)
    {
        const json11::Json& entry = json[keybinding->name];
        if (!entry.is_object())
            continue;
        if (entry["key"].is_string())
        {
            keybinding->setKey(entry["key"].string_value());
        }
        else if (entry["key"].is_array())
        {
            keybinding->clearKeys();
            for(auto key_entry : entry["key"].array_items())
            {
                if (key_entry.is_string())
                    keybinding->addKey(key_entry.string_value());
            }
        }
        else
        {
            keybinding->bindings.clear();
        }
    }
}

void Keybinding::saveKeybindings(const string& filename)
{
    json11::Json::object obj;
    for(Keybinding* keybinding = keybindings; keybinding; keybinding=keybinding->next)
    {
        json11::Json::object data;
        json11::Json::array keys;
        for(unsigned int index=0; index<keybinding->bindings.size(); index++)
            keys.push_back(keybinding->getKey(index).c_str());
        data["key"] = keys;
        obj[keybinding->name] = data;
    }
    json11::Json json = obj;

    std::ofstream file(filename);
    file << json.dump();
}

Keybinding* Keybinding::getByName(const string& name)
{
    for(Keybinding* keybinding = keybindings; keybinding; keybinding=keybinding->next)
        if (keybinding->name == name)
            return keybinding;
    return nullptr;
}

std::vector<string> Keybinding::getCategories()
{
    std::vector<string> result;
    std::unordered_set<string> found;
    for(Keybinding* keybinding = keybindings; keybinding; keybinding=keybinding->next)
    {
        if (found.find(keybinding->getCategory()) != found.end())
            continue;
        found.insert(keybinding->getCategory());
        result.push_back(keybinding->getCategory());
    }
    return result;
}

std::vector<Keybinding*> Keybinding::listAllByCategory(const string& category)
{
    std::vector<Keybinding*> result;
    for(Keybinding* keybinding = keybindings; keybinding; keybinding=keybinding->next)
    {
        if (keybinding->getCategory() == category)
            result.push_back(keybinding);
    }
    return result;
}

void Keybinding::setVirtualKey(int index, float value)
{
    SDL_assert(index >= 0 && index <= 255);//"Virtual key indexes need to be in the range 0-255"
    
    updateKeys(virtual_mask | index, value);
}

void Keybinding::addBinding(int key, bool inverted)
{
    for(auto& bind : bindings)
    {
        if (bind.key == key)
        {
            bind.inverted = inverted;
            return;
        }
    }
    bindings.push_back({key, inverted});
}

void Keybinding::setValue(float value)
{
    if (value < 0.05f && value > -0.05f)//Add a tiny dead zone by default. Assists in gamepads that give off "almost zero" in neutral.
        value = 0.0;
    if (this->value < 0.5f && value >= 0.5f)
        down_event = true;
    if (this->value >= 0.5f && value < 0.5f)
        up_event = true;
    this->value = value;
}

void Keybinding::postUpdate()
{
    if (down_event)
        down_event = false;
    else if (up_event)
        up_event = false;
}

static int release_mouse = 0;

void Keybinding::allPostUpdate()
{
    for(Keybinding* keybinding = keybindings; keybinding; keybinding=keybinding->next)
        keybinding->postUpdate();
    
    if (release_mouse & (1 << 0))
        updateKeys(0 | mouse_wheel_mask, 0.0);
    if (release_mouse & (1 << 1))
        updateKeys(1 | mouse_wheel_mask, 0.0);

    if (release_mouse & (1 << 2))
        updateKeys(0 | mouse_movement_mask, 0.0);
    if (release_mouse & (1 << 3))
        updateKeys(1 | mouse_movement_mask, 0.0);
    release_mouse = 0;
}

void Keybinding::handleEvent(const SDL_Event& event)
{
    switch(event.type)
    {
    case SDL_KEYDOWN:
        updateKeys(event.key.keysym.sym | keyboard_mask, 1.0);
        break;
    case SDL_KEYUP:
        updateKeys(event.key.keysym.sym | keyboard_mask, 0.0);
        break;
    case SDL_MOUSEBUTTONDOWN:
        {
            io::Pointer::Button button = io::Pointer::Button::Unknown;
            switch(event.button.button)
            {
            case SDL_BUTTON_LEFT: button = io::Pointer::Button::Left; break;
            case SDL_BUTTON_MIDDLE: button = io::Pointer::Button::Middle; break;
            case SDL_BUTTON_RIGHT: button = io::Pointer::Button::Right; break;
            case SDL_BUTTON_X1: button = io::Pointer::Button::X1; break;
            case SDL_BUTTON_X2: button = io::Pointer::Button::X2; break;
            default: break;
            }
            if (button != io::Pointer::Button::Unknown)
                updateKeys(int(button) | pointer_mask, 1.0);
        }
        break;
    case SDL_MOUSEBUTTONUP:
        {
            io::Pointer::Button button = io::Pointer::Button::Unknown;
            switch(event.button.button)
            {
            case SDL_BUTTON_LEFT: button = io::Pointer::Button::Left; break;
            case SDL_BUTTON_MIDDLE: button = io::Pointer::Button::Middle; break;
            case SDL_BUTTON_RIGHT: button = io::Pointer::Button::Right; break;
            case SDL_BUTTON_X1: button = io::Pointer::Button::X1; break;
            case SDL_BUTTON_X2: button = io::Pointer::Button::X2; break;
            default: break;
            }
            if (button != io::Pointer::Button::Unknown)
                updateKeys(int(button) | pointer_mask, 0.0);
        }
        break;
    case SDL_MOUSEMOTION:{
        int w, h;
        SDL_GetWindowSize(SDL_GetWindowFromID(event.motion.windowID), &w, &h);
        if (event.motion.xrel != 0)
        {
            updateKeys(0 | mouse_movement_mask, float(event.motion.xrel) / float(w) * 500.0f);
            release_mouse |= 1 << 2;
        }
        if (event.motion.yrel != 0)
        {
            updateKeys(1 | mouse_movement_mask, float(event.motion.yrel / float(h)) * 500.0f);
            release_mouse |= 1 << 3;
        }
        }break;
    case SDL_MOUSEWHEEL:
        if (event.wheel.x > 0)
        {
            updateKeys(0 | mouse_wheel_mask, 1.0);
            release_mouse |= 1 << 0;
        }
        if (event.wheel.x < 0)
        {
            updateKeys(0 | mouse_wheel_mask, -1.0);
            release_mouse |= 1 << 0;
        }
        if (event.wheel.y > 0)
        {
            updateKeys(1 | mouse_wheel_mask, 1.0);
            release_mouse |= 1 << 1;
        }
        if (event.wheel.y < 0)
        {
            updateKeys(1 | mouse_wheel_mask, -1.0);
            release_mouse |= 1 << 1;
        }
        break;
    case SDL_FINGERDOWN:
        //event.tfinger.x, event.tfinger.x
        updateKeys(int(io::Pointer::Button::Touch) | pointer_mask, 1.0);
        break;
    case SDL_FINGERUP:
        //event.tfinger.x, event.tfinger.x
        updateKeys(int(io::Pointer::Button::Touch) | pointer_mask, 0.0);
        break;
    case SDL_JOYBUTTONDOWN:
        updateKeys(int(event.jbutton.button) | int(event.jbutton.which) << 8 | joystick_button_mask, 1.0);
        break;
    case SDL_JOYBUTTONUP:
        updateKeys(int(event.jbutton.button) | int(event.jbutton.which) << 8 | joystick_button_mask, 0.0);
        break;
    case SDL_JOYAXISMOTION:
        updateKeys(int(event.jaxis.axis) | int(event.jaxis.which) << 8 | joystick_axis_mask, float(event.jaxis.value) / 32768.0f);
        break;
    case SDL_JOYDEVICEADDED:
        if (!SDL_IsGameController(event.jdevice.which))
        {
            SDL_Joystick* joystick = SDL_JoystickOpen(event.jdevice.which);
            if (joystick)
                LOG(Info, "Found joystick:", SDL_JoystickName(joystick));
            else
                LOG(Warning, "Failed to open joystick...");
        }
        break;
    case SDL_JOYDEVICEREMOVED:
        for(int button=0; button<32; button++)
            updateKeys(int(button) | int(event.jdevice.which) << 8 | joystick_button_mask, 0.0);
        for(int axis=0; axis<32; axis++)
        {
            updateKeys(int(axis) | int(event.jdevice.which) << 8 | joystick_axis_mask, 0.0);
        }
        SDL_JoystickClose(SDL_JoystickFromInstanceID(event.jdevice.which));
        break;
    case SDL_CONTROLLERAXISMOTION:
        updateKeys(int(event.caxis.axis) | int(event.caxis.which) << 8 | game_controller_axis_mask, float(event.caxis.value) / 32768.0f);
        break;
    case SDL_CONTROLLERBUTTONDOWN:
        updateKeys(int(event.cbutton.button) | int(event.cbutton.which) << 8 | game_controller_button_mask, 1.0);
        break;
    case SDL_CONTROLLERBUTTONUP:
        updateKeys(int(event.cbutton.button) | int(event.cbutton.which) << 8 | game_controller_button_mask, 0.0);
        break;
    case SDL_CONTROLLERDEVICEADDED:
        {
            SDL_GameController* gc = SDL_GameControllerOpen(event.cdevice.which);
            if (gc)
                LOG(Info, "Found game controller:", SDL_GameControllerName(gc));
            else
                LOG(Warning, "Failed to open game controller...");
        }
        break;
    case SDL_CONTROLLERDEVICEREMOVED:
        for(int button=0; button<SDL_CONTROLLER_BUTTON_MAX; button++)
            updateKeys(int(button) | int(event.cdevice.which) << 8 | game_controller_button_mask, 0.0);
        for(int axis=0; axis<SDL_CONTROLLER_AXIS_MAX; axis++)
        {
            updateKeys(int(axis) | int(event.cdevice.which) << 8 | game_controller_axis_mask, 0.0);
        }
        SDL_GameControllerClose(SDL_GameControllerFromInstanceID(event.cdevice.which));
        break;
    case SDL_CONTROLLERDEVICEREMAPPED:
        break;
    case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        {
            //Focus lost, release all keys.
            for(Keybinding* keybinding = keybindings; keybinding; keybinding=keybinding->next)
                if (keybinding->bindings.size() > 0)
                    keybinding->setValue(0.0);
        }
        break;
    default:
        break;
    }
}

void Keybinding::updateKeys(int key_number, float value)
{
    if (rebinding_key)
    {
        if ((value > 0.5f || value < -0.5f) && (key_number & (static_cast<int>(rebinding_type) << 16)))
        {
            rebinding_key->addBinding(key_number, value < 0.0f);
            rebinding_key = nullptr;
        }
    }

    for(Keybinding* keybinding = keybindings; keybinding; keybinding=keybinding->next)
    {
        for(const auto& bind : keybinding->bindings)
        {
            if (bind.key == key_number)
            {
                if (bind.inverted)
                    keybinding->setValue(-value);
                else
                    keybinding->setValue(value);
            }
        }
    }
}

bool operator&(const Keybinding::Type a, const Keybinding::Type b)
{
    return static_cast<int>(a) & static_cast<int>(b);
}

Keybinding::Type operator|(const Keybinding::Type a, const Keybinding::Type b)
{
    return static_cast<Keybinding::Type>(static_cast<int>(a) | static_cast<int>(b));
}

}//namespace io
}//namespace sp
