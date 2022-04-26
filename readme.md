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
   
Session 04:
	Because builds were failing, needed to find a solution to a Linker error in Visual Studio after doing the 
	"makePeakFilter()" method updates in the tutorial.

	Found a solution here: (use the inline keyword)
	https://stackoverflow.com/questions/1240634/how-to-get-rid-of-warning-lnk4006-when-not-using-templates
	
	after this page
	https://stackoverflow.com/questions/3705740/c-lnk2019-error-unresolved-external-symbol-template-classs-constructor-and
	suggested the body of the function needed to move from .cpp to .h (but still got same Linker errors killing builds), 
	it was easy to add inline keyword above the definition in PluginProcesor.h 
	
	Need to figure out why the "final" build step does not automatically copy the "vst3 folder" anywhere on my machine.
	When AudioPluginHost has the VST3 open, VisualStudio can't currently finish that build.