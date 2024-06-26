#ifndef __APP_GLOBAL_MACRO_H__
#define __APP_GLOBAL_MACRO_H__

/* If the use case is literally just printing the enum name macro */
#define str(x) #x
#define xstr(x) str(x)

/* expansion macro for enum value definition */
#define ENUM_VALUE_SUM(name, assign) name +

/* expansion macro for enum value & assign definition */
#define ENUM_VALUE_ASSIGN(name, assign) name assign,

/* expansion macro for enum to string conversion */
#define ENUM_CASE(name, assign) \
	case name:              \
		return #name;

/* declare the access function and define enum values */
#define DECLARE_ENUM(EnumName, ENUM_DEF)                             \
	enum EnumName {                                              \
		ENUM_DEF(ENUM_VALUE_ASSIGN)                          \
			EnumName##_sum = ENUM_DEF(ENUM_VALUE_SUM) 0, \
	};

/* define the access function names */
#define DEFINE_ENUM(EnumType, ENUM_DEF)                     \
	static const char *enum_to_str(enum EnumType value) \
	{                                                   \
		switch (value) {                            \
			ENUM_DEF(ENUM_CASE)                 \
		default:                                    \
			return ""; /* handle input error */ \
		}                                           \
	}

/* declare & define enum macro */
#define CREATE_ENUM(EnumType, ENUM_DEF)  \
	DECLARE_ENUM(EnumType, ENUM_DEF) \
	DEFINE_ENUM(EnumType, ENUM_DEF)

/* Code for concise writing of if-else statements */
#define CODE_IF_ELSE(enable, code1, code2) \
	if (enable == 1) {                 \
		code1;                     \
	} else if (enable == 0) {          \
		code2;                     \
	}
#endif // __APP_GLOBAL_MACRO_H__
