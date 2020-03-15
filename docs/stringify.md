The header file [types.h](../inc/cutils/types.h) provides macros to help stringify your enums. ie. provide helper functions for enums declared appropriately that will return string representations of enums given their values. This is useful to create descriptive logs.

## Example use case
Assume we want to create an enum of the following form
```
typedef enum {
  eRed = 1,
  eGreen = 2,
  eBlue 
}color_e;
```

Instead of the above definition use the following 

```
#include <types.h>

#define COLOR_E(XX)\
  XX(eRed,= 1)\
  XX(eGreen, = 2)\
  XX(eBlue,)

DECLARE_ENUM(color_e, COLOR_E);
```

The above will create the expected enum type `color_e`. In addition two static inline helper functions will come into scope:
* `const char* get_color_e_string(color_e value)` - Given an enum value will return the string representation. So if we gave it a value of 1, we would get the string "eRed". 
* `color_e get_color_e_value(const char* string)`- which will return the enum value for a matching string. ie. given a string "eRed", it will return 1.

## Caveats
You can only use this for enums whose values are unique. The moment you create an enum entry with the same value as another entry in the same enum, the compiler will complain about an ambiguous switch statement. 

 