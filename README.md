# Robtop Bot Playback

This code **only** enables playback and recording for the internal record string format. This **will** break playability of Geometry Dash, and there is no intent on fixing that. This repository is meant to be an "light" form of using this format, and as such no GUI interfaces have been made for the purpose of using this.

This extension does interact with save data, but only to set the `recordString` of the `GJGameLevel`.

# Usage

1. Build DLL.
2. Put DLL into directory of Geometry Dash and run game.
3. Play a level, exit it, and attempt to play it again.

# Questions

## Why?

This is to provide a more clear view of how RobTop's internal rating tools look, including the replay functionality.

## How do I delete a replay/get out of playback mode?

You don't.

## How would one use this for a GDPS?

1. Add a GUI interface.
2. Tell the GDPS developer to add the `levelInfo` parameter to index 26 in `downloadGJLevel.php`.

## Why is this so inaccurate?

* RobTop.

# Credits

* Andre for [CappuccinoSDK](https://github.com/AndreNIH/CappuccinoSDK) which saved 30 minutes of cocos2d-x header making.
* Wylie for asking about record format and 2.1.
