/* ============================================================

Copyright (c) 2009 Advanced Micro Devices, Inc.  All rights reserved.
 
Redistribution and use of this material is permitted under the following 
conditions:
 
Redistributions must retain the above copyright notice and all terms of this 
license.
 
In no event shall anyone redistributing or accessing or using this material 
commence or participate in any arbitration or legal action relating to this 
material against Advanced Micro Devices, Inc. or any copyright holders or 
contributors. The foregoing shall survive any expiration or termination of 
this license or any agreement or access or use related to this material. 

ANY BREACH OF ANY TERM OF THIS LICENSE SHALL RESULT IN THE IMMEDIATE REVOCATION 
OF ALL RIGHTS TO REDISTRIBUTE, ACCESS OR USE THIS MATERIAL.

THIS MATERIAL IS PROVIDED BY ADVANCED MICRO DEVICES, INC. AND ANY COPYRIGHT 
HOLDERS AND CONTRIBUTORS "AS IS" IN ITS CURRENT CONDITION AND WITHOUT ANY 
REPRESENTATIONS, GUARANTEE, OR WARRANTY OF ANY KIND OR IN ANY WAY RELATED TO 
SUPPORT, INDEMNITY, ERROR FREE OR UNINTERRUPTED OPERA TION, OR THAT IT IS FREE 
FROM DEFECTS OR VIRUSES.  ALL OBLIGATIONS ARE HEREBY DISCLAIMED - WHETHER 
EXPRESS, IMPLIED, OR STATUTORY - INCLUDING, BUT NOT LIMITED TO, ANY IMPLIED 
WARRANTIES OF TITLE, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, 
ACCURACY, COMPLETENESS, OPERABILITY, QUALITY OF SERVICE, OR NON-INFRINGEMENT. 
IN NO EVENT SHALL ADVANCED MICRO DEVICES, INC. OR ANY COPYRIGHT HOLDERS OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, REVENUE, DATA, OR PROFITS; OR 
BUSINESS INTERRUPTION) HOWEVER CAUSED OR BASED ON ANY THEORY OF LIABILITY 
ARISING IN ANY WAY RELATED TO THIS MATERIAL, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE. THE ENTIRE AND AGGREGATE LIABILITY OF ADVANCED MICRO DEVICES, 
INC. AND ANY COPYRIGHT HOLDERS AND CONTRIBUTORS SHALL NOT EXCEED TEN DOLLARS 
(US $10.00). ANYONE REDISTRIBUTING OR ACCESSING OR USING THIS MATERIAL ACCEPTS 
THIS ALLOCATION OF RISK AND AGREES TO RELEASE ADVANCED MICRO DEVICES, INC. AND 
ANY COPYRIGHT HOLDERS AND CONTRIBUTORS FROM ANY AND ALL LIABILITIES, 
OBLIGATIONS, CLAIMS, OR DEMANDS IN EXCESS OF TEN DOLLARS (US $10.00). THE 
FOREGOING ARE ESSENTIAL TERMS OF THIS LICENSE AND, IF ANY OF THESE TERMS ARE 
CONSTRUED AS UNENFORCEABLE, FAIL IN ESSENTIAL PURPOSE, OR BECOME VOID OR 
DETRIMENTAL TO ADVANCED MICRO DEVICES, INC. OR ANY COPYRIGHT HOLDERS OR 
CONTRIBUTORS FOR ANY REASON, THEN ALL RIGHTS TO REDISTRIBUTE, ACCESS OR USE 
THIS MATERIAL SHALL TERMINATE IMMEDIATELY. MOREOVER, THE FOREGOING SHALL 
SURVIVE ANY EXPIRATION OR TERMINATION OF THIS LICENSE OR ANY AGREEMENT OR 
ACCESS OR USE RELATED TO THIS MATERIAL.

NOTICE IS HEREBY PROVIDED, AND BY REDISTRIBUTING OR ACCESSING OR USING THIS 
MATERIAL SUCH NOTICE IS ACKNOWLEDGED, THAT THIS MATERIAL MAY BE SUBJECT TO 
RESTRICTIONS UNDER THE LAWS AND REGULATIONS OF THE UNITED STATES OR OTHER 
COUNTRIES, WHICH INCLUDE BUT ARE NOT LIMITED TO, U.S. EXPORT CONTROL LAWS SUCH 
AS THE EXPORT ADMINISTRATION REGULATIONS AND NATIONAL SECURITY CONTROLS AS 
DEFINED THEREUNDER, AS WELL AS STATE DEPARTMENT CONTROLS UNDER THE U.S. 
MUNITIONS LIST. THIS MATERIAL MAY NOT BE USED, RELEASED, TRANSFERRED, IMPORTED,
EXPORTED AND/OR RE-EXPORTED IN ANY MANNER PROHIBITED UNDER ANY APPLICABLE LAWS, 
INCLUDING U.S. EXPORT CONTROL LAWS REGARDING SPECIFICALLY DESIGNATED PERSONS, 
COUNTRIES AND NATIONALS OF COUNTRIES SUBJECT TO NATIONAL SECURITY CONTROLS. 
MOREOVER, THE FOREGOING SHALL SURVIVE ANY EXPIRATION OR TERMINATION OF ANY 
LICENSE OR AGREEMENT OR ACCESS OR USE RELATED TO THIS MATERIAL.

NOTICE REGARDING THE U.S. GOVERNMENT AND DOD AGENCIES: This material is 
provided with "RESTRICTED RIGHTS" and/or "LIMITED RIGHTS" as applicable to 
computer software and technical data, respectively. Use, duplication, 
distribution or disclosure by the U.S. Government and/or DOD agencies is 
subject to the full extent of restrictions in all applicable regulations, 
including those found at FAR52.227 and DFARS252.227 et seq. and any successor 
regulations thereof. Use of this material by the U.S. Government and/or DOD 
agencies is acknowledgment of the proprietary rights of any copyright holders 
and contributors, including those of Advanced Micro Devices, Inc., as well as 
the provisions of FAR52.227-14 through 23 regarding privately developed and/or 
commercial computer software.

This license forms the entire agreement regarding the subject matter hereof and 
supersedes all proposals and prior discussions and writings between the parties 
with respect thereto. This license does not affect any ownership, rights, title,
or interest in, or relating to, this material. No terms of this license can be 
modified or waived, and no breach of this license can be excused, unless done 
so in a writing signed by all affected parties. Each term of this license is 
separately enforceable. If any term of this license is determined to be or 
becomes unenforceable or illegal, such term shall be reformed to the minimum 
extent necessary in order for this license to remain in effect in accordance 
with its terms as modified by such reformation. This license shall be governed 
by and construed in accordance with the laws of the State of Texas without 
regard to rules on conflicts of law of any state or jurisdiction or the United 
Nations Convention on the International Sale of Goods. All disputes arising out 
of this license shall be subject to the jurisdiction of the federal and state 
courts in Austin, Texas, and all defenses are hereby waived concerning personal 
jurisdiction and venue of these courts.

============================================================ */


