Me and My Shadow Script API Reference
=====================================

(draft)

The script language is Lua 5.3.

Always check `ScriptAPI.cpp` for the newest API changed unmentioned in this document.

How to edit script
==================

To edit the script of a block, right click the block and select "Scripting".

To edit the script of the level, right click the empty space of the level and select "Scripting".
You can also edit the `<level_file_name>.lua` file,
if this file is present it will be executed before the "onCreate" event of level.

Currently the scenery block doesn't support scripting.

Available event types of block:

Event type            | Description
----------------------|--------------
"playerWalkOn"        | Fired once when the player walks on. (For example this is used in fragile block.)
"playerIsOn"          | Fired every frame when the player is on.
"playerLeave"         | Fired once when the player leaves.
"onCreate"            | Fired when object creates.
"onEnterFrame"        | Fired every frame.
"onPlayerInteraction" | Fired when the player press DOWN key. Currently this event only fires when the block type is TYPE_SWITCH.
"onToggle"            | Fired when the block receives "toggle" from a switch/button. NOTE: The switch/button itself will also receive this event. This is used in an old example found on my old computer.
"onSwitchOn"          | Fired when the block receives "switch on" from a switch/button. NOTE: The switch/button itself will also receive this event. This is used in an old example found on my old computer.
"onSwitchOff"         | Fired when the block receives "switch off" from a switch/button. NOTE: The switch/button itself will also receive this event. This is used in an old example found on my old computer.

NOTE: During the event execution the global variable `this` temporarily points to current block. (Ad-hoc workaround!)
When the event execution ends the global variable `this` is reset to its previous value.

The block event may return an integer value (default is 0) to alter the game logic:

Return value | Description
-------------|--------------
0            | Skip the default game logic for this event.
1            | Do the default game logic for this event.

Available event types of level:

Event type | Description
-----------|--------------
"onCreate" | Fired when the level is created or the game is reset. This happens **after** all the blocks are created and their `onCreate` is called.
"onSave"   | Fired when the game is saved. NOTE: During event execution the global variable `SAVESTATE` is set to `nil`. You can assign a new table to it and save necessary states inside it. After event execution `SAVESTATE` will be saved to other place and is reset to its previous value.
"onLoad"   | Fired when the game is loaded. NOTE: During event execution the global variable `SAVESTATE` is loaded and you can restore necessary states from it. After event execution `SAVESTATE` is reset to its previous value.

For the newest lists of event types, see `init()` function in `Functions.cpp`.

NOTE: the following methods to specify scripts can be used:

* Specify scripts for each events in the block script editing dialog.
* Only specify `onCreate` script in the block script editing dialog,
  and use `setEventHandler()` function in script to specify scripts for other events dynamically.
* Only specify `onCreate` script in the level script editing dialog
  (you can put this script to the `<level_file_name>.lua` file),
  and use `setEventHandler()` function in script to specify scripts for other events for level/blocks dynamically.

List of block names
===================

The block name is used in script and `.map` file, reference: `Game.cpp`.
The constant name is used in C++ code, reference: `Globals.h`.

Block name           | Constant name
---------------------|---------------
"Block"              | TYPE_BLOCK
"PlayerStart"        | TYPE_START_PLAYER
"ShadowStart"        | TYPE_START_SHADOW
"Exit"               | TYPE_EXIT
"ShadowBlock"        | TYPE_SHADOW_BLOCK
"Spikes"             | TYPE_SPIKES
"ShadowSpikes"       | TYPE_SHADOW_SPIKES
"Checkpoint"         | TYPE_CHECKPOINT
"Swap"               | TYPE_SWAP
"Fragile"            | TYPE_FRAGILE
"ShadowFragile"      | TYPE_SHADOW_FRAGILE
"MovingBlock"        | TYPE_MOVING_BLOCK
"MovingShadowBlock"  | TYPE_MOVING_SHADOW_BLOCK
"MovingSpikes"       | TYPE_MOVING_SPIKES
"MovingShadowSpikes" | TYPE_MOVING_SHADOW_SPIKES
"Teleporter"         | TYPE_PORTAL
"Button"             | TYPE_BUTTON
"Switch"             | TYPE_SWITCH
"ConveyorBelt"       | TYPE_CONVEYOR_BELT
"ShadowConveyorBelt" | TYPE_SHADOW_CONVEYOR_BELT
"NotificationBlock"  | TYPE_NOTIFICATION_BLOCK
"Collectable"        | TYPE_COLLECTABLE
"Pushable"           | TYPE_PUSHABLE
"ShadowPushable"     | TYPE_SHADOW_PUSHABLE

