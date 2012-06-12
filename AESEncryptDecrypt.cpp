#include "AESEncryptDecrypt.hpp"

using namespace AES;

#define DYNAMIC_STREAM 1

void AESEncryptDecrypt::write_pkt_offset(cl_uint *pkt_offset)
{
	cl_uint i = 0;
	cl_uint offset = 0;
	for (i = 0; i <= num_flows; i ++) {
		pkt_offset[i] = offset;
		offset += flow_len;
	}
}

void AESEncryptDecrypt::set_random(cl_uchar *input, int len)
{
	int i;
	for (i = 0; i < len; i ++)
		input[i] = rand() % 26 + 'a';
}

int AESEncryptDecrypt::setupAESEncryptDecrypt()
{
#if defined(DYNAMIC_STREAM)

	// Set parameters for the stream set
	streamGenerator.SetStreamNumber(1024);
	streamGenerator.SetInterval(30);
	streamGenerator.InitStreams(50000, 10, 50, 5, 30, 5);

		/* 1 Byte = 8 bits, 128bits => 16bytes */
	keySize = keySizeBits/8;
  
	/* due to unknown represenation of cl_uchar */ 
	keySizeBits = keySize*sizeof(cl_uchar); 

	buffer_size = streamGenerator.GetMaxBufferSize();
	stream_num = streamGenerator.GetStreamNumber();

	input = (cl_uchar *)malloc(buffer_size * sizeof(cl_uchar));
	CHECK_ALLOCATION(input, "Failed to allocate host memory. (input)");

	memset(input, 'a', buffer_size * sizeof(cl_uchar));

	output = (cl_uchar *)malloc(buffer_size * sizeof(cl_uchar));
	CHECK_ALLOCATION(output, "Failed to allocate host memory. (output)");

	pkt_offset = (cl_uint *)malloc((stream_num + 1) * sizeof(cl_uint));
	CHECK_ALLOCATION(output, "Failed to allocate host memory. (pkt_offset)");

	keys = (cl_uchar *)malloc(stream_num * keySize);
	CHECK_ALLOCATION(output, "Failed to allocate host memory. (keys)");

	ivs = (cl_uchar *)malloc(stream_num * AES_IV_SIZE);
	CHECK_ALLOCATION(output, "Failed to allocate host memory. (ivs)");

#else
	cl_uint i, j;

	// ------- Input Buffer------- 
	input = (cl_uchar *)malloc(num_flows * flow_len * sizeof(cl_uchar));
	CHECK_ALLOCATION(input, "Failed to allocate host memory. (input)");
	
	/* Generate Randomized Input data -- Kay */
	set_random(input, num_flows * flow_len);
	
	// ------- Output Buffer------- 
	output = (cl_uchar *)malloc(num_flows * flow_len * sizeof(cl_uchar));
	CHECK_ALLOCATION(output, "Failed to allocate host memory. (output)");
	
	if (decrypt) {
		block_count = (num_flows * flow_len) / AES_BLOCK_SIZE;
		pkt_index = (cl_uint *)malloc(block_count * sizeof(cl_uint));

		// Init packet index
		for (i = 0; i < num_flows; i ++) {
			for (j = 0; j < flow_len / AES_BLOCK_SIZE; j ++) {
				pkt_index[i * j] = i;
			}
		}
	} else {
    	// ------- Pkt Offset Buffer------- 
		pkt_offset = (cl_uint *)malloc((num_flows + 1) * sizeof(cl_uint));
		CHECK_ALLOCATION(output, "Failed to allocate host memory. (pkt_offset)");
	
		// init packet offset array
		write_pkt_offset(pkt_offset);
	}
	// ------- Key Buffer------- 
	/* 1 Byte = 8 bits, 128bits => 16bytes */
	keySize = keySizeBits/8;
  
	/* due to unknown represenation of cl_uchar */ 
	keySizeBits = keySize*sizeof(cl_uchar); 

	keys = (cl_uchar *)malloc(num_flows * keySize);
	CHECK_ALLOCATION(output, "Failed to allocate host memory. (keys)");

	/* Generate Randomized Keys*/
	set_random(keys, num_flows * keySize);

	// ------- Ivs Buffer------- 
	ivs = (cl_uchar *)malloc(num_flows * AES_IV_SIZE);
	CHECK_ALLOCATION(output, "Failed to allocate host memory. (ivs)");

	/* Generate Randomized Ivs */
	set_random(ivs, num_flows * AES_IV_SIZE);
#endif

	return SDK_SUCCESS;
}