#include "AESEncryptDecrypt.hpp"

using namespace AES;


void AESEncryptDecrypt::write_pkt_offset(cl_uint *pkt_offset)
{
	int i = 0;
	cl_uint offset = 0;
	for (i = 0; i < num_flows; i ++) {
		pkt_offset[i] = offset;
		offset += flow_len;
	}
}

void AESEncryptDecrypt::set_random(cl_uchar *input, int len)
{
	int i;
	for (i = 0; i < len; i ++)
		input[i] = rand() % 256;
}

int AESEncryptDecrypt::setupAESEncryptDecrypt()
{
    assert(1 == sizeof(cl_uchar));

    // ------- Input Buffer------- 
    input = (cl_uchar *)malloc(num_flows * flow_len * sizeof(cl_uchar));
    if(input == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (input)");
        return SDK_FAILURE;
    }

    /* Generate Randomized Input data -- Kay */
    set_random(input, num_flows * flow_len);

    // ------- Output Buffer------- 
    output = (cl_uchar *)malloc(num_flows * flow_len * sizeof(cl_uchar));
    if(output == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (output)");
        return SDK_FAILURE;
    } 

    // ------- Pkt Offset Buffer------- 
    pkt_offset = (cl_uint *)malloc(num_flows * sizeof(cl_uint));
    if(pkt_offset == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (pkt_offset)");
        return SDK_FAILURE;
    }

    // init packet offset array
    write_pkt_offset(pkt_offset);

    // ------- Key Buffer------- 
    /* 1 Byte = 8 bits */
    keySize = keySizeBits/8;
  
    /* due to unknown represenation of cl_uchar */ 
    keySizeBits = keySize*sizeof(cl_uchar); 

    keys = (cl_uchar *)malloc(num_flows * keySize);
    if(keys == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (keys)");
        return SDK_FAILURE;
    }

    /* Generate Randomized Keys*/
    set_random(keys, num_flows * keySize);

    // ------- Ivs Buffer------- 
    ivs = (cl_uchar *)malloc(num_flows * AES_IV_SIZE);
    if(ivs == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (ivs)");
        return SDK_FAILURE;
    }

    /* Generate Randomized Ivs */
    set_random(ivs, num_flows * AES_IV_SIZE);

    return SDK_SUCCESS;
}

