#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

int main()
{
    @autoreleasepool {

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();

        printf("GPU: %s\n", device.name.UTF8String);

        NSError *error = nil;

        // Load metallib
        id<MTLLibrary> library =
            [device newLibraryWithFile:@"add.metallib"
                                 error:&error];

        if (!library) {
            NSLog(@"Library error: %@", error);
            return -1;
        }

        id<MTLFunction> function =
            [library newFunctionWithName:@"add_arrays"];


        id<MTLComputePipelineState> pipeline =
            [device newComputePipelineStateWithFunction:function
                                                    error:&error];


        if (!pipeline) {
            NSLog(@"Pipeline error: %@", error);
            return -1;
        }


        // Input arrays
        float A[4] = {1,2,3,4};
        float B[4] = {10,20,30,40};
        float result[4] = {0};


        id<MTLBuffer> bufferA =
            [device newBufferWithBytes:A
                                length:sizeof(A)
                               options:MTLResourceStorageModeShared];


        id<MTLBuffer> bufferB =
            [device newBufferWithBytes:B
                                length:sizeof(B)
                               options:MTLResourceStorageModeShared];


        id<MTLBuffer> bufferResult =
            [device newBufferWithLength:sizeof(result)
                                options:MTLResourceStorageModeShared];


        // Command queue
        id<MTLCommandQueue> queue =
            [device newCommandQueue];


        id<MTLCommandBuffer> commandBuffer =
            [queue commandBuffer];


        id<MTLComputeCommandEncoder> encoder =
            [commandBuffer computeCommandEncoder];


        [encoder setComputePipelineState:pipeline];

        [encoder setBuffer:bufferA offset:0 atIndex:0];
        [encoder setBuffer:bufferB offset:0 atIndex:1];
        [encoder setBuffer:bufferResult offset:0 atIndex:2];


        MTLSize gridSize = MTLSizeMake(4,1,1);

        MTLSize threadGroupSize =
            MTLSizeMake(4,1,1);


        [encoder dispatchThreads:gridSize
           threadsPerThreadgroup:threadGroupSize];


        [encoder endEncoding];


        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];


        float *output =
            bufferResult.contents;


        printf("Results:\n");

        for(int i=0;i<4;i++)
        {
            printf("%f\n", output[i]);
        }
    }

    return 0;
}