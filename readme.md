# HostPluginDemo-cmake
This is a CMake port of JUCE's [HostPluginDemo](https://github.com/juce-framework/JUCE/blob/master/examples/Plugins/HostPluginDemo.h), which, to my knowledge, is only available as a JUCE [PIP](https://forum.juce.com/t/what-is-a-pip/26821)  
Unlike the original HostPluginDemo, this plugin actually implements audio processing  
I've also added comments explaining parts of the code that I found confusing  
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
- [x] *indicates a fixed bug*


- [x] for some reason you have to click the open plugin button twice in order for it to work. 
  And then when you close the plugin, another one will pop up and you'll have to close that one too  
  - Most likely an issue with the ping-ponging system
- [x] segfault when closing a plugin 
- [x] after close plugin is clicked, the plugin closes but another "close plugin" button shows up in the host plugin window that needs to be clicked before the user can return to the plugin list
- [x] changes made in the inner plugin's editor don't seem to mark the project as "dirty" in reaper, meaning reaper will close without an unsaved work dialog, and any changes made to the inner plugin will be lost
- note that everything is saved and restored correctly if the user manually saves the reaper project
- [apparently this is an issue with juce/the underlying plugin libraries](https://forum.juce.com/t/how-the-plugin-can-tell-to-the-host-the-project-state-became-dirty/33830/20)
  - I'll probably have to add a dummy parameter
- interestingly though, the project isn't marked as dirty even if the outer plugin's parameters have visibly changed
  - I think I'm missing a call to `setValueNotifyingHost()`
    - will probably have to use change listeners here
- [ ] mysterious freeze on plugin load
- [ ] JUCE assert fails when `HostPluginDemo-cmake` is hosting itself and the user presses the outermost close button
- [x] The behavior of the UI button in Reaper is confusing. It will sometimes reopen a closed inner plugin editor and doesn't seem to reopen an inner plugin editor that *was* open
  - after doing more testing, I've realized that this behavior isn't random. It oscillates between opening the editor and not opening the editor
  - interestingly, this behavior doesn't occur when closing and reopening the entire DAW
    - perhaps there's a way to exploit this and achieve the same behavior without having to add a bunch of manual checks  
  - also if you disable and then re-enable the gui with an inner plugin that has no editor, the gui of the last loaded plugin that *did* have an editor will open instead
- [ ] a juce assert fails when using the 'X' in the upper right corner to close the host plugin when the inner plugin is running in the same window as the host plugin
- [x] after plugin initialization, in reaper, all forwarded parameters sliders are all the way to the left, regardless of what the parameter's value is
  - moving a slider will change its value to be in sync with the position of the slider
  - fixed by calling `setValueNotifyingHost` in `forwarding_parameter_ptr::set_forwarded_parameter()`
- [ ] incorrect juce "no compatible plugin format exists" error message when reloading the plugin on linux
- [ ] segfault when loading certain plugins on linux (calf compressor)
- [ ] `jassert` failure on application exit on linux ([juce_LeakedObjectDetector.h](./lib/JUCE/modules/juce_core/memory/juce_LeakedObjectDetector.h) line 104)
- [ ] VST3 `kNotInitialized` errors when loading a plugin (linux)


## Todo
- [x] actually have the hosted plugin do the audio processing
- [x] add an error dialog box if something goes wrong when trying to load a plugin 
- [x] parameter forwarding 
- [x] use native window decorations when the inner plugin is running in its own top level window
  - [x] remove the redundant juce close button when the plugin is running in a different window
  - [ ] add icons for native window decorations

- [ ] be more flexible about supporting different bus layouts
- [ ] add a data loss warning when closing a plugin? Or maybe just use a unique save file for every plugin so nothing gets lost?
- [ ] give `forwarding_parameter_ptr` the ability to take a `get_name_string` callback that take in the original name string and outputs some other name string
  - would be useful for creating parameter "namespaces"
    - `gain_plugin_instance_0.gain`
    - `gain_plugin_instance_1.gain`
    - etc.
  
- [ ] suppress warnings that should only show up when a plugin is first loaded when a plugin is reloaded (e.g., because the DAW was closed and reopened)
- [ ] add support for running the inner plugin as a native ~~child~~ owned window, instead of just using juce's `setAlwaysOnTop()`
  * windowing system support:
    - [x] windows (does the windowing system of windows even have a formal name?)
      * Minimizing the host plugin window when the inner plugin window is [active](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features#:~:text=An%20active%20window%20is%20the%20top%2Dlevel%20window%20of%20the%20application%20with%20which%20the%20user%20is%20currently%20working.%20To%20allow%20the%20user%20to%20easily%20identify%20the%20active%20window%2C%20the%20system%20places%20it%20at%20the%20top%20of%20the%20z%2Dorder%20and%20changes%20the%20color%20of%20its%20title%20bar%20and%20border%20to%20the%20system%2Ddefined%20active%20window%20colors) causes the host window to minimize without minimizing the inner plugin window. 
        This might just be a quirk of the windows API
        * this doesn't seem to occur in reaper (only in juce's `AudioPluginHost`) so maybe this isn't an issue in practice?
    - [x] X11
    - [ ] macos (Quartz? NSWindow? You know what I mean lol)

## Code style
The code contains a mix of `camelCase` and `snake_case`. all of JUCE's code uses `camelCase` so I use `snake_case` to make it clear which parts are written by me  
Also I just prefer `snake_case`  
I sign all of my comments with `--original-picture`. Unsigned ones are from the JUCE people