int 
AESEncryptDecrypt::genBinaryImage()
{
    cl_int status = CL_SUCCESS;

    /*
     * Have a look at the available platforms and pick either
     * the AMD one if available or a reasonable default.
     */
    cl_uint numPlatforms;
    cl_platform_id platform = NULL;
    status = clGetPlatformIDs(0, NULL, &numPlatforms);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetPlatformIDs failed."))
    {
        return SDK_FAILURE;
    }
    if (0 < numPlatforms) 
    {
        cl_platform_id* platforms = new cl_platform_id[numPlatforms];
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clGetPlatformIDs failed."))
        {
            return SDK_FAILURE;
        }

        char platformName[100];
        for (unsigned i = 0; i < numPlatforms; ++i) 
        {
            status = clGetPlatformInfo(platforms[i],
                                       CL_PLATFORM_VENDOR,
                                       sizeof(platformName),
                                       platformName,
                                       NULL);

            if(!sampleCommon->checkVal(status,
                                       CL_SUCCESS,
                                       "clGetPlatformInfo failed."))
            {
                return SDK_FAILURE;
            }

            platform = platforms[i];
            if (!strcmp(platformName, "Advanced Micro Devices, Inc.")) 
            {
                break;
            }
        }
        std::cout << "Platform found : " << platformName << "\n";
        delete[] platforms;
    }

    if(NULL == platform)
    {
        sampleCommon->error("NULL platform found so Exiting Application.");
        return SDK_FAILURE;
    }

    /*
     * If we could find our platform, use it. Otherwise use just available platform.
     */
    cl_context_properties cps[5] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        CL_CONTEXT_OFFLINE_DEVICES_AMD,
        (cl_context_properties)1,
        0
    };

    context = clCreateContextFromType(cps,
                                      CL_DEVICE_TYPE_ALL,
                                      NULL,
                                      NULL,
                                      &status);

    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateContextFromType failed."))
    {
        return SDK_FAILURE;
    }

    /* create a CL program using the kernel source */
    streamsdk::SDKFile kernelFile;
    std::string kernelPath = sampleCommon->getPath();
    kernelPath.append("AESEncryptDecrypt_Kernels.cl");
    if(!kernelFile.open(kernelPath.c_str()))
    {
        std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
        return SDK_FAILURE;
    }
    const char * source = kernelFile.source().c_str();
    size_t sourceSize[] = {strlen(source)};
    program = clCreateProgramWithSource(context,
                                        1,
                                        &source,
                                        sourceSize,
                                        &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateProgramWithSource failed."))
    {
        return SDK_FAILURE;
    }
    
    std::string flagsStr = std::string("");

    // Get additional options
    if(isComplierFlagsSpecified())
    {
        streamsdk::SDKFile flagsFile;
        std::string flagsPath = sampleCommon->getPath();
        flagsPath.append(flags.c_str());
        if(!flagsFile.open(flagsPath.c_str()))
        {
            std::cout << "Failed to load flags file: " << flagsPath << std::endl;
            return SDK_FAILURE;
        }
        flagsFile.replaceNewlineWithSpaces();
        const char * flags = flagsFile.source().c_str();
        flagsStr.append(flags);
    }

    if(flagsStr.size() != 0)
        std::cout << "Build Options are : " << flagsStr.c_str() << std::endl;


    /* create a cl program executable for all the devices specified */
    status = clBuildProgram(program,
                            0,
                            NULL,
                            flagsStr.c_str(),
                            NULL,
                            NULL);

    sampleCommon->checkVal(status,
                        CL_SUCCESS,
                        "clBuildProgram failed.");

    size_t numDevices;
    status = clGetProgramInfo(program, 
                           CL_PROGRAM_NUM_DEVICES,
                           sizeof(numDevices),
                           &numDevices,
                           NULL );
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetProgramInfo(CL_PROGRAM_NUM_DEVICES) failed."))
    {
        return SDK_FAILURE;
    }

    std::cout << "Number of devices found : " << numDevices << "\n\n";
    devices = (cl_device_id *)malloc( sizeof(cl_device_id) * numDevices );
    if(devices == NULL)
    {
        sampleCommon->error("Failed to allocate host memory.(devices)");
        return SDK_FAILURE;
    }
    /* grab the handles to all of the devices in the program. */
    status = clGetProgramInfo(program, 
                              CL_PROGRAM_DEVICES, 
                              sizeof(cl_device_id) * numDevices,
                              devices,
                              NULL );
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetProgramInfo(CL_PROGRAM_DEVICES) failed."))
    {
        return SDK_FAILURE;
    }


    /* figure out the sizes of each of the binaries. */
    size_t *binarySizes = (size_t*)malloc( sizeof(size_t) * numDevices );
    if(devices == NULL)
    {
        sampleCommon->error("Failed to allocate host memory.(binarySizes)");
        return SDK_FAILURE;
    }
    
    status = clGetProgramInfo(program, 
                              CL_PROGRAM_BINARY_SIZES,
                              sizeof(size_t) * numDevices, 
                              binarySizes, NULL);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetProgramInfo(CL_PROGRAM_BINARY_SIZES) failed."))
    {
        return SDK_FAILURE;
    }

    size_t i = 0;
    /* copy over all of the generated binaries. */
    char **binaries = (char **)malloc( sizeof(char *) * numDevices );
    if(binaries == NULL)
    {
        sampleCommon->error("Failed to allocate host memory.(binaries)");
        return SDK_FAILURE;
    }

    for(i = 0; i < numDevices; i++)
    {
        if(binarySizes[i] != 0)
        {
            binaries[i] = (char *)malloc( sizeof(char) * binarySizes[i]);
            if(binaries[i] == NULL)
            {
                sampleCommon->error("Failed to allocate host memory.(binaries[i])");
                return SDK_FAILURE;
            }
        }
        else
        {
            binaries[i] = NULL;
        }
    }
    status = clGetProgramInfo(program, 
                              CL_PROGRAM_BINARIES,
                              sizeof(char *) * numDevices, 
                              binaries, 
                              NULL);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetProgramInfo(CL_PROGRAM_BINARIES) failed."))
    {
        return SDK_FAILURE;
    }

    /* dump out each binary into its own separate file. */
    for(i = 0; i < numDevices; i++)
    {
        char fileName[100];
        sprintf(fileName, "%s.%d", dumpBinary.c_str(), (int)i);
        if(binarySizes[i] != 0)
        {
            char deviceName[1024];
            status = clGetDeviceInfo(devices[i], 
                                     CL_DEVICE_NAME, 
                                     sizeof(deviceName),
                                     deviceName, 
                                     NULL);
            if(!sampleCommon->checkVal(status,
                                       CL_SUCCESS,
                                       "clGetDeviceInfo(CL_DEVICE_NAME) failed."))
            {
                return SDK_FAILURE;
            }

            printf( "%s binary kernel: %s\n", deviceName, fileName);
            streamsdk::SDKFile BinaryFile;
            if(!BinaryFile.writeBinaryToFile(fileName, 
                                             binaries[i], 
                                             binarySizes[i]))
            {
                std::cout << "Failed to load kernel file : " << fileName << std::endl;
                return SDK_FAILURE;
            }
        }
        else
        {
            printf("Skipping %s since there is no binary data to write!\n",
                    fileName);
        }
    }

    // Release all resouces and memory
    for(i = 0; i < numDevices; i++)
    {
        if(binaries[i] != NULL)
        {
            free(binaries[i]);
            binaries[i] = NULL;
        }
    }

    if(binaries != NULL)
    {
        free(binaries);
        binaries = NULL;
    }

    if(binarySizes != NULL)
    {
        free(binarySizes);
        binarySizes = NULL;
    }

    if(devices != NULL)
    {
        free(devices);
        devices = NULL;
    }

    status = clReleaseProgram(program);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseProgram failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseContext(context);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseContext failed."))
    {
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}


int
AESEncryptDecrypt::setupCL(void)
{
    cl_int status = 0;
    size_t deviceListSize;

    cl_device_type dType;
    
    if(deviceType.compare("cpu") == 0)
    {
        dType = CL_DEVICE_TYPE_CPU;
    }
    else //deviceType = "gpu" 
    {
        dType = CL_DEVICE_TYPE_GPU;
        if(isThereGPU() == false)
        {
            std::cout << "GPU not found. Falling back to CPU device" << std::endl;
            dType = CL_DEVICE_TYPE_CPU;
        }
    }

    /*
     * Have a look at the available platforms and pick either
     * the AMD one if available or a reasonable default.
     */

    cl_uint numPlatforms;
    cl_platform_id platform = NULL;
    status = clGetPlatformIDs(0, NULL, &numPlatforms);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetPlatformIDs failed."))
    {
        return SDK_FAILURE;
    }
    if (0 < numPlatforms) 
    {
        cl_platform_id* platforms = new cl_platform_id[numPlatforms];
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clGetPlatformIDs failed."))
        {
            return SDK_FAILURE;
        }
        if(isPlatformEnabled())
        {
            platform = platforms[platformId];
        }
        else
        {
            for (unsigned i = 0; i < numPlatforms; ++i) 
            {
                char pbuf[100];
                status = clGetPlatformInfo(platforms[i],
                                           CL_PLATFORM_VENDOR,
                                           sizeof(pbuf),
                                           pbuf,
                                           NULL);

                if(!sampleCommon->checkVal(status,
                                           CL_SUCCESS,
                                           "clGetPlatformInfo failed."))
                {
                    return SDK_FAILURE;
                }

                platform = platforms[i];
                if (!strcmp(pbuf, "Advanced Micro Devices, Inc.")) 
                {
                    break;
                }
            }
        }
        delete[] platforms;
    }

    if(NULL == platform)
    {
        sampleCommon->error("NULL platform found so Exiting Application.");
        return SDK_FAILURE;
    }

    // Display available devices.
    if(!sampleCommon->displayDevices(platform, dType))
    {
        sampleCommon->error("sampleCommon::displayDevices() failed");
        return SDK_FAILURE;
    }

    /*
     * If we could find our platform, use it. Otherwise use just available platform.
     */
    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        0
    };

    context = clCreateContextFromType(
                  cps,
                  dType,
                  NULL,
                  NULL,
                  &status);

    if(!sampleCommon->checkVal(status, 
                        CL_SUCCESS,
                        "clCreateContextFromType failed."))
        return SDK_FAILURE;

    /* First, get the size of device list data */
    status = clGetContextInfo(
                context, 
                CL_CONTEXT_DEVICES, 
                0, 
                NULL, 
                &deviceListSize);
    if(!sampleCommon->checkVal(
                status, 
                CL_SUCCESS,
                "clGetContextInfo failed."))
        return SDK_FAILURE;

    int deviceCount = (int)(deviceListSize / sizeof(cl_device_id));
    if(!sampleCommon->validateDeviceId(deviceId, deviceCount))
    {
        sampleCommon->error("sampleCommon::validateDeviceId() failed");
        return SDK_FAILURE;
    }

    /* Now allocate memory for device list based on the size we got earlier */
    devices = (cl_device_id *)malloc(deviceListSize);
    if(devices == NULL)
    {
        sampleCommon->error("Failed to allocate memory (devices).");
        return SDK_FAILURE;
    }

    /* Now, get the device list data */
    status = clGetContextInfo(
                context, 
                CL_CONTEXT_DEVICES, 
                deviceListSize, 
                devices, 
                NULL);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS, 
                "clGetGetContextInfo failed."))
        return SDK_FAILURE;

    /* Get Device specific Information */
    status = clGetDeviceInfo(
            devices[deviceId],
            CL_DEVICE_MAX_WORK_GROUP_SIZE,
            sizeof(size_t),
            (void *)&maxWorkGroupSize,
            NULL);

    std::cout << "Max work group size : " << maxWorkGroupSize << std::endl;
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS, 
                "clGetDeviceInfo CL_DEVICE_MAX_WORK_GROUP_SIZE failed."))
        return SDK_FAILURE;


    status = clGetDeviceInfo(
                devices[deviceId],
                CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                sizeof(cl_uint),
                (void *)&maxDimensions,
                NULL);

    std::cout << "Max work item dimensions : " << maxDimensions << std::endl;
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS, 
                "clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS failed."))
        return SDK_FAILURE;


    maxWorkItemSizes = (size_t *)malloc(maxDimensions*sizeof(size_t));
    
    status = clGetDeviceInfo(
                devices[deviceId],
                CL_DEVICE_MAX_WORK_ITEM_SIZES,
                sizeof(size_t)*maxDimensions,
                (void *)maxWorkItemSizes,
                NULL);

    std::cout << "Max work item size : " << maxWorkItemSizes << std::endl;
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS, 
                "clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_SIZES failed."))
        return SDK_FAILURE;


    status = clGetDeviceInfo(
                devices[deviceId],
                CL_DEVICE_LOCAL_MEM_SIZE,
                sizeof(cl_ulong),
                (void *)&totalLocalMemory,
                NULL);

    std::cout << "Device local mem size : " << totalLocalMemory << std::endl;
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS, 
                        "clGetDeviceInfo CL_DEVICE_LOCAL_MEM_SIZES failed."))
        return SDK_FAILURE;


    {
        /* The block is to move the declaration of prop closer to its use */
        cl_command_queue_properties prop = 0;
        commandQueue = clCreateCommandQueue(
                        context, 
                        devices[deviceId], 
                        prop, 
                        &status);
        if(!sampleCommon->checkVal(
                            status,
                            0,
                            "clCreateCommandQueue failed."))
            return SDK_FAILURE;
    }

    // Set Presistent memory only for AMD platform
    cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
    if(isAmdPlatform())
        inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;
    
    // input buffer
    inputBuffer = clCreateBuffer(
                    context, 
                    inMemFlags,
                    sizeof(cl_uchar ) * num_flows * flow_len,
                    NULL, 
                    &status);
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clCreateBuffer failed. (inputBuffer)"))
        return SDK_FAILURE;

    // output buffer
    outputBuffer = clCreateBuffer(
                    context, 
                    CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
                    sizeof(cl_uchar ) * num_flows * flow_len,
                    NULL, 
                    &status);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clCreateBuffer failed. (outputBuffer)"))
        return SDK_FAILURE;

    // pkt offset array
    pktOffsetBuffer = clCreateBuffer(
                    context, 
                    CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                    sizeof(cl_uint) * num_flows,
                    pkt_offset,
                    &status);
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clCreateBuffer failed. (pkt offset)"))
        return SDK_FAILURE;

    // key buffer
    keyBuffer = clCreateBuffer(
                    context, 
                    CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                    keySize * num_flows,
                    keys,
                    &status);
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clCreateBuffer failed. (key buffer)"))
        return SDK_FAILURE;

    // ivs Buffer
    ivsBuffer = clCreateBuffer(
                    context, 
                    CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                    AES_IV_SIZE * num_flows,
                    ivs,
                    &status);
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clCreateBuffer failed. (ivs buffer)"))
        return SDK_FAILURE;
    

    /* create a CL program using the kernel source */
    streamsdk::SDKFile kernelFile;
    std::string kernelPath = sampleCommon->getPath();

    if(isLoadBinaryEnabled())
    {
        kernelPath.append(loadBinary.c_str());
        if(!kernelFile.readBinaryFromFile(kernelPath.c_str()))
        {
            std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
            return SDK_FAILURE;
        }

        const char * binary = kernelFile.source().c_str();
        size_t binarySize = kernelFile.source().size();
        program = clCreateProgramWithBinary(context,
                                            1,
                                            &devices[deviceId], 
                                            (const size_t *)&binarySize,
                                            (const unsigned char**)&binary,
                                            NULL,
                                            &status);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clCreateProgramWithBinary failed."))
        {
            return SDK_FAILURE;
        }

    }
    else
    {
        kernelPath.append("AESEncryptDecrypt_Kernels.cl");
        if(!kernelFile.open(kernelPath.c_str()))
        {
            std::cout << "Failed to load kernel file: " << kernelPath << std::endl;
            return SDK_FAILURE;
        }
        const char * source = kernelFile.source().c_str();
        size_t sourceSize[] = { strlen(source) };
        program = clCreateProgramWithSource(
                    context,
                    1,
                    &source,
                    sourceSize,
                    &status);
        if(!sampleCommon->checkVal(
                            status,
                            CL_SUCCESS,
                            "clCreateProgramWithSource failed."))
            return SDK_FAILURE;
    }

    std::string flagsStr = std::string("");

    // Get additional options
    if(isComplierFlagsSpecified())
    {
        streamsdk::SDKFile flagsFile;
        std::string flagsPath = sampleCommon->getPath();
        flagsPath.append(flags.c_str());
        if(!flagsFile.open(flagsPath.c_str()))
        {
            std::cout << "Failed to load flags file: " << flagsPath << std::endl;
            return SDK_FAILURE;
        }
        flagsFile.replaceNewlineWithSpaces();
        const char * flags = flagsFile.source().c_str();
        flagsStr.append(flags);
    }

    if(flagsStr.size() != 0)
        std::cout << "Build Options are : " << flagsStr.c_str() << std::endl;

    

    /* create a cl program executable for all the devices specified */
    status = clBuildProgram(program, 1, &devices[deviceId], flagsStr.c_str(), NULL, NULL);
    if(status != CL_SUCCESS)
    {
        if(status == CL_BUILD_PROGRAM_FAILURE)
        {
            cl_int logStatus;
            char * buildLog = NULL;
            size_t buildLogSize = 0;
            logStatus = clGetProgramBuildInfo(program,
                                              devices[deviceId],
                                              CL_PROGRAM_BUILD_LOG,
                                              buildLogSize,
                                              buildLog,
                                              &buildLogSize);
            if(!sampleCommon->checkVal(logStatus,
                                       CL_SUCCESS,
                                       "clGetProgramBuildInfo failed."))
            {
                  return SDK_FAILURE;
            }
            
            buildLog = (char*)malloc(buildLogSize);
            if(buildLog == NULL)
            {
                sampleCommon->error("Failed to allocate host memory. (buildLog)");
                return SDK_FAILURE;
            }
            memset(buildLog, 0, buildLogSize);

            logStatus = clGetProgramBuildInfo(program, 
                                              devices[deviceId], 
                                              CL_PROGRAM_BUILD_LOG, 
                                              buildLogSize, 
                                              buildLog, 
                                              NULL);
            if(!sampleCommon->checkVal(logStatus,
                                       CL_SUCCESS,
                                       "clGetProgramBuildInfo failed."))
            {
                  free(buildLog);
                  return SDK_FAILURE;
            }

            std::cout << " \n\t\t\tBUILD LOG\n";
            std::cout << " ************************************************\n";
            std::cout << buildLog << std::endl;
            std::cout << " ************************************************\n";
            free(buildLog);
        }

          if(!sampleCommon->checkVal(status,
                                     CL_SUCCESS,
                                     "clBuildProgram failed."))
          {
                return SDK_FAILURE;
          }
    }

    /* get a kernel object handle for a kernel with the given name */
    if(decrypt)
    {
        kernel = clCreateKernel(program, "AES_cbc_128_decrypt", &status);
    }
    else
    {
        kernel = clCreateKernel(program, "AES_cbc_128_encrypt", &status);
    }
        
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clCreateKernel failed."))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}


