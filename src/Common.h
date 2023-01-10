#ifdef DEBUG_MODE
#define DEBUG(val) Serial.print(val)
#define DEBUGLN(val) Serial.println(val)
#define DEBUG_FLUSH() Serial.flush()
#define DEBUGF Serial.printf
#else
#define DEBUG(val)
#define DEBUGLN(val)
#define DEBUG_FLUSH()
#define DEBUGF(...)
#endif
