__kernel void img_conv_filter(
__read_only image2d_t inputImg, 
__write_only image2d_t outputImg, 
const sampler_t sampler,
__global const float* filter,
const int filterWidth
)
{
    // use global IDs for output coords
    int x = get_global_id(0); // columns
    int y = get_global_id(1); // rows
    int halfWidth = (int)(filterWidth/2); // auto-round nearest int ???
    float4 sum = (float4)(0);
    int filtIdx = 0; // filter kernel passed in as linearized buffer array
    int2 coords;
    for(int i = -halfWidth; i <= halfWidth; i++) // iterate filter rows
    {
        coords.y = y + i;
        for(int j = -halfWidth; j <= halfWidth; j++) // iterate filter cols
        {
            coords.x = x + j;
            //float4 pixel = convert_float4(read_imageui(inputImg, sampler, coords)); // operate element-wise on all 3 color components (r,g,b)
            float4 pixel = read_imagef(inputImg, sampler, coords); // operate element-wise on all 3 color components (r,g,b)
            filtIdx++;
            sum += pixel * (float4)(filter[filtIdx],filter[filtIdx],filter[filtIdx],1.0f); // leave a-channel unchanged
        }
    }
    //write resultant filtered pixel to output image
    coords = (int2)(x,y);
    //write_imageui(outputImg, coords, convert_uint4(sum));
    write_imagef(outputImg, coords, sum);
}
