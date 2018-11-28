kernel void outputGlobal (
    global int* output
)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int z = get_global_id(2);

    int xMax = get_global_size(0);
    int yMax = get_global_size(1);
    int zMax = get_global_size(2);

    int index = z * xMax * yMax + y * xMax + x;
    int value = z << 16 | y << 8 | x;

    output[index] = value;
}