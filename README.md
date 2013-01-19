A standalone library to use the Razer Hydra, based on the findings of 
Ryan Pavlik (https://github.com/rpavlik/razer-hydra-hid-protocol) 
and his code from the VRPN software (http://www.cs.unc.edu/Research/vrpn/)

This was done because there was not a dedicated API for the Hydra, and the Sixense API
was generic for the Sixense products, starting 4 threads as that is the max number of 
controller bases they support (as the Hydra can not change its magnetic frequency, 
only has sense to plug just one). 

I've tried to maintain the code as portable as possible, but I've only tested it on Linux.

Lerna requires the hidapi library.
The test example is done using GLEW and GLFW.
