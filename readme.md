# HostPluginDemo-cmake
This is a CMake port of JUCE's [HostPluginDemo](https://github.com/juce-framework/JUCE/blob/master/examples/Plugins/HostPluginDemo.h), which, to my knowledge, is only available as a JUCE [PIP](https://forum.juce.com/t/what-is-a-pip/26821).  
Unlike the original HostPluginDemo, this plugin actually implements audio processing  
I've also added comments to places that I think could be confusing  
Fair warning: I'm very new to developing with JUCE, so the chances that you'll find some non-idiomatic or otherwise weird code here are high

## Build 
```shell
git clone --recurse https://github.com/original-picture/HostPluginDemo-cmake
cd HostPluginDemo-cmake
mkdir build 
cmake -B build -S .
cmake --build .
```

## Bugs
- [x] indicates a fixed bug  


- [ ] for some reason you have to click the open plugin button twice in order for it to work. 
  And then when you close the plugin, another one will pop up and you'll have to close that one too  
  Most likely an issue with the ping-ponging system

## Todo
- [x] actually have the hosted plugin do the audio processing
- [x] add an error dialog box if something goes wrong when trying to load a plugin 
- 

## Code style
The code contains a mix of `camelCase` and `snake_case`. all of JUCE's code uses `camelCase` so I use `snake_case` to make it clear which parts are written by me  
Also I just prefer `snake_case` 

