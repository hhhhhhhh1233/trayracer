#pragma once
#include <vector>
#include "vec3.h"
#include "mat4.h"
#include "color.h"
#include "ray.h"
#include "object.h"
#include <float.h>

// For multithreading
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <condition_variable>

//------------------------------------------------------------------------------
/**
*/

// Passed to each thread
struct RayMultithreadParameters
{
    RayMultithreadParameters(int a, int b)
    {
        MinY = a;
        MaxY = b;
    }

    RayMultithreadParameters()
    {
        MinY = 0;
        MaxY = 0;
    }

    int MinY;
    int MaxY;
};

class Raytracer
{
public:
    Raytracer(unsigned w, unsigned h, std::vector<Color>& frameBuffer, unsigned rpp, unsigned bounces);
    ~Raytracer();

    // start raytracing!
    unsigned int Raytrace();

    // same thing as above but it uses multithreading
    unsigned int RaytraceMultithreaded(unsigned int NumberOfJobs);

    // same thing as above but it uses multithreading
    void RaytraceChunk(RayMultithreadParameters Param);

    // add object to scene
    void AddObject(Object* obj);

    // single raycast, find object
    static bool Raycast(Ray ray, vec3& hitPoint, vec3& hitNormal, Object*& hitObject, float& distance, std::vector<Object*> const& objects);

    // set camera matrix
    void SetViewMatrix(mat4 val);

    // clear screen
    void Clear();

    // update matrices. Called automatically after setting view matrix
    void UpdateMatrices();

    // trace a path and return intersection color
    // n is bounce depth
    Color TracePath(Ray ray, unsigned n);
    Color TracePathNoRecursion(Ray ray, unsigned n);

    // get the color of the skybox in a direction
    Color Skybox(vec3 direction);

    std::vector<Color>& frameBuffer;
    
    // rays per pixel
    unsigned rpp;
    // max number of bounces before termination
    unsigned bounces = 5;

    // width of framebuffer
    const unsigned width;
    // height of framebuffer
    const unsigned height;
    
    const vec3 lowerLeftCorner = { -2.0, -1.0, -1.0 };
    const vec3 horizontal = { 4.0, 0.0, 0.0 };
    const vec3 vertical = { 0.0, 2.0, 0.0 };
    const vec3 origin = { 0.0, 2.0, 10.0f };

    // view matrix
    mat4 view;
    // Go from canonical to view frustum
    mat4 frustum;

    // Multithreading methods
	void StartThreads();
    void ThreadLoop();
    void QueueJob(RayMultithreadParameters Params);
    void JoinAllThreads();
    void StopThreads();

private:

    std::vector<Object*> objects;

    // Multithreading variables
	std::vector<std::thread> Threads;
    std::atomic<int> DoneThreads;
    std::queue<RayMultithreadParameters> MultithreadParameters;
	std::mutex QueueMutex;
	std::condition_variable MutexCondition;
	bool ShouldTerminate = false;
};

inline void Raytracer::AddObject(Object* o)
{
    this->objects.push_back(o);
}

inline void Raytracer::SetViewMatrix(mat4 val)
{
    this->view = val;
    this->UpdateMatrices();
}