Variable expanding for block messages
=====================================

There is a special syntax for the message of notification block, portal, switch, etc.
If the message contains something like `{{{name}}}` and the `name` is in the following list,
then the `{{{name}}}` will be replaced by corresponding value.

Name            | Value
----------------|-------
`key_<keyName>` | The name of the key `keyName` configured in the options screen. Available key names: "up", "down", "left", "right", "jump", "action", "space", "cancelRecording", "escape", "restart", "tab", "save", "load", "swap", "teleport", "suicide", "shift", "next", "previous", "select". See the options screen for description of these keys.

Script API reference
====================

The "block" library
-------------------

### Static functions:

* getBlockById(id)

Returns the first block with specified id. If not found, returns `nil`.

Example:

~~~lua
local b=block.getBlockById("1")
local x,y=b:getLocation()
print(x..","..y)
~~~

* getBlocksById(id)

Returns the list of all blocks with specified id.

Example:

~~~lua
local l=block.getBlocksById("1")
for i,b in ipairs(l) do
  local x,y=b:getLocation()
  print(x..","..y)
end
~~~

* removeAll()

Remove all blocks.

* addBlock(string,[x],[y],[w],[h])

Add a new block (optionally give it a new position and size) and return the newly created block.

The `string` is the text representation of a block which is used in `.map` file.

The new block can have scripts and the `onCreate` script will be executed immediately.

Example:

~~~lua
-- Assume we have moving blocks of id 1,
-- then this newly added switch can operate existing moving blocks.
block.addBlock([[
tile(Switch,0,0,50,50){
  behaviour=toggle
  id=1
  script(onCreate){
    script="print('Hello world from onCreate of dynamically added block')"
  }
}]],250,300)
~~~

Another example:

~~~lua
local b=block.addBlock('tile(MovingBlock,0,0)')
b:setBaseLocation(
  math.random()*level.getWidth(),
  math.random()*level.getHeight())
local bx,by=b:getBaseLocation()
for i=1,10 do
  b:addMovingPos({
    math.random()*level.getWidth()-bx,
    math.random()*level.getHeight()-by,
    math.random()*80+40
  })
end
b:addMovingPos({0,0,math.random()*80+40})
~~~

* addBlocks(string,[positions]) / addBlocks(string,offsetX,offsetY)

Add new blocks (optionally give them new positions and sizes) and return an array the newly created blocks.

The `string` is the text representation of blocks which is used in `.map` file.

In the first form,
If `string` contains only one block, then it will be created repeatedly using the specified positions.
If it contains more than one block, then each block will use corresponding position in `positions`.
The `positions` is an array of new positions whose entry is of format `{[x],[y],[w],[h]}`.

In the second form,
the `string` can contain one or more blocks,
and the position of each block will be offset by two numbers `offsetX` and `offsetY`.

The new blocks can have scripts and the `onCreate` script will be executed immediately.

### Member functions:

* isValid() -- check the object is valid (i.e. not deleted, etc.)

* moveTo(x,y)

Move the block to the new position, update the velocity of block according to the position changed.

Example:

~~~lua
local b=block.getBlockById("1")
local x,y=b:getLocation()
b:moveTo(x+1,y)
~~~

