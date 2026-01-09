--- @meta
--- @diagnostic disable:inject-field

--- @param description string
--- @param body fun(): nil
describe = function(description, body) end

--- @param description string
--- @param body fun(): nil
it = function(description, body) end

--- @param it any
assert.is_true = function(it) end

--- @param it any
assert.is_false = function(it) end

--- @param it nil
assert.is_nil = function(it) end

--- @generic T
--- @param expected T
--- @param received T
assert.are_equal = function(expected, received) end

--- @generic T
--- @param expected T
--- @param received T
assert.are_not_equal = function(expected, received) end

--- @generic T
--- @param expected T
--- @param received T
assert.are_same = function(expected, received) end
