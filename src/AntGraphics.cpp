    #include "AntGraphics.h"

   // Draw text related to the current state of the ant
    void AntGraphics::drawText(shared_ptr<Ant>& ant,int currInt) { 
        string ACOAnt = "ACO On Ant: " + to_string(ant->id);
        string currIteration = "Iteration: " + to_string(currInt) + " / " + to_string(iterations);

        DrawText(ACOAnt.c_str(), 10, 10, 20, DARKGRAY); 
        DrawText(currIteration.c_str(), 10, 30, 20, DARKGRAY);
        
        string antsRoute = "Current Ant Route: [";
        for (auto city : ant->route) {
            string cityText = to_string(city->id);
            antsRoute += cityText + ", ";
        }
        antsRoute += "]";
        DrawText(antsRoute.c_str(), 10, HEIGHT/1.5, 20, DARKGRAY);
    }

    // Set the current ant for rendering
    void AntGraphics::setAnt(shared_ptr<Ant> newAnt) {
        currAnt = newAnt;
        currCity = newAnt->currCity;
    }
    
    // Get current position of the ant
    Vector2 AntGraphics::getPosition() const {
        return currAnt->position;
    }

    // Set position of the ant
    void AntGraphics::setPosition(const Vector2& pos) {
        currAnt->position = pos;
    }

    // Update ant's position and state based on time delta
    void AntGraphics::Update(float delta) {
	// Clmap in case of frame go bad
	if(delta > 1.0f/20.0f)
		delta = 1.0f/20.f;

        currCity = currAnt->currCity;
        moveToNextPoint(delta);
    }

    // Render the full scene including cities and ant
    void AntGraphics::RenderScene() {
        RenderCityGraph();
        DrawAnt();
        DrawMatrices();
    }

    // Check if the ant has reached its target
    bool AntGraphics::reachedTarget() {
        Vector2 target = currCity->position;
        Vector2 direction = {target.x - currAnt->position.x, target.y - currAnt->position.y};
        float length = sqrt(pow(direction.x, 2.0f) + pow(direction.y, 2.0f));
        return length < 2;
    }

    // Restart Button in Development
    /* 
    void AntGraphics::restart(){

      int buttonWidth = 60;
      int buttonHeight = 40;
      int buttonX = 30;
      int buttonY = (HEIGHT / 1.5) + 30;

      DrawRectangleLines(buttonX, buttonY, buttonWidth, buttonHeight, BLACK);

      DrawText("Restart",buttonX + (buttonWidth/2),buttonY + (buttonHeight/2), 10, RED);
      
    }
    */
    
    // Check and validate the resource paths and availability
    bool AntGraphics::checkAndValidateResources() {
        string directoryPath = "./resources/";
        string antImagePath = directoryPath + "ant.png";

        if (!checkDirectoryExists(directoryPath) || !checkFileExists(antImagePath)) {
            return false;
        }

        return true;
    }

    // Load the ant image from resources
    Image AntGraphics::loadAntImage() {
        string antImagePath = "./resources/ant.png";
        Image fullAntImage = LoadImage(antImagePath.c_str());

        if (!fullAntImage.data) {
            throw runtime_error("Failed to load Image");
        }

        Rectangle section = {17, 320, 52, 64};
        Image antImage = ImageFromImage(fullAntImage, section);

        if (!antImage.data) {
            throw runtime_error("Failed to load image section");
        }

        return antImage;
    }

    // Prepare the texture from image for rendering
    void AntGraphics::prepareTexture(const Image& antImage) {
        Image antResized = antImage;
        ImageResize(&antResized, (52 / 3), (64 / 3)); // Resizing the ant image
        ImageColorReplace(&antResized, WHITE, RAYWHITE);

        antTexture = LoadTextureFromImage(antResized);
        if (antTexture.id == 0) {
            throw runtime_error("Failed to load texture from image.");
        }
    }
    
    // Move the ant towards the next point
    void AntGraphics::moveToNextPoint(float delta) {
        Vector2 target = currCity->position;
	Vector2 toTarget = {target.x - currAnt->position.x, target.y - currAnt->position.y};
	float dist = lengthVec(toTarget);

	// Speed slows as we enter the slowRadius
	float desiredSpeed = maxSpeed;
	if(dist < slowRadius)
		desiredSpeed = maxSpeed * (dist / slowRadius);
	
	Vector2 desiredVel = {0,0};
	if(dist > 0.0001f) {
		Vector2 dir = normVec(toTarget);
		desiredVel = {dir.x * desiredSpeed, dir.y * desiredSpeed};
	}

	// Steering = change in velocity, limited by maxAccel
	Vector2 steer = {desiredVel.x - velocity.x, desiredVel.y - velocity.y};
	steer = clampMag(steer,maxAccel * delta);

	// Integrate velocity and position
	velocity = {velocity.x + steer.x, velocity.y + steer.y};
	currAnt->position = {currAnt->position.x + velocity.x * delta,
			     currAnt->position.y + velocity.y * delta};

	// Stop cleanly when inside stopRadius and nearly stopped
	if(dist < stopRadius && lengthVec(velocity) < 10.0f){
		currAnt->position = target;
		velocity = {0,0};
	}


	// Smoothy rotate to face velocity (if moving)
	float targetRot = currentRotation;
	float vlen = lengthVec(velocity);
	if(vlen > 0.1f){
		targetRot = atan2f(velocity.y, velocity.x) * (180.0f / (float)PI) + 90.0f;
	}

	// Exponentialish smoothing: larger dt -> bigger catch u 
	float rotT = 1.0f - powf(0.0001f,delta); // tweak factor (0.0001-0.01)
	currentRotation = lerpAngleDeg(currentRotation, targetRot, rotT);

    }
   
    // Render the graph of cities
    void AntGraphics::RenderCityGraph() {
        float size = (WIDTH / (20 * cities.size()));
        for(const auto& city : cities) {
            DrawCircle(city->position.x, city->position.y, size, BLUE);
            DrawText(TextFormat("%d", city->id), city->position.x, city->position.y - (size * 2), size, BLACK);
        }

        for (size_t i = 0; i < cities.size() - 1; ++i) {
            for (size_t j = i + 1; j < cities.size(); ++j) {
                DrawLineEx(cities[i]->position, cities[j]->position, pheromones[i][j] * size, PATHGREEN);
            }
        }
    }

    // Draw matrices related to proximity, pheromones, and probabilities
    void AntGraphics::DrawMatrices() {
        const int cellSize = (WIDTH - 40) / (cities.size() * 2.15); 
        const float margin = cellSize / 10;
        for (size_t i = 0; i < proximitys.size(); ++i) {
            for (size_t j = 0; j < proximitys[i].size(); ++j) {
                if (i > j) {
                    float startX = (WIDTH / 2.0f) + i * cellSize;
                    float startY = (50) + j * cellSize;

                    if (j == 0) {
                        DrawText(TextFormat("%d", i), startX + (cellSize * .5), startY - (margin * 1.1), margin, BLACK);
                    }

                    if (i == proximitys.size() - 1) {
                        DrawText(TextFormat("%d", j), startX + (cellSize * 1.1), startY + (cellSize * .5), margin, BLACK);
                    }

                    DrawRectangleLines(startX, startY, cellSize, cellSize, BLACK);
                    DrawMatrixElements(startX, startY, i, j, cellSize, margin);
                }
            }
        }
    }
    
    // Draw individual matrix elements
    void AntGraphics::DrawMatrixElements(float x, float y, size_t i, size_t j, const int cellSize, const float margin) {
        DrawText(TextFormat("Proximity:\n\t %.3f", proximitys[i][j]), x + 5, y + margin, margin, BLACK);
        DrawText(TextFormat("Pheromones:\n\t %.3f", pheromones[i][j]), x + 5, y + 4 * margin, margin, GREEN);
        DrawText(TextFormat("Probablity:\n\t %.3f", probablitys[i][j]), x + 5, y + 8 * margin, margin, RED);
    }



    // Draw the ant at its current position
    void AntGraphics::DrawAnt() const {

        float scale = 1.0f; 
        if(cities.size() < 5.0f){ // Change size of the ant based on how many cities there are
          scale = 15.0f / (float)cities.size();
          
        }

        float antTexWidth = (float)antTexture.width * scale;
        float antTexHeight = (float)antTexture.height * scale; 
        Vector2 origin = {antTexWidth * 0.5f, antTexHeight * 0.5f};
        
        Rectangle sourceRec = {0.0f, 0.0f, (float)antTexture.width, (float)antTexture.height};
        Rectangle destRec = {currAnt->position.x, currAnt->position.y, antTexWidth, antTexHeight};

        DrawTexturePro(antTexture, sourceRec, destRec, origin, currentRotation, WHITE);
    }

    // Check if a file exists
    bool AntGraphics::checkFileExists(const std::string& pathIn) {
        return fs::exists(pathIn) && fs::is_regular_file(pathIn);
    }

    // Check if a directory exists
    bool AntGraphics::checkDirectoryExists(const std::string& pathIn) {
        return fs::exists(pathIn) && fs::is_directory(pathIn);
    }
