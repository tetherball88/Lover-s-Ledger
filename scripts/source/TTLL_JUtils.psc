scriptname TTLL_JUtils

import TTLL_JCDomain

;/
  Helper to get a JMap object, creating it if it doesn't exist.
  @param parentMap The parent JMap object
  @param k         The key to look up or create
  @return          The JMap object
/;
int Function _GetOrCreateJMap(int parentMap, string k) global
    int res = JMap_getObj(parentMap, k)
    if (!res)
        res = JMap_object()
        JMap_setObj(parentMap, k, res)
    endif
    return res
EndFunction

;/
  Helper to get a JArray object, creating it if it doesn't exist.
  @param parentMap The parent JMap object
  @param k         The key to look up or create
  @return          The JArray object
/;
int Function _GetOrCreateJArray(int parentMap, string k) global
    int res = JMap_getObj(parentMap, k)
    if (!res)
        res = JArray_object()
        JMap_setObj(parentMap, k, res)
    endif
    return res
EndFunction

;/
  Helper to get a JFormMap object, creating it if it doesn't exist.
  @param parentMap The parent JMap object
  @param k         The key to look up or create
  @return          The JFormMap object
/;
int Function _GetOrCreateJFormMap(int parentMap, string k) global
    int res = JMap_getObj(parentMap, k)
    if (!res)
        res = JFormMap_object()
        JMap_setObj(parentMap, k, res)
    endif
    return res
EndFunction

;/**
  * Increments an integer property in a JMap object.
  * @param {int} JObj - The JMap object.
  * @param {string} propName - The property name to increment.
  * @returns {int} - The new value of the property.
*/; 
int Function JMapIntIncrement(int JObj, string propName, int incr = 1) global
    int newVal = JMap_getInt(JObj, propName) + incr
    JMap_setInt(JObj, propName, newVal)
    return newVal
EndFunction