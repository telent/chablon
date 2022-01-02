
typedef unsigned int u32;

struct test1 {
    char * john;
    int paul;
    double george;
    u32 ringo;
} instance_test1;

struct anon_nested_struct {
    char * john;
    struct {
	int len;
	char buf[32];
    } ;
    double george;
    u32 ringo;
} instance_anon_nested_struct;
