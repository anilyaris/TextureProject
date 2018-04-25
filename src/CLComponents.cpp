#include "CLComponents.hpp"
#include <string>

using std::string;

//Gets the GPU information that will be used for building OpenCL programs.
void CLComponents::init() {

	//get all platforms (drivers)
	std::vector<cl::Platform> all_platforms;
	cl::Platform::get(&all_platforms);
	if (all_platforms.size() == 0) throw string("No platforms found. Check OpenCL installation!");

	cl::Platform default_platform = all_platforms[0];
	string platformName = default_platform.getInfo<CL_PLATFORM_NAME>();
	if (platformName.compare(0, 5, "Intel") == 0 && all_platforms.size() > 1) default_platform = all_platforms[1];
	//std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";

	//get default device of the default platform
	std::vector<cl::Device> all_devices;
	default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
	if (all_devices.size() == 0) throw string("No devices found. Check OpenCL installation!");

	default_device = all_devices[0];
	//std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << "\n";

	context = cl::Context({ default_device });

	//create queue to which we will push commands for the device.
	queue = cl::CommandQueue(context, default_device);

}

//Builds an OpenCL program from a code string and its length.
void CLComponents::build(const char* code, size_t len) {

	cl::Program::Sources sources;
	sources.push_back({ code, len });

	program = cl::Program(context, sources);
	if (program.build({ default_device }) != CL_SUCCESS) throw string("Error building: " + program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device));

}