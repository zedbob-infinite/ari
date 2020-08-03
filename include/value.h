#ifndef ari_value_h
#define ari_value_h

#define ALLOCATE_STRING_VAL(val, value)     (val->val_string = malloc(sizeof(char) * strlen(value)))

#define OBJ_VAL(object)         ((value) VAL_OBJ, {.obj = (Obj*)object})
#define BOOL_VAL(val, value)    ((value){ VAL_BOOL, {.int = value}})
#define NULL_VAL(val)           ((value){ VAL_NULL, {.int = 0}})
#define NUMBER_VAL(val, value)  ((value){ VAL_NUMBER, {.number = value}})

#define AS_BOOL(val)                        (val->val_int)
#define AS_NUMBER(val)                      (val->val_number)
#define AS_NULL(val)                        (val->val_int)
#define AS_STRING(val)                      (val->val_string)

typedef enum valtype_t
{
    VAL_NUMBER,
    VAL_STRING,
    VAL_BOOL,
    VAL_NULL
} valtype;

typedef struct value_t
{
    valtype type;
    union
    {
        int val_int;
        double val_number;
        char *val_string;
    //    object *obj;
    };
} value;

value *create_new_value(valtype type);
void print_value(value *val);
#endif