* getLocation()

Returns the position of the block.

Example: see the example for moveTo().

* setLocation(x,y)

Move the block to the new position without updating the velocity of block.

Example: omitted since it's almost the same as moveTo().

* getBaseLocation() / setBaseLocation(x,y)

Get or set the base position of the block. Mainly used for moving blocks.

* growTo(w,h)

Resize the block, update the velocity of block according to the size changed.

NOTE: I don't think the velocity need to be updated when resizing block, so don't use this function.

Example: omitted since it's almost the same as setSize().

* getSize()

Returns the size of the block.

Example:

~~~lua
local b=block.getBlockById("1")
local w,h=b:getSize()
print(w..","..h)
~~~

* setSize(w,h)

Resize the block without updating the velocity of block.

Example:

~~~lua
local b=block.getBlockById("1")
local w,h=b:getSize()
b:setSize(w+1,h)
~~~

* getBaseSize() / setBaseSize(x,y)

Get or set the base size of the block. Mainly used for moving blocks.

* getType()

Returns the type of the block (which is a string).

Example:

~~~lua
local b=block.getBlockById("1")
local s=b:getType()
print(s)
~~~

* changeThemeState(new_state)

Change the state of the block to new_state (which is a string).

Example:

~~~lua
local b=block.getBlockById("1")
b:changeThemeState("activated")
~~~

* setVisible(b)

Set the visibility the block.

NOTE: The default value is `true`. If set to `false` the block is hidden completely,
the animation is stopped, can't receive any event, can't execute any scripts (except for `onCreate`),
can't be used as a portal destination,
doesn't participate in collision check and game logic, etc...

NOTE: This is a newly added feature.
If you find any bugs (e.g. if an invisible block still affects the game logic)
please report the bugs to GitHub issue tracker.

Example:

~~~lua
local b=block.getBlockById("1")
if b:isVisible() then
  b:setVisible(false)
else
  b:setVisible(true)
end
~~~

* isVisible()

Returns whether the block is visible.

Example: see the example for setVisible().

* getEventHandler(event_type)

Returns the event handler of event_type (which is a string).

Example:

~~~lua
local b=block.getBlockById("1")
local f=b:getEventHandler("onSwitchOn")
b:setEventHandler("onSwitchOff",f)
~~~

* setEventHandler(event_type,handler)

Set the handler of event_type (which is a string). The handler should be a function or `nil`.
Returns the previous event handler.

Example:

~~~lua
local b=block.getBlockById("1")
b:setEventHandler("onSwitchOff",function()
  print("I am switched off.")
end)
~~~

* onEvent(eventType)

Fire an event to specified block.

NOTE: The event will be processed immediately.

Example:

~~~lua
local b=block.getBlockById("1")
b:onEvent("onToggle")
~~~

NOTE: Be careful not to write infinite recursive code! Bad example:

~~~lua
-- onToggle event of a moving block
this:onEvent("onToggle")
~~~

* isActivated() / setActivated(bool)

Get/set a boolean indicates if the block is activated.

The block should be one of TYPE_MOVING_BLOCK, TYPE_MOVING_SHADOW_BLOCK, TYPE_MOVING_SPIKES,
TYPE_CONVEYOR_BELT, TYPE_SHADOW_CONVEYOR_BELT.

* isAutomatic() / setAutomatic(bool)

Get/set a boolean indicates if the portal is automatic.

The block should be TYPE_PORTAL.

* getBehavior() / setBehavior(str)

Get/set a string (must be "on", "off" or "toggle"),
representing the behavior of the block.

The block should be TYPE_BUTTON, TYPE_SWITCH.

* getState() / setState(num)

Get/set a number (must be 0,1,2 or 3),
representing the state of a fragile block.

The block should be TYPE_FRAGILE.

* isPlayerOn()

Get a boolean indicates if the player is on.

