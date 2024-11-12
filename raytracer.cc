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
    // empty
    Pool.Start();
}

//------------------------------------------------------------------------------
/**
*/
unsigned int
Raytracer::Raytrace()
{
    std::cout << "This is NOT multithreaded!\n";

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
                color += this->TracePath(ray, 0);

                //color += this->TracePath(Ray(get_position(this->view), transform(vec3(u, v, -1.0f), this->frustum)), 0);

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
    std::cout << "This is multithreaded!\n";

    std::mutex zLock;
    std::atomic<int> z(0);

    std::atomic<int> DoneThreads(0);

    for (int i = 0; i < NumberOfJobs; i++)
    {
		Pool.QueueJob([this, &zLock, &z, &DoneThreads, NumberOfJobs]() {
            int minX, maxX;

            {
                std::unique_lock<std::mutex> lock(zLock);
                z++;
                minX = (this->width / NumberOfJobs) * (z - 1);
                maxX = (this->width / NumberOfJobs) * (z);
            }

			for (int x = minX; x < maxX; ++x)
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
						color += this->TracePath(ray, 0);
					}

					// divide by number of samples per pixel, to get the average of the distribution
					color.r /= this->rpp;
					color.g /= this->rpp;
					color.b /= this->rpp;

					this->frameBuffer[y * this->width + x] += color;
				}
			}

            DoneThreads.fetch_add(1);
		});
    }

    //std::cout << "Threads: " << Pool.Threads.size() << "\n";
    //std::function<void()> Job = Pool.PopJob();
    //Job();

    while (DoneThreads < NumberOfJobs) {}

	return this->width * this->height * this->rpp;
}

//------------------------------------------------------------------------------
/**
 * @parameter n - the current bounce level
*/
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
Raytracer::Raycast(Ray ray, vec3& hitPoint, vec3& hitNormal, Object*& hitObject, float& distance, std::vector<Object*> world)
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
