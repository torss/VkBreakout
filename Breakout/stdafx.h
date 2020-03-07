#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORECE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#define APP_NAME "Vulkan Breakout"
#define SCREEN_W 1280
#define SCREEN_H 720

#define ENABLE_OS_FRAME_TIME 0
#define ENABLE_VK_TIMESTAMP 1

#define STRESS_TEST 1
#if !STRESS_TEST
#define BRICK_COUNT 500
#endif

// Benchmark options
#define BRICK_COUNT 50000 // 5060
#define PERSIST_SAMPLES 1
#define DEVICE_LOCAL_MEMORY 0
#define SKIP_FRAMES 10

// Alias
#define MAX_PRIMS BRICK_COUNT

// Global variables
extern bool activeFrame;
