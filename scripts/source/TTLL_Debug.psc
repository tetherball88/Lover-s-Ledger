scriptname TTLL_Debug

Function log(string msg) global
    MiscUtil.PrintConsole(msg)
EndFunction

Function trace(string msg) global
    log("[ll(trace)]: " + msg)
EndFunction

Function debug(string msg) global
    log("[ll(debug)]: " + msg)
EndFunction

Function info(string msg) global
    log("[ll(info)]: " + msg)
EndFunction

Function warn(string msg) global
    log("[ll(warn)]: " + msg)
EndFunction

Function err(string msg) global
    log("[ll(err)]: " + msg)
EndFunction