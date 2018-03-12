This demo client is for a Windows client communicating with a Linux
server running Predict. You need the VB development environment. (Note:
this was developed under VB6, but other versions may handle it OK)

Open a new project, and add frmWinPredictClient to it, and also set this
as the Projects Startup object. You'll also need the Microsoft Winsock
Control selected in Project/Components. Then click Run (or Compile if
you wish).

Note: the command line may be set to contain the name of the Predict host,
in which case the program will not prompt for that name, but immediately
try to get the satellite list.

This was developed by Steve Fraser, vk5asf (sfraser AT sierra.apana.org.au)

