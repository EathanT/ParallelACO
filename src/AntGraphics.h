#ifndef ANTGRAPHICS_H
#define ANTGRAPHICS_H

#include "Ant.h"

// Constants
#define WIDTH 2000
#define HEIGHT 1000
static constexpr Color PATHGREEN{ 0, 158, 47, 175 }; // Path Color

namespace fs = std::filesystem;

struct Point {
    float x, y;
};


// Class responsible for rendering graphics of the ants and the city graph
class AntGraphics {
	public:
		// Constructor that initializes the graphical representation of the ants			
         AntGraphics(vector<shared_ptr<Ant>>& antRefs, vector<vector<float>>& pheromonesIn, vector<vector<float>>& proximitysIn,
                vector<vector<float>>& probablitysIn, vector<shared_ptr<city>> citiesIn, float simSpeed, int totalIterations)
        : ants(antRefs),
          pheromones(pheromonesIn),
          proximitys(proximitysIn),
          probablitys(probablitysIn),
          cities(citiesIn),
          speed(simSpeed),
          iterations(totalIterations){

        if (!checkAndValidateResources()) {
            throw runtime_error("Resource validation failed");
        }
       
        Image antImage = loadAntImage();
        prepareTexture(antImage);
        }
    
        ~AntGraphics() {
            UnloadTexture(antTexture);
        }

        
        // Draw text related to the current state of the ant
        void drawText(shared_ptr<Ant>& ant,int currInt);

        // Set the current ant for rendering
        void setAnt(shared_ptr<Ant> newAnt);

        // Get current position of the ant
        Vector2 getPosition() const;

        // Set position of the ant
        void setPosition(const Vector2& pos);

        // Update ant's position and state based on time delta
        void Update(float delta);

        // Render the full scene including cities and ant
        void RenderScene();

        // Check if the ant has reached its target
        bool reachedTarget();

        // reset all values Button
       /* void restart(); */ // in development

        float speed{500.0f}; // Simulation speed
    private:
        float currentRotation{0.0f};
        shared_ptr<Ant> currAnt;
        shared_ptr<city> currCity;
        vector<shared_ptr<Ant>>& ants;  
        vector<vector<float>>& pheromones;
        vector<vector<float>>& proximitys;
        vector<vector<float>>& probablitys;
        vector<shared_ptr<city>> cities;
        Texture2D antTexture;
        int iterations;
    	Vector2 velocity{0.0f, 0.0f};
    	float maxSpeed = 500.0f; // units per second
    	float maxAccel = 2000.0f; // host fast velocity can change
    	float slowRadius = 80.0f; // start slowing down when within this many pixels
    	float stopRadius = 3.0f; // consider reached when within this distance
	
    	static float lengthVec(Vector2 v){
    		return sqrtf(v.x*v.x + v.y*v.y);
    	}

    	static Vector2 normVec(Vector2 v){
    		float L = lengthVec(v);
    		return (L>0.0001f) ? Vector2{v.x/L, v.y/L} : Vector2{0,0};
	    }
    
	    static Vector2 clampMag(Vector2 v, float maxMag){
	    	float L = lengthVec(v);
	    	if (L > maxMag && L > 0.0f){
		    	float s = maxMag/L;
	    		return Vector2{v.x*s, v.y*s};
	    	}
	    	return v;
    	}


      	// Angle helpers for smooth rotating
    	static float wrapAngleDeg(float a){
	    	while(a <=-180)
	    		a+= 360;
	    	while(a > 180)
	    		a-=360;

	    	return a;
    	}

    	static float lerp(float a, float b, float t){
    		return a + (b-a) * t;
    	}

    	static float lerpAngleDeg(float a,float b, float t){
    		float d = wrapAngleDeg(b - a);
		
	    	return a + d * t;
    	}
    
        // Check and validate the resource paths and availability
        bool checkAndValidateResources();

        // Load the ant image from resources
        Image loadAntImage();

        // Prepare the texture from image for rendering
        void prepareTexture(const Image& antImage);

        // Move the ant towards the next point
        void moveToNextPoint(float delta);

        // Move the ant in the specified direction
        void move(const Vector2& direction, float delta,Vector2 target);

        // Render the graph of cities
        void RenderCityGraph();

        // Draw matrices related to proximity, pheromones, and probabilities
        void DrawMatrices();

        // Draw individual matrix elements
        void DrawMatrixElements(float x, float y, size_t i, size_t j, const int cellSize, const float margin);

        // Draw the ant at its current position
        void DrawAnt() const;

        // Check if a file exists
        bool checkFileExists(const std::string& pathIn);

        // Check if a directory exists
        bool checkDirectoryExists(const std::string& pathIn);
 
};

#endif