int 
AESEncryptDecrypt::runCLKernels(void)
{
    cl_int   status;
    cl_int eventStatus = CL_QUEUED;
    
    size_t globalThreads[1]= {num_flows};
    size_t localThreads[1] = {256};

    status =  clGetKernelWorkGroupInfo(
                    kernel,
                    devices[deviceId],
                    CL_KERNEL_LOCAL_MEM_SIZE,
                    sizeof(cl_ulong),
                    &usedLocalMemory,
                    NULL);
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clGetKernelWorkGroupInfo failed.(usedLocalMemory)"))
        return SDK_FAILURE;

    availableLocalMemory = totalLocalMemory - usedLocalMemory; 

    std::cout << " availableLocalMemory : " << availableLocalMemory 
	     << " usedLocalMemory : "<< usedLocalMemory << std::endl;

    /* Check group size against kernelWorkGroupSize */
    status = clGetKernelWorkGroupInfo(kernel,
                                      devices[deviceId],
                                      CL_KERNEL_WORK_GROUP_SIZE,
                                      sizeof(size_t),
                                      &kernelWorkGroupSize,
                                      0);
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS, 
                        "clGetKernelWorkGroupInfo failed."))
    {
        return SDK_FAILURE;
    }
    std::cout << "kernelWorkGroupSize : " << kernelWorkGroupSize << std::endl;

    if((cl_uint)(localThreads[0]) > kernelWorkGroupSize )
    {
        std::cout << "Unsupported: Device does not support requested number of work items."<<std::endl;
        return SDK_SUCCESS;
    }

    
    if(localThreads[0] > maxWorkItemSizes[0] ||
       localThreads[0] > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support requested number of work items."<<std::endl;
        return SDK_SUCCESS;
    }
 
    cl_event writeEvt;
    status = clEnqueueWriteBuffer(
                commandQueue,
                inputBuffer,
                CL_FALSE,
                0,
                sizeof(cl_uchar ) * num_flows * flow_len,
                input,
                0,
                NULL,
                &writeEvt);
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clEnqueueWriteBuffer failed. (inputBuffer)"))
        return SDK_FAILURE;

    status = clFlush(commandQueue);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clFlush failed."))
        return SDK_FAILURE;

    eventStatus = CL_QUEUED;

	 // Wait for write to complete FIXME
    while(eventStatus != CL_COMPLETE)
    {
        status = clGetEventInfo(
                        writeEvt, 
                        CL_EVENT_COMMAND_EXECUTION_STATUS, 
                        sizeof(cl_int),
                        &eventStatus,
                        NULL);
        if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clGetEventInfo failed."))
                return SDK_FAILURE;
    }

    status = clReleaseEvent(writeEvt);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseEvent failed. (writeEvt)"))
        return SDK_FAILURE;


    /*** Set appropriate arguments to the kernel ***/
    status = clSetKernelArg(
                    kernel, 
                    0, 
                    sizeof(cl_mem), 
                    (void *)&outputBuffer);
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clSetKernelArg failed. (outputBuffer)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    1, 
                    sizeof(cl_mem), 
                    (void *)&inputBuffer);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (inputBuffer)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    2, 
                    sizeof(cl_uint), 
                    (void *)&num_flows);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (num_flows)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    3, 
                    sizeof(cl_uint), 
                    (void *)&rounds);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (rounds)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    4, 
                    sizeof(cl_mem), 
                    (void *)&pktOffsetBuffer);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (pktOffsetBuffer)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    5, 
                    sizeof(cl_mem), 
                    (void *)&keyBuffer);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (keyBuffer)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    6, 
                    sizeof(cl_mem), 
                    (void *)&ivsBuffer);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (ivsBuffer)"))
        return SDK_FAILURE;

    /* 
     * Enqueue a kernel run call.
     */
    cl_event ndrEvt;
    status = clEnqueueNDRangeKernel(
            commandQueue,
            kernel,
            1,
            NULL,
            globalThreads,
            localThreads,
            0,
            NULL,
            &ndrEvt);

    std::cout << "After kernel running--------------------------------" << std::endl;
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clEnqueueNDRangeKernel failed."))
        return SDK_FAILURE;
    std::cout << "--------------------------------" << std::endl;

    status = clFlush(commandQueue);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clFlush failed."))
        return SDK_FAILURE;

    eventStatus = CL_QUEUED;
    while(eventStatus != CL_COMPLETE)
    {
        status = clGetEventInfo(
                        ndrEvt, 
                        CL_EVENT_COMMAND_EXECUTION_STATUS, 
                        sizeof(cl_int),
                        &eventStatus,
                        NULL);
            if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clGetEventInfo failed."))
                return SDK_FAILURE;
    }

    status = clReleaseEvent(ndrEvt);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseEvent failed. (ndrEvt)"))
        return SDK_FAILURE;

    /* Enqueue the results to application pointer*/
    cl_event readEvt;
    status = clEnqueueReadBuffer(
                commandQueue,
                outputBuffer,
                CL_FALSE,
                0,
                num_flows * flow_len * sizeof(cl_uchar),
                output,
                0,
                NULL,
                &readEvt);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clEnqueueReadBuffer failed."))
        return SDK_FAILURE;

    status = clFlush(commandQueue);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clFlush failed."))
        return SDK_FAILURE;

    eventStatus = CL_QUEUED;
	 // FIXME: wait for read to complete
    while(eventStatus != CL_COMPLETE)
    {
	    status = clGetEventInfo(
			    readEvt, 
			    CL_EVENT_COMMAND_EXECUTION_STATUS, 
			    sizeof(cl_int),
			    &eventStatus,
			    NULL);
	    if(!sampleCommon->checkVal(status,
				    CL_SUCCESS, 
				    "clGetEventInfo failed."))
		    return SDK_FAILURE;
    }

    status = clReleaseEvent(readEvt);
    if(!sampleCommon->checkVal(status,
			    CL_SUCCESS,
			    "clReleaseEvent failed. (readEvt)"))
	    return SDK_FAILURE;

    return SDK_SUCCESS;
}

	int 
