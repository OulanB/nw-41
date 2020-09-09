# nw-41
HP 41 emulator for numworks with Omega 1.20 as external app

- full HP41CX emulated
- save state when leave (HOME) in a 8KB variable
- clock module is ok for stopwatch, but real-time is not actually possible.
  Next Omega relase will have a real-time part, so it could be used if this
  part is integrated in the external api (extapp_timedate)
- key mapping is illustrated in Docs
- ED key mapping is not tested at all ;)

LCD part should be rewritten : 
 Do not move data in array !! instead use a pointer to keep the current index ;)