Currently only works for TYPE_BUTTON.

* getPathMaxTime()

Get the total time of the path of a moving block.

* getPathTime() / setPathTime(num)

Get/set the current time of the path of a moving block.

* isLooping() / setLooping(bool)

Get/set the looping property of a moving block.

* getSpeed() / setSpeed(num)

Get/set the speed of a conveyor belt.

NOTE: 1 Speed = 0.08 block/s = 0.1 pixel/frame.

* getAppearance() / setAppearance(str)

Get/set the custom appearance of a block.

The `str` is the name of the custom appearance, either `"<blockName>_Scenery"` or name of a scenery block.
Empty string or nil means the default appearance.

* getId() / setId(str)

Get/set the id of a block.

* getDestination() / setDestination(str)

Get/set the destination of a portal.

The block should be TYPE_PORTAL.

* getMessage() / setMessage(str)

Get/set the message of a notification block or a portal or a switch.

The block should be TYPE_NOTIFICATION_BLOCK, TYPE_PORTAL or TYPE_SWITCH.

NOTE: The block message will be always passed to gettext() before showing on the screen.
Therefore it's a good idea to wrap the message with `__()`.
DON'T wrap it with `_()`!

NOTE: The variables in the block message will also be expanded.
See the corresponding section in this document.

Example:

~~~lua
local b=block.getBlockById("1")
b:setMessage(__("do something"))
-- it will show (a localized version of) "do something"
~~~

* getMovingPosCount()

Get the number of moving positions of a moving block.

* getMovingPos() / getMovingPos(index) / getMovingPos(start, length)

Get the array of moving positions or the moving position at specified index
(the array index starts with 1 in Lua).

The individual point is of format `{x,y,t}`.

* setMovingPos(array) / setMovingPos(index, point) / setMovingPos(start, length, array)

Set the array of moving positions or modify the moving position at specified index
(the array index starts with 1 in Lua).

NOTE: the last two forms won't change the number of points,
while the first form will overwrite the list of points completely.

* addMovingPos(p) / addMovingPos(index, p)

Insert points to the array of moving positions at the end or at the specified index.

The `p` can be one point or a list of points.

* removeMovingPos() / removeMovingPos(index) / removeMovingPos(listOfIndices) / removeMovingPos(start, length)

Remove points in the array of moving positions: remove all points,
or remove a point at specified index, or remove points at specified indices,
or remove points in given range.

* clone([x],[y],[w],[h])

Create a clone of current block, optionally give it a new position and size.

The new block can have scripts and the `onCreate` script will be executed immediately.

Returns the newly created block.

* cloneMultiple(number) / cloneMultiple(positions)

Create multiple clones of current block, optionally give them new positions and sizes.

The `number` is the number of clones to made, whose positions are the same as the source block,
whereas `positions` is an array of new positions whose entry is of format `{[x],[y],[w],[h]}`.

The new blocks can have scripts and the `onCreate` script will be executed immediately.

Returns an array of newly created blocks.

* remove()

Remove current block.

The "playershadow" library
--------------------------

### Global constants:

* player

The player object.

* shadow

The shadow object.

### Member functions:

* getLocation()

Returns the location of player/shadow.

Example:

~~~lua
local x,y=player:getLocation()
print("player: "..x..","..y)
x,y=shadow:getLocation()
print("shadow: "..x..","..y)
~~~

* setLocation(x,y)

Set the location of player/shadow.

Example:

~~~lua
local x,y=player:getLocation()
player:setLocation(x+1,y)
~~~

* jump([strength=13])

Let the player/shadow jump if it's allowed.

strength: Jump strength.

Example:

~~~lua
player:jump(20)
~~~

* isShadow()

Returns whether the current object is shadow.

Example:

~~~lua
print(player:isShadow())
print(shadow:isShadow())
~~~

* getCurrentStand()

Returns the block on which the player/shadow is standing on. Can be `nil`.

Example:

