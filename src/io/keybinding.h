#ifndef SP_IO_KEYBINDING_H
#define SP_IO_KEYBINDING_H

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

        Default = Keyboard | Pointer | Joystick | Controller | Virtual,
    };

    //Create a keybinding, and optionally set the default key(s). See setKey for documentation on key naming.
    Keybinding(const string& name);
    Keybinding(const string& name, const string& default_key);
    Keybinding(const string& name, const std::initializer_list<const string>& default_keys);
    ~Keybinding();

    const string& getName() const { return name; }
    const string& getLabel() const { return label; }
    const string& getCategory() const { return category; }
    void setLabel(const string& label) { this->label = label; }
    void setLabel(const string& category, const string& label) { this->category = category; this->label = label; }

    //Set/get/add bound keys to the keybinding.
    //  Key name should be one of the following:
    //  * name of a keyboard key ("Space", "Left", "A")
    //  * "joy:[id]:axis:[axis]" for a joystick axis, where id is the index of the joystick. And axis is the axis number.
    //  * "joy:[id]:button:[button]" for a joystick button, where id is the index of the joystick. And axis is the button number.
    //  * "gamecontroller:[id]:axis:[axis], where axis is "leftx", "lefty", "rightx", "righty", "lefttrigger" or "righttrigger"
    //  * "gamecontroller:[id]:button:[button], where button is one of "a", "b", "x", "y", "back", "guide", "start", "leftstick", "rightstick", "leftshoulder", "rightshoulder", "dpup", "dpdown", "dpleft", "dpright"
    //  * "pointer:[id], where id is 0 to 4 for left,right,middle mouse button, or a touch screen press.
    //  * "wheel:[dir], where dir is one of "x", "y" for mouse wheel binding. Note that these are never helt down. Only generate up&down events.
    //  * "virtual:[id], where id is the number of a virtual key, controlled by setVirtualKey, and the virtual touchscreen controls.
    void setKey(const string& key);
    void setKeys(const std::initializer_list<const string>& keys);
    void addKey(const string& key, bool inverted=false);
    void removeKey(int index);
    void clearKeys();

    // Get the name of the key in the same format as used for setKey and friends. Returns empty string if the index as no set key.
    string getKey(int index) const;
    Type getKeyType(int index) const;
    // Get a human readable name of the key for display purposes.
    string getHumanReadableKeyName(int index) const;

    //Returns true if there is anything bound to this keybinding.
    bool isBound() const;

    bool get() const; //True when this key is currently being pressed.
    bool getDown() const; //True for 1 update cycle when the key is pressed.
    bool getUp() const; //True for 1 update cycle when the key is released.
    float getValue() const; //Returns a value in the range -1 to 1 for this keybinding. On keyboard keys this is always 0 or 1, but for joysticks this can be anywhere in the range -1.0 to 1.0

    //Start a binding process from the user. The next button pressed by the user will be bound to this key.
    //Note that this will add on top of the already existing binds, so clearKeys() need to be called if you want to bind a single key.
    void startUserRebind(Type bind_type=Type::Default);
    bool isUserRebinding() const;

    static int joystickCount();
    static int gamepadCount();

    static void loadKeybindings(const string& filename);
    static void saveKeybindings(const string& filename);

    static Keybinding* getByName(const string& name);

    static std::vector<string> getCategories();
    static std::vector<Keybinding*> listAllByCategory(const string& category);

    static void setVirtualKey(int index, float value);
private:
    string name;
    string category;
    string label;

    struct Binding
    {
        int key;
        bool inverted;
    };
    std::vector<Binding> bindings;

    float value;
    bool down_event;
    bool up_event;

    string getKeyInternal(int index) const;
    void addBinding(int key, bool inverted);

    void setValue(float value);
    void postUpdate();
    
    static void allPostUpdate();
    
    static void handleEvent(const SDL_Event& event);
    static void updateKeys(int key_number, float value);
    
    static Keybinding* keybindings;
    Keybinding* next=nullptr;
    static Keybinding* rebinding_key;
    static Type rebinding_type;
    
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

}//namespace io
}//namespace sp

#endif//SP2_IO_KEYBINDING_H
