#pragma once

#include <utility>
#include <CL/cl.hpp>

struct CLComponents {

	cl::Program program;
	cl::CommandQueue queue;
	cl::Context context;
	cl::Device default_device;

	void init();
	void build(const char* code, size_t len);

};