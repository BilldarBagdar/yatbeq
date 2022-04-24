Yet Another Three Band EQ

based on following Chuck's free code camp C++ for musicians video

Session 01:
stopping at 41:15 in video    
   need to find an audio file player vst to load on this dev machine (to run in the AudioPluginHost program)
   I don't have any actual VST things installed on this machine.
   
   tried installing Bidule because it has an audio file player, but Bidule doesn't load this VST3?

Session 02:   
   because I have a license for Bidule I can run the VST3 version inside AudioPluginHost.  
   Inside Bidule vst, the Audio File Player bidule outputs to the Level Meter bidule which outputs to the vst audio outputs
   
   Also figured out how to attach visual studio to the AudioPluginHost (running) process for debugging.
   But it is still a two step process?  Need to manually start and shutdown AudioPluginHost before and after every 
   debug session?  Want to figure out how to automate the steps.  Build, start AudioPluginHost from save file, attach, debug, 
   detach, stop AudioPluginHost.