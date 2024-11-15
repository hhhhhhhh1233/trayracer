#include "raytracer.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <functional>
#include "random.h"

//------------------------------------------------------------------------------
/**
*/
Raytracer::Raytracer(unsigned w, unsigned h, std::vector<Color>& frameBuffer, unsigned rpp, unsigned bounces) :
    frameBuffer(frameBuffer),
    rpp(rpp),
    bounces(bounces),
    width(w),
    height(h),
    frustum(mat4()),
    view(mat4())
{
    StartThreads();
}

Raytracer::~Raytracer() {
	StopThreads();
}

//------------------------------------------------------------------------------
/**
*/
unsigned int
Raytracer::Raytrace()
{
    //static int leet = 1337;
    //std::mt19937 generator (leet++);
    //std::uniform_real_distribution<float> dis(0.0f, 1.0f);

	unsigned int NumberOfTraces = 0;
    for (int x = 0; x < this->width; ++x)
    {
        for (int y = 0; y < this->height; ++y)
        {
            Color color;
            for (int i = 0; i < this->rpp; ++i)
            {
                float u = ((float(x + RandomFloat()) * (1.0f / this->width)) * 2.0f) - 1.0f;
                float v = ((float(y + RandomFloat()) * (1.0f / this->height)) * 2.0f) - 1.0f;

                vec3 direction = vec3(u, v, -1.0f);
                direction = transform(direction, this->frustum);
                
                Ray ray = Ray(get_position(this->view), direction);
                color += this->TracePathNoRecursion(ray, this->bounces);
                //color += this->TracePath(ray, 0);

				NumberOfTraces++;
            }

            // divide by number of samples per pixel, to get the average of the distribution
            color.r /= this->rpp;
            color.g /= this->rpp;
            color.b /= this->rpp;

            this->frameBuffer[y * this->width + x] += color;
        }
    }
	return NumberOfTraces;
}

unsigned int
Raytracer::RaytraceMultithreaded(unsigned int NumberOfJobs)
{
    std::mutex zLock;
    std::atomic<int> z(0);

    DoneThreads.store(0);

    for (int i = 0; i < NumberOfJobs; i++)
    {
        QueueJob(RayMultithreadParameters((this->width / NumberOfJobs) * i, (this->width / NumberOfJobs) * (i + 1), 1337 + i));
    }

    while (DoneThreads < NumberOfJobs) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return this->width * this->height * this->rpp;
}

void Raytracer::RaytraceChunk(RayMultithreadParameters Param)
{
    int MinY = Param.MinY;
    int MaxY = Param.MaxY;

    //static int leet = 1337;
    //std::mt19937 generator (leet++);
    std::mt19937 generator (Param.RandomSeed);
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

	for (int x = 0; x < this->width; ++x)
	{
		for (int y = MinY; y < MaxY; ++y)
		{
			Color color;
			for (int i = 0; i < this->rpp; ++i)
			{
				float u = ((float(x + dis(generator)) * (1.0f / this->width)) * 2.0f) - 1.0f;
				float v = ((float(y + dis(generator)) * (1.0f / this->height)) * 2.0f) - 1.0f;
				//float u = ((float(x + RandomFloat()) * (1.0f / this->width)) * 2.0f) - 1.0f;
				//float v = ((float(y + RandomFloat()) * (1.0f / this->height)) * 2.0f) - 1.0f;

				vec3 direction = vec3(u, v, -1.0f);
				direction = transform(direction, this->frustum);

				Ray ray = Ray(get_position(this->view), direction);
				color += this->TracePathNoRecursion(ray, this->bounces);
				//color += this->TracePath(ray, 0);
			}

			// divide by number of samples per pixel, to get the average of the distribution
			color.r /= this->rpp;
			color.g /= this->rpp;
			color.b /= this->rpp;

			this->frameBuffer[y * this->width + x] += color;
		}
	}
    DoneThreads.fetch_add(1);
}

