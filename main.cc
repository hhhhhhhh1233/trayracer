#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <string>
#include "color.h"
#include "window.h"
#include "vec3.h"
#include "raytracer.h"
#include "sphere.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

#define degtorad(angle) angle * MPI / 180

/// Prints info with a box around it, helps highlight the important stuff
static void PrintAsBox(int BoxSize, std::vector<std::string> PrintStrings)
{
	for (auto& s : PrintStrings)
	{
		if (s.length() + 2 > BoxSize)
			BoxSize = s.length() + 2;
	}

	std::cout << " +" + std::string(BoxSize, '-') + "+\n";
	for (auto& s : PrintStrings)
	{
		std::cout << " | " << s << std::string(BoxSize - s.length() - 1, ' ') + "|\n";
	}
	std::cout << " +" + std::string(BoxSize, '-') + "+\n";
}

int main(int argc, char *argv[])
{ 
	// Default values that can be overriden by commandline arguments
	unsigned w = 300;
	unsigned h = 300;
    int raysPerPixel = 1;
    int maxBounces = 5;
	int spheresAmount = 36;
	bool multithread = false;
	unsigned int NumberOfJobs = 50;

	for (int i = 0; i < argc; i++)
	{
		if (std::string(argv[i]).compare("-w") == 0)
		{
			i++;
			w = std::stoi(argv[i]);
		}
		else if (std::string(argv[i]).compare("-h") == 0)
		{
			i++;
			h = std::stoi(argv[i]);
		}
		else if (std::string(argv[i]).compare("-wh") == 0)
		{
			i++;
			w = h = std::stoi(argv[i]);
		}
		else if (std::string(argv[i]).compare("-rpp") == 0)
		{
			i++;
			raysPerPixel = std::stoi(argv[i]);
		}
		else if (std::string(argv[i]).compare("-b") == 0)
		{
			i++;
			maxBounces = std::stoi(argv[i]);
		}
		else if (std::string(argv[i]).compare("-s") == 0)
		{
			i++;
			spheresAmount = std::stoi(argv[i]);
		}
		else if (std::string(argv[i]).compare("-m") == 0)
		{
			multithread = true;
		}
		else if (std::string(argv[i]).compare("-j") == 0)
		{
			i++;
			NumberOfJobs = std::stoi(argv[i]);
		}
	}

    std::vector<Color> framebuffer;

    framebuffer.resize(size_t(w * h));

    Raytracer rt = Raytracer(w, h, framebuffer, raysPerPixel, maxBounces);

    // Create some objects
	Material* mat = new Material();
	mat->type = "Lambertian";
	mat->color = { 0.5,0.5,0.5 };
	mat->roughness = 0.3;
	Sphere* ground = new Sphere(1000, { 0,-1000, -1 }, mat);
	rt.AddObject(ground);

	std::vector<std::string> Types = {"Lambertian", "Dielectric", "Conductor"};
	std::vector<float> Spans = {10, 30, 25};

    for (int it = 0; it < spheresAmount; it++)
    {
		Material* mat = new Material();
		mat->type = Types[it % 3];
		if (it % 3 == 1)
			mat->refractionIndex = 1.65;
		float r = RandomFloat();
		float g = RandomFloat();
		float b = RandomFloat();
		mat->color = { r,g,b };
		mat->roughness = RandomFloat();
		const float span = Spans[it % 3];
		Sphere* ground = new Sphere(
			RandomFloat() * 0.7f + 0.2f,
			{
				RandomFloatNTP() * span,
				RandomFloat() * span + 0.2f,
				RandomFloatNTP() * span
			},
			mat);
		rt.AddObject(ground);
	}
    
    // camera
    vec3 camPos = { 0,1.0f,10.0f };

    float rotx = 0;
    float roty = 0;

    {
        mat4 xMat = (rotationx(rotx));
        mat4 yMat = (rotationy(roty));
        mat4 cameraTransform = multiply(yMat, xMat);

        
        cameraTransform.m30 = camPos.x;
        cameraTransform.m31 = camPos.y;
        cameraTransform.m32 = camPos.z;

        rt.SetViewMatrix(cameraTransform);
        
		int NumberOfRays;
		auto start = std::chrono::high_resolution_clock::now();

		if (multithread)
			NumberOfRays = rt.RaytraceMultithreaded(NumberOfJobs);
		else
			NumberOfRays = rt.Raytrace();

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

		PrintAsBox(40, {
			"TRAYRACER INFO", "",
			std::string("Multithreaded: ").append(multithread ? "True" : "False"),
			"Time " + std::to_string(duration.count()/1000.0f),
			"Number of Rays: " + std::to_string(NumberOfRays),
			"MRays/s: " + std::to_string((NumberOfRays/1'000'000.0f)/(duration.count()/1000.0f)),
			"Resolution: " + std::to_string(w) + "x" + std::to_string(h),
			"Rays Per Pixel: " + std::to_string(raysPerPixel),
			"Max Bounces: " + std::to_string(maxBounces),
			"Number of Sphere: " + std::to_string(spheresAmount),
		});

		std::vector<uint8_t> framebufferInt;

		for (int y = h - 1; y >= 0; y--)
		{
			for (int x = 0; x < w; x++)
			{
				framebufferInt.push_back(std::clamp(int(framebuffer[size_t(w * y + x)].r * 255), 0, 255));
				framebufferInt.push_back(std::clamp(int(framebuffer[size_t(w * y + x)].g * 255), 0, 255));
				framebufferInt.push_back(std::clamp(int(framebuffer[size_t(w * y + x)].b * 255), 0, 255));
			}
		}

		stbi_write_png("Frame.png", w, h, 3, framebufferInt.data(), w*3);
    }

    return 0;
} 
