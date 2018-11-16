__kernel void img_conv_filter(
__read_only image2d_t inputImg, 
__write_only image2d_t outputImg, 
const sampler_t sampler,
__global const float* filter,
const int filterWidth
)
{
    // use global IDs for output coords
    int x = get_global_id(0);
    int y = get_global_id(1);

    // Floor to an int.
    int halfWidth = (int)(filterWidth/2); 

    float4 sum = (float4)(0);

    // filter kernel passed in as linearized buffer array
    int filtIdx = 0; 
    int2 coords;

    // iterate filter rows
    for(int i = -halfWidth; i <= halfWidth; i++) 
    {
        coords.y = y + i;
        // iterate filter cols
        for(int j = -halfWidth; j <= halfWidth; j++) 
        {
            coords.x = x + j;
            float filterVal = filter[filtIdx];

            // operate element-wise on all 3 color components (r,g,b)
            float4 pixel = convert_float4(read_imageui(inputImg, sampler, coords)); 
            // leave a-channel unchanged
            sum += pixel * (float4)(filterVal,filterVal,filterVal,filterVal); 

            filtIdx++;
        }
    }
    //write resultant filtered pixel to output image
    coords = (int2)(x,y);

    uint4 sumUint = convert_uint4((float4)(sum.x,sum.y,sum.z,sum.w));
    //printf("%i\n", sumUint.w);
    write_imageui(outputImg, coords, sumUint); 
    //write_imagef(outputImg, coords, sum);
}
