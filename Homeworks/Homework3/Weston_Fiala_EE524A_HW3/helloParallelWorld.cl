union my_union { 
    float f;
    short s;
    char c;
};

struct my_struct { 
    char my_char;
    char4 my_char4;
    union my_union u;
    uint2 my_uint2_vector[4];
};

__kernel void helloParallelWorld(
    const float3 my_float3,
    const float4 my_float4,
    const float8 my_float8,
    __global const struct my_struct* my_struct_ptr
)
{ 
    // Get and print the global IDs.
    int x_global_id = get_global_id(0);
    int y_global_id = get_global_id(1);

    printf("Hello Parallel World from Global X:%d Y:%d\n", x_global_id, y_global_id);

    // Get and print the local IDs.
    int x_local_id = get_local_id(0);
    int y_local_id = get_local_id(1);

    printf("\tLocal ID X:%d Y:%d\n", x_local_id, y_local_id);

    // Print the contents of the float4 vector.
    printf("\tfloat4 contents: %f %f %f %f\n", my_float4.x, my_float4.y, my_float4.z, my_float4.w );

    // Print the contents of the float4 vector in reverse.
    printf("\tfloat4 contents reversed: %f %f %f %f\n", my_float4.w, my_float4.z, my_float4.y, my_float4.x);

    // Print the contents of the float8 vector high then low.
    printf("\tfloat8 contents high: %f %f %f %f\n", my_float8.hi.x, my_float8.hi.y, my_float8.hi.z, my_float8.hi.w);
    printf("\tfloat8 contents low: %f %f %f %f\n", my_float8.lo.x, my_float8.lo.y, my_float8.lo.z, my_float8.lo.w);
    
    // Print the contents of the float8 vector even then odd.
    printf("\tfloat8 contents even: %f %f %f %f\n", my_float8.even.x, my_float8.even.y, my_float8.even.z, my_float8.even.w);
    printf("\tfloat8 contents odd: %f %f %f %f\n", my_float8.odd.x, my_float8.odd.y, my_float8.odd.z, my_float8.odd.w);

    // Print the contents of the structure members.
    printf("\tstruct contents:\n");
    printf("\t\tchar: %c\n", my_struct_ptr->my_char);
    printf("\t\tchar4: %c %c %c %c\n", my_struct_ptr->my_char4.x, my_struct_ptr->my_char4.y, my_struct_ptr->my_char4.z, my_struct_ptr->my_char4.w);
    printf("\t\tunion Contents:\n");
    printf("\t\t\tfloat: %f\n", my_struct_ptr->u.f);
    printf("\t\t\tshort: %d\n", my_struct_ptr->u.s);
    printf("\t\t\tchar: %c\n", my_struct_ptr->u.c);
    printf("\t\tuint2 vector contents:\n");
    printf("\t\t\tuint2[0]: %d %d\n", my_struct_ptr->my_uint2_vector[0].s0, my_struct_ptr->my_uint2_vector[0].s1);
    printf("\t\t\tuint2[1]: %d %d\n", my_struct_ptr->my_uint2_vector[1].s0, my_struct_ptr->my_uint2_vector[1].s1);
    printf("\t\t\tuint2[2]: %d %d\n", my_struct_ptr->my_uint2_vector[2].s0, my_struct_ptr->my_uint2_vector[2].s1);
    printf("\t\t\tuint2[3]: %d %d\n", my_struct_ptr->my_uint2_vector[3].s0, my_struct_ptr->my_uint2_vector[3].s1);

    // Print the sizeof of the entire struct.
    printf("\tsizeof entire struct: %d\n", sizeof(*my_struct_ptr));

    // Print the sizeof sum of the individual struct elements.
    int struct_element_size = 0;
    struct_element_size += sizeof(char);
    struct_element_size += sizeof(char4);
    struct_element_size += sizeof(union my_union);
    struct_element_size += sizeof(uint2) * 4;
    printf("\tsum of sizeof individual struct elements : %d\n", struct_element_size);

    // Print the sizeof of the union.
    printf("\tsizeof union element in struct : %d\n", sizeof(union my_union));

    // Print the sizeof of the union subelement u.c.
    printf("\tsizeof union element c in union in struct : %d\n", sizeof(char));
}