AESEncryptDecrypt::initialize()
{
	// Call base class Initialize to get default configuration
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    streamsdk::Option* ifilename_opt = new streamsdk::Option;
    if(!ifilename_opt)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }

    ////////////////
    streamsdk::Option* decrypt_opt = new streamsdk::Option;
    if(!decrypt_opt)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }
    decrypt_opt->_sVersion = "z";
    decrypt_opt->_lVersion = "decrypt";
    decrypt_opt->_description = "Decrypt the Input Image"; 
    decrypt_opt->_type     = streamsdk::CA_NO_ARGUMENT;
    decrypt_opt->_value    = &decrypt;
    sampleArgs->AddOption(decrypt_opt);

    delete decrypt_opt;

    streamsdk::Option* num_iterations = new streamsdk::Option;
    if(!num_iterations)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }

    num_iterations->_sVersion = "i";
    num_iterations->_lVersion = "iterations";
    num_iterations->_description = "Number of iterations for kernel execution";
    num_iterations->_type = streamsdk::CA_ARG_INT;
    num_iterations->_value = &iterations;

    sampleArgs->AddOption(num_iterations);

    delete num_iterations;

    return SDK_SUCCESS;
}

int 
AESEncryptDecrypt::setup()
{
    if(setupAESEncryptDecrypt()!=SDK_SUCCESS)
      return SDK_FAILURE;
    
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    if(setupCL()!=SDK_SUCCESS)
      return SDK_FAILURE;

    sampleCommon->stopTimer(timer);

    setupTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}


