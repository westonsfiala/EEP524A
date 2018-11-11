__kernel void SobelGrad(
__read_only image2d_t inputImg, 
__write_only image2d_t outputImg,
const sampler_t sampler,
__global float* horizontalFilter,
__global float* verticalFilter,
const int filterWidth,
const int rows,
const int columns,
const float theta
)
{
    // use global IDs for output coords
    int x = get_global_id(0); // columns
    int y = get_global_id(1); // rows
    int halfWidth = (int)(filterWidth/2); // auto-round nearest int ???
    float4 sum1 = (float4)(0);
    float4 sum2 = (float4)(0);
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
            
            printf("%f %f %f %f\n", pixel.s0, pixel.s1, pixel.s2, pixel.s3);
            sum1 += pixel * (float4)(horizontalFilter[filtIdx],horizontalFilter[filtIdx],horizontalFilter[filtIdx],1.0f); // leave a-channel unchanged
            sum2 += pixel * (float4)(verticalFilter[filtIdx],verticalFilter[filtIdx],verticalFilter[filtIdx],1.0f); // leave a-channel unchanged
        }
    }
    //write resultant filtered pixel to output image
    coords = (int2)(x,y);

    //float4 sum1Square = pown(sum1,2);
    //float4 sum2Square = pown(sum2,2);

    //float4 sumSqrt = sqrt(sum1Square + sum2Square);

    float4 sumAbs = fabs(sum1) + fabs(sum2);

    printf("%f %f %f %f\n", sumAbs.s0, sumAbs.s1, sumAbs.s2, sumAbs.s3);

    //write_imageui(outputImg, coords, convert_uint4(sum));
    write_imagef(outputImg, coords, sumAbs);
}