//------------------------------------------------------------------------------
/**
 * @parameter n - the current bounce level
*/
Color
Raytracer::TracePathNoRecursion(Ray ray, unsigned n)
{
    vec3 hitPoint;
    vec3 hitNormal;
    Object* hitObject = nullptr;
    float distance = FLT_MAX;

    Color color = { 1,1,1 };

    Ray CurrentRay = ray;

    for (int i = 0; i < this->bounces; i++)
    {
        if (Raycast(CurrentRay, hitPoint, hitNormal, hitObject, distance, this->objects))
        {
			color = color * hitObject->GetColor();
			CurrentRay = Ray(hitObject->ScatterRay(CurrentRay, hitPoint, hitNormal));
        }
        else
        {
			color = color * this->Skybox(CurrentRay.m);
            break;
        }

        if (i == this->bounces)
            return { 0,0,0 };
    }

    return color;
}

Color
Raytracer::TracePath(Ray ray, unsigned n)
{
    vec3 hitPoint;
    vec3 hitNormal;
    Object* hitObject = nullptr;
    float distance = FLT_MAX;

    if (Raycast(ray, hitPoint, hitNormal, hitObject, distance, this->objects))
    {
        Ray scatteredRay = Ray(hitObject->ScatterRay(ray, hitPoint, hitNormal));
        if (n < this->bounces)
        {
            return hitObject->GetColor() * this->TracePath(scatteredRay, n + 1);
        }

        if (n == this->bounces)
        {
            return {0,0,0};
        }
    }

    return this->Skybox(ray.m);
}

//------------------------------------------------------------------------------
/**
*/
bool
Raytracer::Raycast(Ray ray, vec3& hitPoint, vec3& hitNormal, Object*& hitObject, float& distance, std::vector<Object*> const& world)
{
    bool isHit = false;
    HitResult closestHit;
    int numHits = 0;
    HitResult hit;

    for (auto& obj : world)
    {
        HitResult opt = obj->Intersect(ray, closestHit.t);
        if (opt.object)
        {
            hit = opt;
            assert(hit.t < closestHit.t);
            closestHit = hit;
            closestHit.object = obj;
            isHit = true;
            numHits++;
        }
    }

    hitPoint = closestHit.p;
    hitNormal = closestHit.normal;
    hitObject = closestHit.object;
    distance = closestHit.t;
    
    return isHit;
}


//------------------------------------------------------------------------------
/**
*/
void
Raytracer::Clear()
{
    for (auto& color : this->frameBuffer)
    {
        color.r = 0.0f;
        color.g = 0.0f;
        color.b = 0.0f;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Raytracer::UpdateMatrices()
{
    mat4 inverseView = inverse(this->view); 
    mat4 basis = transpose(inverseView);
    this->frustum = basis;
}

//------------------------------------------------------------------------------
/**
*/
Color
Raytracer::Skybox(vec3 direction)
{
    float t = 0.5*(direction.y + 1.0);
    vec3 vec = vec3(1.0, 1.0, 1.0) * (1.0 - t) + vec3(0.5, 0.7, 1.0) * t;
    return {(float)vec.x, (float)vec.y, (float)vec.z};
}

void 
Raytracer::StopThreads()
{
	{
		std::unique_lock<std::mutex> lock(QueueMutex);
		ShouldTerminate = true;
	}
	MutexCondition.notify_all();
	JoinAllThreads();
}

void 
Raytracer::JoinAllThreads()
{
	for (auto& Thread : Threads)
	{
		Thread.join();
	}
	Threads.clear();
}

void 
Raytracer::StartThreads()
{
	const int ThreadsNum = std::thread::hardware_concurrency();
	for (int i = 0; i < ThreadsNum; i++)
	{
		Threads.emplace_back(std::thread(&Raytracer::ThreadLoop, this));
	}
}

void 
Raytracer::ThreadLoop()
{
	while (true)
	{
		RayMultithreadParameters Params;
		{
			std::unique_lock<std::mutex> lock(QueueMutex);
			MutexCondition.wait(lock, [this]{
				return !MultithreadParameters.empty() || ShouldTerminate;
			});
			if (ShouldTerminate)
				return;

			Params = MultithreadParameters.front();
			MultithreadParameters.pop();
		}
		RaytraceChunk(Params);
	}
}

void 
Raytracer::QueueJob(RayMultithreadParameters Params)
{
	{
		std::unique_lock<std::mutex> lock(QueueMutex);
		MultithreadParameters.push(Params);
	}
	MutexCondition.notify_one();
}