int 
AESEncryptDecrypt::run()
{
    for(int i = 0; i < 2 && iterations != 1; i++)
    {
        /* Arguments are set and execution call is enqueued on command buffer */
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);   

    std::cout << "Executing kernel for " << iterations << 
        " iterations" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        /* Arguments are set and execution call is enqueued on command buffer */
        if(runCLKernels()!=SDK_SUCCESS)
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);
    totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;
    
    return SDK_SUCCESS;
}


int AESEncryptDecrypt::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

    status = clReleaseKernel(kernel);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseKernel failed."))
        return SDK_FAILURE;

    status = clReleaseProgram(program);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseProgram failed."))
        return SDK_FAILURE;
 
   status = clReleaseMemObject(inputBuffer);
   if(!sampleCommon->checkVal(
      status,
      CL_SUCCESS,
      "clReleaseMemObject failed."))
      return SDK_FAILURE;

   status = clReleaseMemObject(outputBuffer);
   if(!sampleCommon->checkVal(
      status,
      CL_SUCCESS,
      "clReleaseMemObject failed."))
      return SDK_FAILURE;

   status = clReleaseMemObject(keyBuffer);
   if(!sampleCommon->checkVal(
      status,
      CL_SUCCESS,
      "clReleaseMemObject failed."))
      return SDK_FAILURE;

   status = clReleaseMemObject(ivsBuffer);
   if(!sampleCommon->checkVal(
      status,
      CL_SUCCESS,
      "clReleaseMemObject failed."))
      return SDK_FAILURE;

   status = clReleaseMemObject(pktOffsetBuffer);
   if(!sampleCommon->checkVal(
      status,
      CL_SUCCESS,
      "clReleaseMemObject failed."))
      return SDK_FAILURE;

   status = clReleaseCommandQueue(commandQueue);
     if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseCommandQueue failed."))
        return SDK_FAILURE;

   status = clReleaseContext(context);
   if(!sampleCommon->checkVal(
         status,
         CL_SUCCESS,
         "clReleaseContext failed."))
      return SDK_FAILURE;

    /* release program resources (input memory etc.) */
    if(input) 
        free(input);
    
    if(keys)
        free(keys);
    
    if(pkt_offset)
        free(pkt_offset);
    
    if(ivs)
        free(ivs);

    if(output) 
        free(output);

    if(devices)
        free(devices);

    if(maxWorkItemSizes)
        free(maxWorkItemSizes);

   return SDK_SUCCESS;
}

// some pure virtual function in SDK
int AESEncryptDecrypt::verifyResults()
{
	return 0;
}

int 
main(int argc, char * argv[])
{
    AESEncryptDecrypt clAESEncryptDecrypt("OpenCL AES Encrypt Decrypt");

    if(clAESEncryptDecrypt.initialize()!=SDK_SUCCESS)
        return SDK_FAILURE;
    if(!clAESEncryptDecrypt.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clAESEncryptDecrypt.isDumpBinaryEnabled())
    {
        return clAESEncryptDecrypt.genBinaryImage();
    }
    else
    {
        if(clAESEncryptDecrypt.setup()!=SDK_SUCCESS)
            return SDK_FAILURE;
        if(clAESEncryptDecrypt.run()!=SDK_SUCCESS)
            return SDK_FAILURE;
        //if(clAESEncryptDecrypt.verifyResults()!=SDK_SUCCESS)
        //    return SDK_FAILURE;
        if(clAESEncryptDecrypt.cleanup()!=SDK_SUCCESS)
            return SDK_FAILURE;
        //clAESEncryptDecrypt.printStats();
    }

    return SDK_SUCCESS;
}