~~~lua
local b=player:getCurrentStand()
if b then
  print(b:getType())
else
  print("The player is not standing on any blocks")
end
~~~

* isInAir() -- returns a boolean indicating if the player is in air

* canMove() -- returns a boolean indicating if the player can move (i.e. not standing on shadow)

* isDead() -- returns a boolean indicating if the player is dead

* isHoldingOther() -- returns a boolean indicating if the player is holding other

The "level" library
-------------------

### Static functions:

* getSize() -- get the level size

* getRect() / setRect(x,y,w,h) -- get or set the level rect (left,top,width,height)

* getWidth() -- get the level width

* getHeight() -- get the level height

* getName() -- get the level name

* getEventHandler(event_type) -- get the event handler

* setEventHandler(event_type,handler) -- set the event handler, return the old handler

* win() -- win the game

* getTime() -- get the game time (in frames)

* getRecordings() -- get the number of recordings

* getCollectables() -- get the number of currently obtained collectibles

* getTotalCollectables() -- get the number of total collectibles in the level

* broadcastObjectEvent(eventType,[objectType=nil],[id=nil],[target=nil])

Broadcast the event to blocks satisfying the specified condition.

NOTE: The event will be processed in next frame.

Argument name | Description
--------------|-------------
eventType     | string.
objectType    | string or nil. If this is set then the event is only received by the block with specified type.
id            | string or nil. If this is set then the event is only received by the block with specified id.
target        | block or nil. If this is set then the event is only received by the specified block.

Example:

~~~lua
level.broadcastObjectEvent("onToggle",nil,"1")
~~~

The "delayExecution" library
----------------------------

### Static functions:

* schedule(func,time,[repeatCount=1],[repeatInterval],[enabled=true],[arguments...])

Schedule a delay execution of a given function after the given time.

Argument name  | Description
---------------|-------------
func           | A function to be executed.
time           | Time, given in frames (NOTE: 40 frames = 1 second). NOTE: If <=0 it is the same as =1.
repeatCount    | The number of times the function will be executed. After such number of times executed, the delay execution will be removed from the list and get deleted. If =0 the delay execution object will be deleted soon. If <0 the function will be executed indefinitely.
repeatInterval | The repeat interval. If it is `nil` then the `time` argument will be used instead. NOTE: If <=0 the repeat execution will be disabled at all and the repeatCount will be set to 1.
enabled        | Enabled.
arguments      | Optional arguments passed to the function.

Return value: the delayExecution object.

NOTE: If you want to update time/repeatCount during the function execution,
notice that the time/repeatCount is updated BEFORE the function execution.

NOTE: During the execution the global variable `this`
temporarily points to current delay execution object. (Ad-hoc workaround!)
When the execution ends the global variable `this` is reset to its previous value.

Example:

~~~lua
local f=function()
  local a
  a=0
  return(function(b)
    shadow:jump()
    print('obj1 '..this:getExecutionTime()..' '..a..' '..tostring(b))
    a=a+2
  end)
end

local obj1=delayExecution.schedule(f(),40*2,5,nil,nil,100)

local obj2=delayExecution.schedule(
function(o)
  print('obj2 '..tostring(o:isValid()))
  if not o:isValid() then
    this:setFunc(f())
  end
end,40*1,-1,nil,nil,obj1)

local obj3=delayExecution.schedule(
function(o)
  o:cancel()
end,40*30,1,nil,nil,obj2)
~~~

### Member functions:

* isValid() -- Check if it's valid, i.e. not removed from list.

* cancel() -- Cancels a delay execution. The canceled delay execution will be removed from the list and can not be restored.

* isEnabled()/setEnabled(bool) -- get/set enabled of a delay execution. A disabled one will not count down its timer.

* getTime()/setTime(integer) -- get/set the remaining time until the next execution. NOTE: If <=0 it is the same as =1.

