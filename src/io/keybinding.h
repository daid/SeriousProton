#pragma once

#include <stringImproved.h>
#include <nonCopyable.h>
#include <io/pointer.h>

//Forward declare the event structure, so we can pass it by reference to the keybindings, without exposing all SDL stuff.
union SDL_Event;

class Engine;

namespace sp {
namespace io {

class Keybinding : sp::NonCopyable
{
public:
    // Key type, indicative of the input (device) method.
    // What is being used to trigger this bind?
    // The user's input defines the input type upon binding.
    enum class Type {
        None = 0,
        Keyboard = (1 << 0),
        Pointer = (1 << 1),
        JoystickButton = (1 << 2),
        JoystickAxis = (1 << 3),
        MouseMovement = (1 << 4),
        MouseWheel = (1 << 5),
        ControllerButton = (1 << 6),
        ControllerAxis = (1 << 7),
        Virtual = (1 << 8),

        Joystick = JoystickButton | JoystickAxis,
        Controller = ControllerButton | ControllerAxis,
        Mouse = Pointer | MouseMovement | MouseWheel,

        Default = Keyboard | Virtual | Joystick | Controller | Mouse,
    };

    // Key interaction types, indicative of the output (control) method.
    // How does this bind trigger its bound control?
    // The control should define available interactions, not the user.
    enum class Interaction {
        // No defined interaction. Equivalent to Continuous but should emit a
        // warning.
        None = 0,

        // Default. Binary actions that fire continuously at a steady rate each
        // update while onDown/non-zero and stop on onUp; momentary controls,
        // digital steering.
        // EE examples: Missile tube firing controls.
        Continuous = (1 << 0),

        // Binary actions that fire once onDown/past a threshold regardless of
        // time held; encoders, toggle switches, controls with detent-only
        // values.
        // EE examples: Most buttons, warp factor slider, scan sliders.
        Discrete = (1 << 1),

        // Binary actions that fire once onDown/past a threshold, waits for a
        // period, and then fires the action repeatedly on an interval.
        // Alternative to both Discrete and Continuous interactions, for
        // backward compatibility with pre-legacy SFML behaviors.
        Repeating = (1 << 2),

        // Actions with variable values between 0 and 1, typically with
        // deadzones at 0 and 1. Linear triggers and potentiometers,
        // pressure-sensitive buttons, unidirectional throttles.
        // EE examples: Combat manevuer forward boost, jump distance slider.
        Axis0 = (1 << 3),

        // Actions with variable values between -1 and 1, typically with a
        // deadzone at 0. Steering axes, bidirectional throttles.
        // EE examples: Helms rotation, impulse throttle.
        Axis1 = (1 << 4)
    };

    // Create a keybinding, and optionally set the default key(s). See setKey for documentation on key naming.
    Keybinding(const string& name);
    Keybinding(const string& name, const string& default_key);
    Keybinding(const string& name, const std::initializer_list<const string>& default_keys);
    ~Keybinding();

    const string& getName() const { return name; }
    const string& getLabel() const { return label; }
    const string& getCategory() const { return category; }
    // Get the Keybinding::Interaction of this bind. Returns None if the index
    // is out of range.
    Interaction getInteraction(int index) const;
    void setLabel(const string& label) { this->label = label; }
    void setLabel(const string& category, const string& label) { this->category = category; this->label = label; }

    // Set/get/add bound keys to the keybinding.
    // Key name should be one of the following:
    // * name of a keyboard key ("Space", "Left", "A")
    // * "joy:[id]:axis:[axis]" for a joystick axis, where id is the index of the joystick and axis is the axis number.
    // * "joy:[id]:button:[button]" for a joystick button, where id is the index of the joystick and axis is the button number.
    // * "gamecontroller:[id]:axis:[axis]", where axis is "leftx", "lefty", "rightx", "righty", "lefttrigger", or "righttrigger"
    // * "gamecontroller:[id]:button:[button]", where button is one of "a", "b", "x", "y", "back", "guide", "start", "leftstick", "rightstick", "leftshoulder", "rightshoulder", "dpup", "dpdown", "dpleft", "dpright"
    // * "mouse:[axis]", where axis is one of "x", "y" for mouse direction binding.
    // * "pointer:[id]", where id is 0 to 4 for left, right, middle mouse button, or a touchscreen press.
    // * "wheel:[dir]", where dir is one of "x", "y" for mouse wheel binding. Note that these are never held down and generate only up and down events.
    // * "virtual:[id]", where id is the number of a virtual key, controlled by setVirtualKey, and the virtual touchscreen controls.
    void setKey(const string& key);
    void setKeys(const std::initializer_list<const string>& keys);
    void addKey(const string& key, bool inverted=false);
    void removeKey(int index);
    void clearKeys();

    static void setDeadzone(float deadzone);

    // Get the name of the key in the same format as used for setKey and friends.
    // Returns empty string if the index as no set key.
    string getKey(int index) const;
    // Get the Keybinding::Type of this bind. Returns None if the index is out
    // of range.
    Type getKeyType(int index) const;
    // Get a human-readable name of the key for display purposes.
    string getHumanReadableKeyName(int index) const;

    // Returns true if there is anything bound to this keybinding.
    bool isBound() const;

    // Returns true when this key is currently being pressed.
    bool get() const;
    // Returns true for 1 update cycle when the key is pressed.
    bool getDown() const;
    // Returns true for 1 update cycle when the key is released.
    bool getUp() const;
    // Returns a value in the range -1 to 1 for this keybinding. On keyboard keys this is always 0 or 1, but for joysticks this can be anywhere in the range -1.0 to 1.0
    float getValue() const;