int 
AESEncryptDecrypt::genBinaryImage()
{
	streamsdk::bifData binaryData;
	binaryData.kernelName = std::string("AESEncryptDecrypt_Kernel.cl");
	binaryData.flagsStr = std::string("");
	if(isComplierFlagsSpecified())
		binaryData.flagsFileName = std::string(flags.c_str());

	binaryData.binaryName = std::string(dumpBinary.c_str());
	int status = sampleCommon->generateBinaryImage(binaryData);
	return status;
}

int
AESEncryptDecrypt::setupEncryption(void)
{
	cl_int status = 0;
	// Set Presistent memory only for AMD platform
	cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
	if(isAmdPlatform())
		inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;

#if defined(DYNAMIC_STREAM)
//	buffer_size = streamGenerator.GetMaxBufferSize();
//	stream_num = streamGenerator.GetStreamNumber();

	inputBuffer = clCreateBuffer(
					context,
					inMemFlags,
					sizeof(cl_uchar) * buffer_size,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inputBuffer)");

	outputBuffer = clCreateBuffer(
					context,
					CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
					sizeof(cl_uchar) * buffer_size,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBuffer)");

	pktOffsetBuffer = clCreateBuffer(
					context,
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					sizeof(cl_uint) * (stream_num + 1),
					pkt_offset,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (pktOffsetBuffer)");

	keyBuffer = clCreateBuffer(
					context,
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					keySize * stream_num,
					keys,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (keyBuffer)");

	ivsBuffer = clCreateBuffer(
					context,
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					AES_IV_SIZE * stream_num,
					ivs,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (ivsBuffer)");

#else
	inputBuffer = clCreateBuffer(
					context,
					inMemFlags,
					sizeof(cl_uchar ) * num_flows * flow_len,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inputBuffer)");

	outputBuffer = clCreateBuffer(
					context,
					CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
					sizeof(cl_uchar ) * num_flows * flow_len,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBuffer)");

	pktOffsetBuffer = clCreateBuffer(
					context,
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					sizeof(cl_uint) * (num_flows + 1),
					pkt_offset,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (pktOffsetBuffer)");

	keyBuffer = clCreateBuffer(
					context,
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					keySize * num_flows,
					keys,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (keyBuffer)");

	ivsBuffer = clCreateBuffer(
					context,
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					AES_IV_SIZE * num_flows,
					ivs,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (ivsBuffer)");
#endif

	return SDK_SUCCESS;
}

int
AESEncryptDecrypt::setupDecryption(void)
{
	cl_int status = 0;
	// Set Presistent memory only for AMD platform
	cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
	if(isAmdPlatform())
		inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;

	inputBuffer = clCreateBuffer(
					context, 
					inMemFlags,
					sizeof(cl_uchar ) * num_flows * flow_len,
					NULL, 
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inputBuffer)");

	outputBuffer = clCreateBuffer(
					context, 
					CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
					sizeof(cl_uchar ) * num_flows * flow_len,
					NULL, 
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBuffer)");

	pktIndexBuffer = clCreateBuffer(
					context, 
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					sizeof(cl_uint) * block_count,
					pkt_index,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (pktIndexBuffer)");

	keyBuffer = clCreateBuffer(
					context, 
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					keySize * num_flows,
					keys,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (keyBuffer)");

	ivsBuffer = clCreateBuffer(
					context, 
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					AES_IV_SIZE * num_flows,
					ivs,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (ivsBuffer)");

	return SDK_SUCCESS;
}

int
AESEncryptDecrypt::setupCL(void)
{
	cl_int status = 0;
	cl_device_type dType;
    
	if(deviceType.compare("cpu") == 0)
	{
		std::cout << "Running on CPU......" << std::endl;
		dType = CL_DEVICE_TYPE_CPU;
	}
	else //deviceType = "gpu" 
	{
		dType = CL_DEVICE_TYPE_GPU;
		std::cout << "Running on GPU......" << std::endl;
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
	cl_platform_id platform = NULL;
	int retValue = sampleCommon->getPlatform(platform, platformId, isPlatformEnabled());
	CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::getPlatform() failed");

	// Display available devices.
	retValue = sampleCommon->displayDevices(platform, dType);
	CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::displayDevices() failed");

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
	CHECK_OPENCL_ERROR(status, "clCreateContextFromType failed.");

	// getting device on which to run the sample
	status = sampleCommon->getDevices(context, &devices, deviceId, isDeviceIdEnabled());
	CHECK_ERROR(status, SDK_SUCCESS, "sampleCommon::getDevices() failed");

	// Select the device!!!!!!!!
	deviceId = DEVICE_ID;

	{
		// The block is to move the declaration of prop closer to its use
		cl_command_queue_properties prop = 0;
		commandQueue = clCreateCommandQueue(
				context, 
				devices[deviceId], 
				prop, 
				&status);
		CHECK_OPENCL_ERROR( status, "clCreateCommandQueue failed.");
	}

	std::cout << "Device ID : " << deviceId << std::endl;
	
	//Set device info of given cl_device_id
	retValue = deviceInfo.setDeviceInfo(devices[deviceId]);
	CHECK_ERROR(retValue, 0, "SDKDeviceInfo::setDeviceInfo() failed");
   
	// create a CL program using the kernel source 
	streamsdk::buildProgramData buildData;
	buildData.kernelName = std::string("AESEncryptDecrypt_Kernel.cl");
	buildData.devices = devices;
	buildData.deviceId = deviceId;
	buildData.flagsStr = std::string("");
	if(isLoadBinaryEnabled())
		buildData.binaryName = std::string(loadBinary.c_str());

	if(isComplierFlagsSpecified())
		buildData.flagsFileName = std::string(flags.c_str());

	retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
	CHECK_ERROR(retValue, 0, "sampleCommon::buildOpenCLProgram() failed");

	/* get a kernel object handle for a kernel with the given name */
	if(decrypt)
	{
		kernel = clCreateKernel(program, "AES_cbc_128_decrypt", &status);
		setupDecryption();
	}
	else
	{
		kernel = clCreateKernel(program, "AES_cbc_128_encrypt_new", &status);
		setupEncryption();
	}
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

	return SDK_SUCCESS;
}


int 
AESEncryptDecrypt::runCLKernels(void)
{
	cl_int   status;
	cl_int eventStatus = CL_QUEUED;
	size_t globalThreads;
    
	// !!!???
	if (decrypt) {
		globalThreads = block_count;
	} else {
		globalThreads = num_flows;
	}
	size_t localThreads = 256;

//	std::cout << "Dimension : " << globalThreads << " " << localThreads << std::endl;

	status = kernelInfo.setKernelWorkGroupInfo(kernel, devices[deviceId]);
	CHECK_ERROR(status, SDK_SUCCESS, "KernelInfo.setKernelWorkGroupInfo() failed");

	availableLocalMemory = deviceInfo.localMemSize - kernelInfo.localMemoryUsed; 

	
	if((cl_uint)(localThreads) > kernelInfo.kernelWorkGroupSize )
	{
		std::cout << "!!! Work group size : " << kernelInfo.kernelWorkGroupSize
			<< " localThreads : " << localThreads << std::endl;
		localThreads = kernelInfo.kernelWorkGroupSize / 4;
	}

	if(localThreads > deviceInfo.maxWorkItemSizes[0] ||
		localThreads > deviceInfo.maxWorkGroupSize)
	{
		std::cout << "Unsupported: Device does not support requested number of work items."<<std::endl;
		return SDK_SUCCESS;
	}
	
 
//    std::cout << "kernelWorkGroupSize : " << kernelInfo.kernelWorkGroupSize
//        << "maxWorkItemSizes[0]" << deviceInfo.maxWorkItemSizes[0]
//        << "maxWorkGroupSize" << deviceInfo.maxWorkGroupSize << std::endl;
#if defined(DYNAMIC_STREAM)

	unsigned int this_stream_num;
	cl_event writeEvt;
	CPerfCounter t, counter;

	streamGenerator.StartStreams();
	counter.Reset();
	counter.Start();

	std::cout << "Stream num : " << stream_num 
		<< ", Start time : " << streamGenerator.GetStartTimestamp() 
		<< ", Interval : " << streamGenerator.GetInterval() << std::endl;

	for (int i = 0; i < iterations; i ++) {
		t.Reset();
		t.Start();

		// Generate Stream Data
		streamGenerator.GetStreams(input, buffer_size, 
			keys, ivs,
			pkt_offset, &this_stream_num);

		t.Stop();

		std::cout << "The " << i << "th iteration," 
			<< " Time of GetStreams is " << t.GetTotalTime() << "ms."
			<< " Time after GetStreams is " << counter.GetElapsedTime() << "ms." << std::endl;
		
		t.Reset();
		t.Start();

		// Write buffers
		status = clEnqueueWriteBuffer(
				commandQueue,
				inputBuffer,
				CL_FALSE,
				0,
				sizeof(cl_uchar) * buffer_size,
				input,
				0,
				NULL,
				&writeEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (inputBuffer)");

		status = clEnqueueWriteBuffer(
				commandQueue,
				pktOffsetBuffer,
				CL_FALSE,
				0,
				sizeof(cl_uint) * (stream_num + 1),
				pkt_offset,
				0,
				NULL,
				&writeEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (pktOffsetBuffer)");

		status = clEnqueueWriteBuffer(
				commandQueue,
				keyBuffer,
				CL_FALSE,
				0,
				keySize * stream_num,
				keys,
				0,
				NULL,
				&writeEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (keyBuffer)");

		status = clEnqueueWriteBuffer(
				commandQueue,
				ivsBuffer,
				CL_FALSE,
				0,
				AES_IV_SIZE * stream_num,
				ivs,
				0,
				NULL,
				&writeEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (ivsBuffer)");

		// Set appropriate arguments to the kernel
		status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outputBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (outputBuffer)");

		status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&inputBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inputBuffer)");

		status = clSetKernelArg(kernel, 2, sizeof(cl_uint), (void *)&stream_num);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (num_flows)");

		status = clSetKernelArg(kernel, 3, sizeof(cl_uint), (void *)&rounds);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (rounds)");

		status = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&pktOffsetBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (pktOffsetBuffer)");

		status = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&keyBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (keyBuffer)");

		status = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&ivsBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (ivsBuffer)");


		//Enqueue a kernel run call.
		cl_event ndrEvt;
		status = clEnqueueNDRangeKernel(
				commandQueue,
				kernel,
				1,
				NULL,
				&globalThreads,
				&localThreads,
				0,
				NULL,
				&ndrEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

		//	std::cout << "After Kernel Running ---------------------" << std::endl;

		status = clFlush(commandQueue);
		CHECK_OPENCL_ERROR(status, "clFlush failed.");

		status = sampleCommon->waitForEventAndRelease(&ndrEvt);
		CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");

		/* Enqueue the results to application pointer*/
		cl_event readEvt;
		status = clEnqueueReadBuffer(
					commandQueue,
					outputBuffer,
					CL_FALSE,
					0,
					sizeof(cl_uchar) * buffer_size,
					output,
					0,
					NULL,
					&readEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

		status = clFlush(commandQueue);
		CHECK_OPENCL_ERROR(status, "clFlush failed.");

		status = sampleCommon->waitForEventAndRelease(&readEvt);
		CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt) Failed");

		t.Stop();
		std::cout << "The " << i << "th iteration," 
			<< " End of loop, Time use is " << t.GetTotalTime() << "ms."
			<< " Time after is " << counter.GetElapsedTime() << "ms." << std::endl;

	}

	counter.Stop();
	std::cout << "End of execution, now is " << counter.GetTotalTime() << std::endl;

#else
	cl_event writeEvt;
	status = clEnqueueWriteBuffer(
				commandQueue,
				inputBuffer,
				CL_FALSE,
				0,
				sizeof(cl_uchar) * num_flows * flow_len,
				input,
				0,
				NULL,
				&writeEvt);
	CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (inputBuffer)");

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	status = sampleCommon->waitForEventAndRelease(&writeEvt);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(writeEvt) Failed");

	if (decrypt) {
		// Set appropriate arguments to the kernel
		status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inputBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inputBuffer)");

		status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&outputBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (outputBuffer)");

		status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&keyBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (keyBuffer)");

		status = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&ivsBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (ivsBuffer)");

		status = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&pktIndexBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (pktIndexBuffer)");
	
		status = clSetKernelArg(kernel, 5, sizeof(cl_uint), (void *)&block_count);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (block_count)");
	} else {
		// Set appropriate arguments to the kernel
		status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outputBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (outputBuffer)");

		status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&inputBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inputBuffer)");

		status = clSetKernelArg(kernel, 2, sizeof(cl_uint), (void *)&num_flows);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (num_flows)");

		status = clSetKernelArg(kernel, 3, sizeof(cl_uint), (void *)&rounds);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (rounds)");

		status = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&pktOffsetBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (pktOffsetBuffer)");

		status = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&keyBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (keyBuffer)");

		status = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&ivsBuffer);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (ivsBuffer)");
	}



	/* 
		* Enqueue a kernel run call.
		*/
	cl_event ndrEvt;
	status = clEnqueueNDRangeKernel(
			commandQueue,
			kernel,
			1,
			NULL,
			&globalThreads,
			&localThreads,
			0,
			NULL,
			&ndrEvt);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

//	std::cout << "After Kernel Running ---------------------" << std::endl;

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	status = sampleCommon->waitForEventAndRelease(&ndrEvt);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");

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
	CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	status = sampleCommon->waitForEventAndRelease(&readEvt);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt) Failed");
#endif

	return SDK_SUCCESS;
}

int 
AESEncryptDecrypt::initialize()
{
	// Call base class Initialize to get default configuration
	if(this->SDKSample::initialize())
		return SDK_FAILURE;

	streamsdk::Option* decrypt_opt = new streamsdk::Option;
	CHECK_ALLOCATION(decrypt_opt, "Memory allocation error.\n"); 

	decrypt_opt->_sVersion = "z";
	decrypt_opt->_lVersion = "decrypt";
	decrypt_opt->_description = "Decrypt the Input Image"; 
	decrypt_opt->_type     = streamsdk::CA_NO_ARGUMENT;
	decrypt_opt->_value    = &decrypt;
	sampleArgs->AddOption(decrypt_opt);

	delete decrypt_opt;

	streamsdk::Option* num_iterations = new streamsdk::Option;
	CHECK_ALLOCATION(num_iterations, "Memory allocation error.\n");

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
	if(setupAESEncryptDecrypt() != SDK_SUCCESS)
		return SDK_FAILURE;
    
	int timer = sampleCommon->createTimer();
	sampleCommon->resetTimer(timer);
	sampleCommon->startTimer(timer);

	if(setupCL()!= SDK_SUCCESS)
		return SDK_FAILURE;

	sampleCommon->stopTimer(timer);

	setupTime = (double)(sampleCommon->readTimer(timer));

	return SDK_SUCCESS;
}


int 
AESEncryptDecrypt::run()
{
#if defined(DYNAMIC_STREAM)

	std::cout << "Executing kernel for " << iterations << 
		" iterations" << std::endl;
	std::cout << "-------------------------------------------" << std::endl;

	int timer = sampleCommon->createTimer();
	sampleCommon->resetTimer(timer);
	sampleCommon->startTimer(timer);

	if(runCLKernels() != SDK_SUCCESS)
		return SDK_FAILURE;

	sampleCommon->stopTimer(timer);
	totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

#else

	for(int i = 0; i < 2 && iterations != 1; i++)
	{
		// Arguments are set and execution call is enqueued on command buffer
		if(runCLKernels() != SDK_SUCCESS)
			return SDK_FAILURE;
	}

	std::cout << "Executing kernel for " << iterations << 
		" iterations" << std::endl;
	std::cout << "-------------------------------------------" << std::endl;

	int timer = sampleCommon->createTimer();
	sampleCommon->resetTimer(timer);
	sampleCommon->startTimer(timer);

	for(int i = 0; i < iterations; i++)
	{
		// Arguments are set and execution call is enqueued on command buffer
		if(runCLKernels()!=SDK_SUCCESS)
			return SDK_FAILURE;
	}

	sampleCommon->stopTimer(timer);
	totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;
#endif
    
	return SDK_SUCCESS;
}

int 
AESEncryptDecrypt::verifyResults()
{
	return SDK_SUCCESS;
}

void AESEncryptDecrypt::printStats()
{
	totalTime = setupTime + totalKernelTime;
	cl_ulong bits = 8 * num_flows * flow_len;

	std::cout << "num_flows : " << num_flows << " flow_len : " << flow_len << std::endl;

	std::cout << "setupTime : " << setupTime << std::endl;
	std::cout << "totalKernelTime : " << totalKernelTime << " " 
			<< bits/(1000000000 * totalKernelTime) << std::endl; 
	std::cout << "totalTime : " << totalTime << " "
			<< bits/(1000000000 * totalTime) << std::endl;
}

int AESEncryptDecrypt::cleanup()
{
	// Releases OpenCL resources (Context, Memory etc.)
	cl_int status;

	status = clReleaseKernel(kernel);
	CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.");

	status = clReleaseProgram(program);
	CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.");
 
	status = clReleaseMemObject(inputBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(outputBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(keyBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(ivsBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	if (decrypt) {
		status = clReleaseMemObject(pktIndexBuffer);
		CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");
	} else {
		status = clReleaseMemObject(pktOffsetBuffer);
		CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");
	}

	status = clReleaseCommandQueue(commandQueue);
	CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.");

	status = clReleaseContext(context);
	CHECK_OPENCL_ERROR(status, "clReleaseContext failed.");

	// release program resources (input memory etc.)
	FREE(input);
    
	FREE(keys);
    
	if (decrypt) {
		FREE(pkt_index);
	} else {
		FREE(pkt_offset);
	}
    
	FREE(ivs);

	FREE(output);

	FREE(verificationOutput);

	FREE(devices);

	return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
	AESEncryptDecrypt clAESEncryptDecrypt("OpenCL AES Encrypt Decrypt");

	if(clAESEncryptDecrypt.initialize() != SDK_SUCCESS)
		return SDK_FAILURE;
	if(clAESEncryptDecrypt.parseCommandLine(argc, argv))
		return SDK_FAILURE;

	if(clAESEncryptDecrypt.isDumpBinaryEnabled())
	{
		return clAESEncryptDecrypt.genBinaryImage();
	}
    
	if(clAESEncryptDecrypt.setup() != SDK_SUCCESS)
		return SDK_FAILURE;
	if(clAESEncryptDecrypt.run() != SDK_SUCCESS)
		return SDK_FAILURE;
	//if(clAESEncryptDecrypt.verifyResults() != SDK_SUCCESS)
	//    return SDK_FAILURE;
	if(clAESEncryptDecrypt.cleanup() != SDK_SUCCESS)
		return SDK_FAILURE;
	clAESEncryptDecrypt.printStats();
	return SDK_SUCCESS;
}