* getRepeatCount()/setRepeatCount(integer) -- get/set the remaining repeat count. If =0 the object will get deleted soon. If <0 the function will be executed indefinitely.

* getRepeatInterval()/setRepeatInterval(integer) -- get/set the repeat interval. NOTE: If <=0 then nothing happens.

* getFunc()/setFunc(func) -- get/set the function to be executed. NOTE: The setFunc will return the original function.

* getArguments()/setArguments(args...) -- get/set the arguments

* getExecutionTime()/setExecutionTime(integer) -- get/set the number of times the function being executed. NOTE: this execution time doesn't affect the default logic.

The "camera" library
--------------------

### Static functions:

* setMode(mode) -- set the camera mode, which is "player" or "shadow"

* lookAt(x,y) -- set the camera mode to "custom" and set the new center of camera

The "audio" library
-------------------

NOTE: the following functions are not going to work if the sound/music volume is 0.

### Static functions:

* playSound(name[,concurrent=-1[,force=false[,fade=-1]]])

Play a sound effect.

Argument name | Description
--------------|-------------
name          | The name of the sound effect. Currently available: "jump", "hit", "checkpoint", "swap", "toggle", "error", "collect", "achievement".
concurrent    | The number of times the same sfx can be played at once, -1 is unlimited. NOTE: there are 64 channels.
force         | If the sound must be played even if all channels are used. In this case the sound effect in the first channel will be stopped.
fade          | A factor to temporarily turn the music volume down (0-128). -1 means don't use this feature.

Return value: The channel of the sfx. -1 means failed (channel is full, invalid sfx name, sfx volume is 0, etc.)

* playMusic(name[,fade=true])

Play a music.

Argument name | Description
--------------|-------------
name          | The name of the song, e.g. "default/neverending" or "menu".
fade          | Boolean if it should fade the current one out or not.

* pickMusic() -- pick a song from the current music list.

* getMusicList()/setMusicList(name_of_the_music_list) -- get/set the music list. Example: "default".

* currentMusic() -- get the current music.

The "gettext" library
--------------------

This library is used for translation support.

NOTE: Currently this library only uses the dictionary for current level pack.
This means it doesn't work for individual level which doesn't contain in a level pack,
and it can't make use of the translations of the core game.

### Global functions:

* `_(msgid)` -- translate the string using default context

* `__(msgid)` -- does nothing, just outputs the original string. However, it will be scanned by `xgettext`.
  Mainly used in block:setMessage() since the block message will always be passed to gettext().
  Also used in construction of an array of strings, which will be translated dynamically.

### Static functions:

* gettext(msgid) -- translate the string using default context

* pgettext(msgctxt,msgid) -- translate the string using specified context

* ngettext(msgid,msgid_plural,n) -- translate the string using default context, taking plural form into consideration

* npgettext(msgctxt,msgid,msgid_plural,n) -- translate the string using specified context, taking plural form into consideration

The "prng" library
--------------------

The Mersenne Twister 19937 pseudo-random number generator.

The random seed is recreated each time the game starts, and is saved to the record file,
which ensures the reproducibility of the replay.

### Static functions:

* random() / random(n) / random(m,n)

These functions have the same arguments as the Lua built-in function `math.random()`. More precisely:

When called without arguments, returns a pseudo-random float with uniform distribution in the range `[0,1)`.

When called with two integers `m` and `n`, returns a pseudo-random integer with uniform distribution in the range `[m, n]`.
The `m` cannot be larger than `n`
(othewise it will return a pseudo-random integer with uniform distribution in the union of `[m, 2^63-1]` and `[-2^63, n]`)
and must fit in two Lua integers.

The call `random(n)` is equivalent to `random(1,n)`.

* getSeed() / setSeed(string)

Get or set the random seed, which is a string.
This is mainly used when you want the pseudo-random number to be reproducible even between each plays.

NOTE: This should only contains alphanumeric characters. Other characters will be filtered out.