    void setSupportedInteractions(Interaction i) { supported_interactions = i; }
    Interaction getSupportedInteractions() const { return supported_interactions; }

    // Per-interaction aggregate query methods.
    // Continuous: sustained value sent every frame and multiplied by sensitivity.
    float getContinuousValue() const { return continuous_value * sensitivity; }
    // Discrete: single step applied once per press.
    bool isDiscreteStepDown() const { return discrete_step_down; }
    bool isDiscreteStepUp() const { return discrete_step_up; }
    // Returns discrete_step_size when a Discrete bind fired this frame, 0 otherwise.
    float getDiscreteValue() const { return discrete_step_down ? discrete_step_size : 0.0f; }
    // Repeating: single discrete step applied once, then repeatedly on an
    // interval after a delay.
    bool isRepeatReady() const { return repeat_ready; }
    // Axis0: axis value from 0.0 to 1.0, dead at extremities.
    float getAxis0Value() const { return axis0_value; }
    // Axis1: axis value from -1.0 to 1.0, dead at 0.0.
    float getAxis1Value() const { return axis1_value; }

    // Per-keybinding configuration.
    float getDiscreteStepSize() const { return discrete_step_size; }
    void setDiscreteStepSize(float v) { discrete_step_size = v; }
    unsigned int getRepeatDelay() const { return repeat_delay; }
    void setRepeatDelay(unsigned int v) { repeat_delay = v; }
    unsigned int getRepeatInterval() const { return repeat_interval; }
    void setRepeatInterval(unsigned int v) { repeat_interval = v; }
    float getContinuousSensitivity() const { return sensitivity; }
    void setContinuousSensitivity(float v) { sensitivity = v; }

    // Start a binding process from the user. The next button pressed by the
    // user will be bound to this key. This will add on top of the existing
    // binds, so clearKeys() must be called in order to bind only one input.
    void startUserRebind(Type bind_type = Type::Default, Interaction bind_interaction = Interaction::None);
    // Cancel any in-progress user rebind, regardless of which keybinding started it.
    static void cancelUserRebind();
    bool isUserRebinding() const;

    static int joystickCount();
    static int gamepadCount();

    static void loadKeybindings(const string& filename);
    static void saveKeybindings(const string& filename);

    static Keybinding* getByName(const string& name);

    static std::vector<string> getCategories();
    static std::vector<Keybinding*> listAllByCategory(const string& category);

    static void setVirtualKey(int index, float value);

    std::vector<string> getDefaultBindings() { return default_bindings; }
private:
    string name;
    string category;
    string label;
    std::vector<string> default_bindings;

    struct Binding
    {
        int key;
        bool inverted;
        Interaction interaction = Interaction::None;
        float last_value = 0.0f;
    };
    std::vector<Binding> bindings;
    Interaction supported_interactions = Interaction::None;

    // Deadzone tolerance.
    static float deadzone;
    // Discrete input step size
    static float discrete_step_size;
    // Discrete input +/- threshold.
    static float threshold;
    // Repeating input delay after first event, in milliseconds.
    static unsigned int repeat_delay;
    // Repeating input interval after second event, in milliseconds.
    static unsigned int repeat_interval;
    // Continuous input sensitivity factor.
    static float sensitivity;

    float value;
    bool down_event;
    bool up_event;
    float continuous_value = 0.0f;
    bool discrete_step_down = false;
    bool discrete_step_up = false;
    bool repeat_ready = false;
    unsigned int repeat_hold_ticks = 0; // SDL ticks at initial press (0 = not held)
    bool repeat_started = false; // true after first repeat has fired
    float axis0_value = 0.0f;
    float axis1_value = 0.0f;

    string getKeyInternal(int index) const;
    void addBinding(int key, bool inverted, Interaction interaction = Interaction::None);

    void setValue(float new_value, int key_type, Interaction bind_interaction, float prev_bind_value);
    void setValue(float new_value, int key_type = 0);
    void postUpdate();
    
    static void allPostUpdate();
    
    static void handleEvent(const SDL_Event& event);
    static void updateKeys(int key_number, float value);
    
    static Keybinding* keybindings;
    Keybinding* next=nullptr;
    static Keybinding* rebinding_key;
    static Type rebinding_type;
    static Interaction rebinding_interaction;
    
    static constexpr int type_mask = 0xfff << 16;
    static constexpr int keyboard_mask = static_cast<int>(Type::Keyboard) << 16;
    static constexpr int pointer_mask = static_cast<int>(Type::Pointer) << 16;
    static constexpr int joystick_button_mask = static_cast<int>(Type::JoystickButton) << 16;
    static constexpr int joystick_axis_mask = static_cast<int>(Type::JoystickAxis) << 16;
    static constexpr int mouse_movement_mask = static_cast<int>(Type::MouseMovement) << 16;
    static constexpr int mouse_wheel_mask = static_cast<int>(Type::MouseWheel) << 16;
    static constexpr int game_controller_button_mask = static_cast<int>(Type::ControllerButton) << 16;
    static constexpr int game_controller_axis_mask = static_cast<int>(Type::ControllerAxis) << 16;
    static constexpr int virtual_mask = static_cast<int>(Type::Virtual) << 16;

    friend class ::Engine;
};

bool operator&(const Keybinding::Type a, const Keybinding::Type b);
Keybinding::Type operator|(const Keybinding::Type a, const Keybinding::Type b);

bool operator&(Keybinding::Interaction a, Keybinding::Interaction b);
Keybinding::Interaction operator|(Keybinding::Interaction a, Keybinding::Interaction b);

}//namespace io
}//namespace sp
