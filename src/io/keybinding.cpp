#include <io/keybinding.h>
#include <i18n.h>
#include <logging.h>
#include <SDL_assert.h>
#include <engine.h>
#include <io/json.h>
#include <fstream>
#include <unordered_set>
#include <algorithm>
#include <SDL_events.h>
#include <SDL_timer.h>


namespace sp {
namespace io {

Keybinding* Keybinding::keybindings = nullptr;
Keybinding* Keybinding::rebinding_key = nullptr;
Keybinding::Type Keybinding::rebinding_type;
Keybinding::Interaction Keybinding::rebinding_interaction = Keybinding::Interaction::None;

float Keybinding::deadzone = 0.05f;
float Keybinding::discrete_step_size = 0.1f;
float Keybinding::threshold = 0.5f;
unsigned int Keybinding::repeat_delay = 500; // ms
unsigned int Keybinding::repeat_interval = 40; // ms
float Keybinding::sensitivity = 1.0f;

Keybinding::Keybinding(const string& name)
: name(name), label(name.substr(0, 1).upper() + name.substr(1).lower())
{
    value = 0.0f;
    down_event = false;
    up_event = false;

    for (auto other = keybindings; other; other = other->next)
        SDL_assert(other->name != name); // "Duplicate keybinding name"

    auto ptr = &keybindings;
    while (*ptr) ptr = &((*ptr)->next);
    *ptr = this;
}

Keybinding::Keybinding(const string& name, const string& default_key)
: Keybinding(name)
{
    default_bindings.emplace_back(default_key);
    addKey(default_key);
}

Keybinding::Keybinding(const string& name, const std::initializer_list<const string>& default_keys)
: Keybinding(name)
{
    for (const string& key : default_keys)
    {
        default_bindings.emplace_back(key);
        addKey(key);
    }
}

Keybinding::~Keybinding()
{
    for (auto ptr = &keybindings; *ptr; ptr = &((*ptr)->next))
    {
        if (*ptr == this)
        {
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
    for (const string& key : keys) addKey(key);
}

void Keybinding::addKey(const string& key, bool inverted)
{
    if (key.startswith("-") && key.length() > 1)
        return addKey(key.substr(1), !inverted);

    // Format for joystick keys:
    // joy:[joystick_id]:axis:[axis_id]
    // joy:[joystick_id]:button:[button_id]
    if (key.startswith("joy:"))
    {
        std::vector<string> parts = key.split(":");
        if (parts.size() == 4)
        {
            int joystick_id = parts[1].toInt();
            int axis_button_id = parts[3].toInt();

            if (parts[2] == "axis")
                addBinding(axis_button_id | joystick_id << 8 | joystick_axis_mask, inverted);
            else if (parts[2] == "button")
                addBinding(axis_button_id | joystick_id << 8 | joystick_button_mask, inverted);
            else LOG(Warning, "Unknown joystick binding:", key);
        }
        return;
    }

    // Format for gamecontroller keys:
    // gamecontroller:[joystick_id]:axis:[axis_name]
    // gamecontroller:[joystick_id]:button:[button_name]
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
            else LOG(Warning, "Unknown game controller binding:", key);
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
    if (code != SDLK_UNKNOWN) addBinding(code | keyboard_mask, inverted);
    else LOG(Warning, "Unknown key binding:", key);
}

void Keybinding::removeKey(int index)
{
    if (index < 0 || index >= int(bindings.size())) return;

    bindings.erase(bindings.begin() + index);
}

void Keybinding::clearKeys()
{
    bindings.clear();
}

void Keybinding::setDeadzone(float new_deadzone)
{
    deadzone = new_deadzone;
}

bool Keybinding::isBound() const
{
    return bindings.size() > 0;
}

string Keybinding::getKey(int index) const
{
    if (index >= 0 && index < int(bindings.size()))
    {
        if (bindings[index].inverted) return "-" + getKeyInternal(index);
        return getKeyInternal(index);
    }

    return "";
}

string Keybinding::getKeyInternal(int index) const
{
    if (index >= 0 && index < static_cast<int>(bindings.size()))
    {
        int key = bindings[index].key;
        switch (key & type_mask)
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
    if (index < 0 || index >= static_cast<int>(bindings.size()))
        return Type::None;
    return static_cast<Type>((bindings[index].key & type_mask) >> 16);
}

Keybinding::Interaction Keybinding::getInteraction(int index) const
{
    if (index < 0 || index >= static_cast<int>(bindings.size()))
        return Interaction::None;
    return bindings[index].interaction;
}

string Keybinding::getHumanReadableKeyName(int index) const
{
    if (index >= 0 && index < static_cast<int>(bindings.size()))
    {
        int key = bindings[index].key;
        int data = key & ~type_mask;
        string sign = bindings[index].inverted ? "-" : "+";
        switch(key & type_mask)
        {
        case keyboard_mask:
            return SDL_GetKeyName(data);
        case pointer_mask:
            switch(Pointer::Button(data))
            {
            case Pointer::Button::Touch: return tr("pointer_input", "Touchscreen");
            case Pointer::Button::Left: return tr("pointer_input", "Left button");
            case Pointer::Button::Middle: return tr("pointer_input", "Middle button");
            case Pointer::Button::Right: return tr("pointer_input", "Right button");
            case Pointer::Button::X1: return tr("pointer_input", "X1 button");
            case Pointer::Button::X2: return tr("pointer_input", "X2 button");
            default: break;
            }
            break;
        case joystick_axis_mask:
            return tr("joystick_input", "Axis {number_plus_minus}").format({
                {"number_plus_minus", string(data & 0xff) + sign}
            });
        case joystick_button_mask:
            return tr("joystick_input", "Button {button_name}").format({
                {"button_name", string(data & 0xff)}
            });
        case mouse_movement_mask:
            switch(data)
            {
            case 0: return tr("mouse_input", "X axis{sign}").format({{"sign", sign}});
            case 1: return tr("mouse_input", "Y axis{sign}").format({{"sign", sign}});
            }
            break;
        case mouse_wheel_mask:
            switch(data)
            {
            case 0: return tr("mouse_input", "Wheel sideways{sign}").format({{"sign", sign}});
            case 1: return tr("mouse_input", "Wheel{sign}").format({{"sign", sign}});
            }
            break;
        case game_controller_button_mask:
            switch(data & 0xff)
            {
            case SDL_CONTROLLER_BUTTON_A: return "A";
            case SDL_CONTROLLER_BUTTON_B: return "B";
            case SDL_CONTROLLER_BUTTON_X: return "X";
            case SDL_CONTROLLER_BUTTON_Y: return "Y";
            case SDL_CONTROLLER_BUTTON_BACK: return tr("controller_input", "Back");
            case SDL_CONTROLLER_BUTTON_GUIDE: return tr("controller_input", "Guide");
            case SDL_CONTROLLER_BUTTON_START: return tr("controller_input", "Start");
            case SDL_CONTROLLER_BUTTON_LEFTSTICK: return tr("controller_input_left", "L stick button");
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return tr("controller_input_right", "R stick button");
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return tr("controller_input_left", "L shoulder");
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return tr("controller_input_right", "R shoulder");
            case SDL_CONTROLLER_BUTTON_DPAD_UP: return tr("controller_input_dpad", "Pad up");
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return tr("controller_input_dpad", "Pad down");
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return tr("controller_input_dpad", "Pad left");
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return tr("controller_input_dpad", "Pad right");
            }
            break;
        case game_controller_axis_mask:
            switch(data & 0xff)
            {
            case SDL_CONTROLLER_AXIS_LEFTX: return tr("controller_input_left", "L stick X axis{sign}").format({{"sign", sign}});
            case SDL_CONTROLLER_AXIS_LEFTY: return tr("controller_input_left", "L stick Y axis{sign}").format({{"sign", sign}});
            case SDL_CONTROLLER_AXIS_RIGHTX: return tr("controller_input_right", "R stick X axis{sign}").format({{"sign", sign}});
            case SDL_CONTROLLER_AXIS_RIGHTY: return tr("controller_input_right", "R stick Y axis{sign}").format({{"sign", sign}});
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return tr("controller_input_left", "L trigger{sign}").format({{"sign", sign}});
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return tr("controller_input_right", "R trigger{sign}").format({{"sign", sign}});
            }
            break;
        case virtual_mask:
            return tr("virtual_input", "Virtual-{number}").format({{"number", string(data & 0xff)}});
        }
        return tr("unknown_input", "Unknown");
    }
    return "";
}

bool Keybinding::get() const
{
    return value > threshold || down_event;
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

void Keybinding::startUserRebind(Type bind_type, Interaction bind_interaction)
{
    rebinding_key = this;
    rebinding_type = bind_type;
    rebinding_interaction = bind_interaction;
}

void Keybinding::cancelUserRebind()
{
    rebinding_key = nullptr;
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
    for (int n = 0; n < SDL_NumJoysticks(); n++)
        if (SDL_IsGameController(n)) count += 1;
    return count;
}

void Keybinding::loadKeybindings(const string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) return;
    std::stringstream data;
    data << file.rdbuf();
    std::string err;

    auto parsed_json = sp::json::parse(data.str(), err);
    if (!parsed_json.has_value())
    {
        LOG(Warning, "Failed to load keybindings from", filename, ": ", err);
        return;
    }

    auto json = parsed_json.value();

    const std::vector<std::pair<string, Interaction>> interaction_names = {
        {"Continuous", Interaction::Continuous},
        {"Discrete", Interaction::Discrete},
        {"Repeating", Interaction::Repeating},
        {"Axis0", Interaction::Axis0},
        {"Axis1", Interaction::Axis1},
    };

    for (Keybinding* keybinding = keybindings; keybinding; keybinding = keybinding->next)
    {
        const auto& entry = json[keybinding->name];
        if (!entry.is_object()) continue;

        if (entry["key"].is_string())
        {
            string key_str = entry["key"].get<std::string>();
            Interaction inter = Interaction::None;

            for (const auto& pair : interaction_names)
            {
                string suffix = ":" + pair.first;
                if (key_str.endswith(suffix))
                {
                    key_str = key_str.substr(0, key_str.length() - suffix.length());
                    inter = pair.second;
                    break;
                }
            }

            keybinding->setKey(key_str);

            if (!keybinding->bindings.empty())
                keybinding->bindings.back().interaction = inter;
        }
        else if (entry["key"].is_array())
        {
            keybinding->clearKeys();
            for (const auto& key_entry : entry["key"])
            {
                if (key_entry.is_string())
                {
                    string key_str = key_entry.get<std::string>();
                    Interaction inter = Interaction::None;
                    for (const auto& pair : interaction_names)
                    {
                        string suffix = ":" + pair.first;
                        if (key_str.endswith(suffix))
                        {
                            key_str = key_str.substr(0, key_str.length() - suffix.length());
                            inter = pair.second;
                            break;
                        }
                    }
                    keybinding->addKey(key_str);

                    if (!keybinding->bindings.empty())
                        keybinding->bindings.back().interaction = inter;
                }
            }
        }
        else keybinding->bindings.clear();

        // Get step size and threshold on discrete inputs.
        if (entry.contains("discrete") && entry["discrete_step"].is_number())
            keybinding->discrete_step_size = entry["discrete_step"].get<float>();
        if (entry.contains("discrete") && entry["discrete_threshold"].is_number())
            keybinding->threshold = entry["discrete_threshold"].get<float>();
        // Get delay values on repeating inputs.
        if (entry.contains("repeat") && entry["repeat_delay"].is_number())
            keybinding->repeat_delay = entry["repeat_delay"].get<int>();
        if (entry.contains("repeat") && entry["repeat_interval"].is_number())
            keybinding->repeat_interval = entry["repeat_interval"].get<int>();
        // Get sensitivity value on continuous inputs.
        if (entry.contains("continuous") && entry["continuous_sensitivity"].is_number())
            keybinding->sensitivity = entry["continuous_sensitivity"].get<float>();
    }
    LOG(Info, "Keybindings loaded from ", filename);
}

void Keybinding::saveKeybindings(const string& filename)
{
    nlohmann::json obj;
    for(Keybinding* keybinding = keybindings; keybinding; keybinding=keybinding->next)
    {
        nlohmann::json data;
        nlohmann::json keys;
        for(unsigned int index=0; index<keybinding->bindings.size(); index++)
        {
            string key_str = keybinding->getKey(index);
            Interaction inter = keybinding->bindings[index].interaction;
            if (inter != Interaction::None)
            {
                string interaction_name;
                switch (inter)
                {
                case Interaction::Continuous: interaction_name = "Continuous"; break;
                case Interaction::Discrete: interaction_name = "Discrete"; break;
                case Interaction::Repeating: interaction_name = "Repeating"; break;
                case Interaction::Axis0: interaction_name = "Axis0"; break;
                case Interaction::Axis1: interaction_name = "Axis1"; break;
                default: break;
                }
                if (!interaction_name.empty())
                    key_str += ":" + interaction_name;
            }
            keys.push_back(key_str.c_str());
        }
        data["key"] = keys;

        // If interaction properties are defined, serialize them.
        // TODO: Move defaults to consts.
        if (keybinding->discrete_step_size != 0.1f)
            data["discrete_step"] = keybinding->discrete_step_size;
        if (keybinding->threshold != 0.5f)
            data["discrete_threshold"] = keybinding->threshold;
        if (keybinding->repeat_delay != 500)
            data["repeat_delay"] = keybinding->repeat_delay;
        if (keybinding->repeat_interval != 40)
            data["repeat_interval"] = keybinding->repeat_interval;
        if (keybinding->sensitivity != 1.0f)
            data["continuous_sensitivity"] = keybinding->sensitivity;
        obj[keybinding->name] = data;
    }

    std::ofstream file(filename);
    file << obj.dump();
    LOG(Info, "Keybindings saved to ", filename);
}

Keybinding* Keybinding::getByName(const string& name)
{
    for (Keybinding* keybinding = keybindings; keybinding; keybinding = keybinding->next)
        if (keybinding->name == name) return keybinding;
    return nullptr;
}

std::vector<string> Keybinding::getCategories()
{
    std::vector<string> result;
    std::unordered_set<string> found;
    for (Keybinding* keybinding = keybindings; keybinding; keybinding = keybinding->next)
    {
        if (found.find(keybinding->getCategory()) != found.end()) continue;
        found.insert(keybinding->getCategory());
        result.push_back(keybinding->getCategory());
    }
    return result;
}

std::vector<Keybinding*> Keybinding::listAllByCategory(const string& category)
{
    std::vector<Keybinding*> result;
    for (Keybinding* keybinding = keybindings; keybinding; keybinding = keybinding->next)
        if (keybinding->getCategory() == category) result.push_back(keybinding);

    return result;
}

void Keybinding::setVirtualKey(int index, float value)
{
    SDL_assert(index >= 0 && index <= 255); // "Virtual key indices need to be in the range 0-255"

    updateKeys(virtual_mask | index, value);
}

void Keybinding::addBinding(int key, bool inverted, Interaction interaction)
{
    for (auto& bind : bindings)
    {
        if (bind.key == key)
        {
            bind.inverted = inverted;
            return;
        }
    }
    bindings.push_back({key, inverted, interaction});
}

void Keybinding::setValue(float new_value, int key_type, Interaction bind_interaction, float prev_bind_value)
{
    // Run existing per-key threshold/event logic first (delegate to 2-param overload).
    setValue(new_value, key_type);

    // Per-interaction state update.
    float threshold_value = fabs(new_value);
    if (threshold_value < deadzone) threshold_value = 0.0f;
    float prev_threshold = fabs(prev_bind_value);
    if (prev_threshold < deadzone) prev_threshold = 0.0f;

    switch (bind_interaction)
    {
    case Interaction::Continuous:
        continuous_value = new_value;
        LOG(Debug, "Continuous: ", continuous_value);
        break;
    case Interaction::Discrete:
        if (prev_threshold < threshold && threshold_value >= threshold)
            discrete_step_down = true;
        if (prev_threshold >= threshold && threshold_value < threshold)
            discrete_step_up = true;
        LOG(Debug, "Discrete: down ", discrete_step_down ? "true" : "false", " up ", discrete_step_up ? "true" : "false");
        break;
    case Interaction::Repeating:
        if (prev_threshold < threshold && threshold_value >= threshold)
        {
            repeat_ready = true;
            repeat_hold_ticks = SDL_GetTicks();
            repeat_started = false;
        }

        if (prev_threshold >= threshold && threshold_value < threshold)
        {
            repeat_hold_ticks = 0;
            repeat_started = false;
        }
        LOG(Debug, "Repeating: ready ", repeat_ready ? "true" : "false", " up ", discrete_step_up ? "true" : "false", "\nhold ticks: ", repeat_hold_ticks, " started: ", repeat_started ? "true" : "false");
        break;
    case Interaction::Axis0:
        axis0_value = std::clamp(new_value, 0.0f, 1.0f);
        LOG(Debug, "Axis0: ", axis0_value, " (", new_value, ")");
        break;
    case Interaction::Axis1:
        axis1_value = new_value;
        LOG(Debug, "Axis1: ", axis1_value);
        break;
    default:
        break;
    }
}

void Keybinding::setValue(float new_value, int key_type)
{
    // Handle axes that return negative values.
    float threshold_value = fabs(new_value);

    // Add a deadzone by default to prevent inputs that emit non-zero jitter at
    // rest from triggering events.
    if (threshold_value < deadzone)
    {
        threshold_value = 0.0f;
        new_value = 0.0f;
    }

    // Transition between keydown/up only for buttons and non-movement axes.
    // Can't avoid game_controller axes because some have axes on buttons and
    // triggers.
    if ((key_type & type_mask) != mouse_movement_mask && (key_type & type_mask) != joystick_axis_mask)
    {
        if (this->value < threshold && threshold_value >= threshold) down_event = true;
        if (this->value >= threshold && threshold_value < threshold) up_event = true;
    }

    // Set the keybind's value to the new value.
    this->value = new_value;
}

void Keybinding::postUpdate()
{
    if (down_event) down_event = false;
    else if (up_event) up_event = false;

    discrete_step_down = false;
    discrete_step_up = false;
    repeat_ready = false;
}

static int release_mouse = 0;

void Keybinding::allPostUpdate()
{
    // Update keybinds.
    for (Keybinding* keybinding = keybindings; keybinding; keybinding = keybinding->next)
        keybinding->postUpdate();

    // Tick clock.
    unsigned int now = SDL_GetTicks();

    // Repeat repeating binds.
    for (Keybinding* keybinding = keybindings; keybinding; keybinding = keybinding->next)
    {
        if (keybinding->repeat_hold_ticks == 0) continue;

        bool has_repeating = false;
        for (const auto& bind : keybinding->bindings)
        {
            if (bind.interaction == Interaction::Repeating)
                has_repeating = true;
            break;
        }
        if (!has_repeating) continue;

        const unsigned int wait = keybinding->repeat_started
            ? repeat_interval
            : repeat_delay + repeat_interval;

        if (now - keybinding->repeat_hold_ticks >= wait)
        {
            keybinding->repeat_ready = true;
            keybinding->repeat_hold_ticks = now;
            keybinding->repeat_started = true;
        }
    }

    // Update mouse state.
    if (release_mouse & (1 << 0)) updateKeys(0 | mouse_wheel_mask, 0.0);
    if (release_mouse & (1 << 1)) updateKeys(1 | mouse_wheel_mask, 0.0);

    if (release_mouse & (1 << 2)) updateKeys(0 | mouse_movement_mask, 0.0);
    if (release_mouse & (1 << 3)) updateKeys(1 | mouse_movement_mask, 0.0);
    release_mouse = 0;
}

void Keybinding::handleEvent(const SDL_Event& event)
{
    switch(event.type)
    {
    case SDL_KEYDOWN:
        // Ignore function keys (F1-F24) if SDL text input is active
        if (!SDL_IsTextInputActive()
            || (event.key.keysym.sym >= SDLK_F1 && event.key.keysym.sym <= SDLK_F12)
            || (event.key.keysym.sym >= SDLK_F13 && event.key.keysym.sym <= SDLK_F24)
        )
            updateKeys(event.key.keysym.sym | keyboard_mask, 1.0f);
        break;
    case SDL_KEYUP:
        if (!SDL_IsTextInputActive()
            || (event.key.keysym.sym >= SDLK_F1 && event.key.keysym.sym <= SDLK_F12)
            || (event.key.keysym.sym >= SDLK_F13 && event.key.keysym.sym <= SDLK_F24)
        )
            updateKeys(event.key.keysym.sym | keyboard_mask, 0.0f);
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
                updateKeys(static_cast<int>(button) | pointer_mask, 1.0f);
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
                updateKeys(static_cast<int>(button) | pointer_mask, 0.0f);
        }
        break;
    case SDL_MOUSEMOTION:
        {
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
        }
        break;
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
        updateKeys(static_cast<int>(io::Pointer::Button::Touch) | pointer_mask, 1.0f);
        break;
    case SDL_FINGERUP:
        //event.tfinger.x, event.tfinger.x
        updateKeys(static_cast<int>(io::Pointer::Button::Touch) | pointer_mask, 0.0f);
        break;

    // To avoid competition between SDL_JOY... and SDL_CONTROLLER... events,
    // skip joystick events for devices that also open as game controllers.
    // Especially can't treat GC axes like joysticks because GCs report 0 at
    // rest and joysticks report -1.
    case SDL_JOYBUTTONDOWN:
        if (!SDL_GameControllerFromInstanceID(event.jbutton.which))
            updateKeys(static_cast<int>(event.jbutton.button) | static_cast<int>(event.jbutton.which) << 8 | joystick_button_mask, 1.0f);
        break;
    case SDL_JOYBUTTONUP:
        if (!SDL_GameControllerFromInstanceID(event.jbutton.which))
            updateKeys(static_cast<int>(event.jbutton.button) | static_cast<int>(event.jbutton.which) << 8 | joystick_button_mask, 0.0f);
        break;
    case SDL_JOYAXISMOTION:
        if (!SDL_GameControllerFromInstanceID(event.jaxis.which))
            updateKeys(static_cast<int>(event.jaxis.axis) | static_cast<int>(event.jaxis.which) << 8 | joystick_axis_mask, static_cast<float>(event.jaxis.value) / 32768.0f);
        break;
    case SDL_JOYDEVICEADDED:
        if (!SDL_IsGameController(event.jdevice.which))
        {
            SDL_Joystick* joystick = SDL_JoystickOpen(event.jdevice.which);
            if (joystick)
                LOG(Info, "Found joystick:", SDL_JoystickName(joystick));
            else LOG(Warning, "Failed to open joystick...");
        }
        break;

    case SDL_JOYDEVICEREMOVED:
        for (int button = 0; button < 32; button++)
            updateKeys(button | static_cast<int>(event.jdevice.which) << 8 | joystick_button_mask, 0.0f);
        for (int axis = 0; axis < 32; axis++)
            updateKeys(axis | static_cast<int>(event.jdevice.which) << 8 | joystick_axis_mask, 0.0f);

        SDL_JoystickClose(SDL_JoystickFromInstanceID(event.jdevice.which));
        break;
    case SDL_CONTROLLERAXISMOTION:
        updateKeys(static_cast<int>(event.caxis.axis) | static_cast<int>(event.caxis.which) << 8 | game_controller_axis_mask, static_cast<float>(event.caxis.value) / 32768.0f);
        break;
    case SDL_CONTROLLERBUTTONDOWN:
        updateKeys(static_cast<int>(event.cbutton.button) | static_cast<int>(event.cbutton.which) << 8 | game_controller_button_mask, 1.0f);
        break;
    case SDL_CONTROLLERBUTTONUP:
        updateKeys(static_cast<int>(event.cbutton.button) | static_cast<int>(event.cbutton.which) << 8 | game_controller_button_mask, 0.0f);
        break;
    case SDL_CONTROLLERDEVICEADDED:
        {
            SDL_GameController* gc = SDL_GameControllerOpen(event.cdevice.which);
            if (gc) LOG(Info, "Found game controller:", SDL_GameControllerName(gc));
            else LOG(Warning, "Failed to open game controller...");
        }
        break;
    case SDL_CONTROLLERDEVICEREMOVED:
        for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; button++)
            updateKeys(button | static_cast<int>(event.cdevice.which) << 8 | game_controller_button_mask, 0.0f);
        for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
        {
            updateKeys(axis | static_cast<int>(event.cdevice.which) << 8 | game_controller_axis_mask, 0.0f);
        }

        SDL_GameControllerClose(SDL_GameControllerFromInstanceID(event.cdevice.which));
        break;
    case SDL_CONTROLLERDEVICEREMAPPED:
        break;
    case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        {
            // Focus lost, release all keys.
            for (Keybinding* keybinding = keybindings; keybinding; keybinding = keybinding->next)
                if (keybinding->bindings.size() > 0) keybinding->setValue(0.0f);
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
        if ((value > threshold || value < -threshold) && (key_number & (static_cast<int>(rebinding_type) << 16)))
        {
            rebinding_key->addBinding(key_number, value < 0.0f, rebinding_interaction);
            rebinding_key = nullptr;
        }
    }

    for (Keybinding* keybinding = keybindings; keybinding; keybinding = keybinding->next)
    {
        for (auto& bind : keybinding->bindings)
        {
            if (bind.key == key_number)
            {
                float v = bind.inverted ? -value : value;
                keybinding->setValue(v, key_number, bind.interaction, bind.last_value);
                bind.last_value = (fabs(v) < deadzone) ? 0.0f : v;
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

bool operator&(Keybinding::Interaction a, Keybinding::Interaction b)
{
    return static_cast<int>(a) & static_cast<int>(b);
}

Keybinding::Interaction operator|(Keybinding::Interaction a, Keybinding::Interaction b)
{
    return static_cast<Keybinding::Interaction>(static_cast<int>(a) | static_cast<int>(b));
}

}//namespace io
}//namespace sp
