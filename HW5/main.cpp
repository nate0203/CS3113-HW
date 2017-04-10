/*
	HW5 Simple SAT Collision Demo
	By: Nathan Ly

	W - Move Up, S - Move Down
	A - Move Left, D - Move Right
	LEFT Arrow - Rotate Left
	RIGHT Arrow - Rotate Right
*/
#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#include "Matrix.h"
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include <algorithm>
#define FIXED_TIMESTEP 0.016666f
#define MAX_TIMESTEPS 6
#include <math.h>

// Vector class - 2D collision
class Vector {
public:
	Vector() {
		x = 0.0f;
		y = 0.0f;
	}
	Vector(float x1, float y1) {
		x = x1;
		y = y1;
	}
	
	float length() const { return sqrtf(x*x + y*y); }
	void normalize() {
		if (length() != 0.0f) {
			x /= length();
			y /= length();
		}
	}

	float x, y;
};

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

// Collision functions
bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector> &points1, const std::vector<Vector> &points2, Vector &penetration) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	normalX /= len;
	normalY /= len;

	std::vector<float> e1Projected;
	std::vector<float> e2Projected;

	for (int i = 0; i < points1.size(); i++) {
		e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
	}
	for (int i = 0; i < points2.size(); i++) {
		e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
	}

	std::sort(e1Projected.begin(), e1Projected.end());
	std::sort(e2Projected.begin(), e2Projected.end());

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];

	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	float p = dist - ((e1Width + e2Width) / 2.0);

	if (p >= 0) {
		return false;
	}

	float penetrationMin1 = e1Max - e2Min;
	float penetrationMin2 = e2Max - e1Min;

	float penetrationAmount = penetrationMin1;
	if (penetrationMin2 < penetrationAmount) {
		penetrationAmount = penetrationMin2;
	}

	penetration.x = normalX * penetrationAmount;
	penetration.y = normalY * penetrationAmount;

	return true;
}

bool penetrationSort(const Vector &p1, const Vector &p2) {
	return p1.length() < p2.length();
}

bool checkSATCollision(const std::vector<Vector> &e1Points, const std::vector<Vector> &e2Points, Vector &penetration) {
	std::vector<Vector> penetrations;
	for (int i = 0; i < e1Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e1Points.size() - 1) {
			edgeX = e1Points[0].x - e1Points[i].x;
			edgeY = e1Points[0].y - e1Points[i].y;
		}
		else {
			edgeX = e1Points[i + 1].x - e1Points[i].x;
			edgeY = e1Points[i + 1].y - e1Points[i].y;
		}
		Vector penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}
	for (int i = 0; i < e2Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e2Points.size() - 1) {
			edgeX = e2Points[0].x - e2Points[i].x;
			edgeY = e2Points[0].y - e2Points[i].y;
		}
		else {
			edgeX = e2Points[i + 1].x - e2Points[i].x;
			edgeY = e2Points[i + 1].y - e2Points[i].y;
		}
		Vector penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);

		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}

	std::sort(penetrations.begin(), penetrations.end(), penetrationSort);
	penetration = penetrations[0];

	Vector e1Center;
	for (int i = 0; i < e1Points.size(); i++) {
		e1Center.x += e1Points[i].x;
		e1Center.y += e1Points[i].y;
	}
	e1Center.x /= (float)e1Points.size();
	e1Center.y /= (float)e1Points.size();

	Vector e2Center;
	for (int i = 0; i < e2Points.size(); i++) {
		e2Center.x += e2Points[i].x;
		e2Center.y += e2Points[i].y;
	}
	e2Center.x /= (float)e2Points.size();
	e2Center.y /= (float)e2Points.size();

	Vector ba;
	ba.x = e1Center.x - e2Center.x;
	ba.y = e1Center.y - e2Center.y;

	if ((penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
		penetration.x *= -1.0f;
		penetration.y *= -1.0f;
	}

	return true;
}

class Entity {
public:
	Entity(){}
	void checkCollision(Entity& other); // handles collision
	void Render(ShaderProgram* program); // draw
	void UpdateMatrix(); // gets the transformation matrix

	Vector pos;
	std::vector<Vector> corners; // in object space
	std::vector<Vector> worldCorners; // in world space
	float rotate;
	Vector scale;
	Vector vel;

	Matrix transform; // transformation matrix
};

void toWorldSpace(Entity& e); // turn object space to world space
void ReenterWorld(Entity& e); // loop around screen (off screen left back in on right, etc.)
void Setup();
void ProcessEvents();
void Update();
void Update(float elapsed);
void Render();

SDL_Window* displayWindow;
ShaderProgram* program;
Matrix projectionMatrix, viewMatrix, modelMatrix;
GLuint spriteTexture;
Entity player, rect1, rect2, rect3;
Vector penVector;
float lastFrameTicks = 0.0f;
const Uint8 *keys = SDL_GetKeyboardState(NULL);
SDL_Event event;
bool done = false;

int main(int argc, char *argv[]) {
	Setup();
	while (!done) {
		ProcessEvents();
		Update();
		Render();
	}
	delete program;
	SDL_Quit();
	return 0;
}

void toWorldSpace(Entity& e) {
	e.worldCorners.clear();

	// use transformation matrix to turn object space coordinates to worldspace coordinates
	for (size_t i = 0; i < e.corners.size(); ++i) {
		Vector tmp(e.corners[i].x * e.transform.m[0][0] + e.corners[i].y * e.transform.m[1][0] + e.transform.m[3][0],
			e.corners[i].x * e.transform.m[0][1] + e.corners[i].y * e.transform.m[1][1] + e.transform.m[3][1]);
		e.worldCorners.push_back(tmp);
	}
}

void Entity::checkCollision(Entity& other) {
	int maxChecks = 10;
	while (checkSATCollision(worldCorners, other.worldCorners, penVector) && maxChecks > 0) {
		// respond to collisions, half of penetration vector
		penVector.normalize();

		pos.x += penVector.x * 0.002f;
		pos.y += penVector.y * 0.002f;
		UpdateMatrix();
		toWorldSpace(*this);

		other.pos.x -= penVector.x * 0.002f;
		other.pos.y -= penVector.y * 0.002f;
		other.UpdateMatrix();
		toWorldSpace(other);

		maxChecks--;

		// continue some movement after resolving collision if applicable
		Vector splitMove(vel.x, vel.y);
		if (vel.x > 0.0f && other.vel.x >= 0.0f || vel.x < 0.0f && other.vel.x <= 0.0f)
			other.vel.x = splitMove.x;
		else if (vel.x > 0.0f && other.vel.x < 0.0f || vel.x < 0.0f && other.vel.x > 0.0f)
			other.vel.x = splitMove.x / 2.0f;

		if (vel.y > 0.0f && other.vel.y >= 0.0f || vel.y < 0.0f && other.vel.y <= 0.0f)
			other.vel.y = splitMove.y;
		if (vel.y > 0.0f && other.vel.y < 0.0f || vel.y < 0.0f && other.vel.y > 0.0f)
			other.vel.y = splitMove.y / 2.0f;
	}
}

void Entity::Render(ShaderProgram* program) {
	// set model matrix using transformation matrix
	UpdateMatrix();
	modelMatrix.identity();
	modelMatrix = modelMatrix * transform;
	program->setModelMatrix(modelMatrix);

	// render the rectangles
	float vertices[] = { -0.5f, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::UpdateMatrix() {
	// update the transformation matrix
	transform.identity();
	ReenterWorld(*this); // if the shape is outside of the bounds of the view matrix, reenter on the other side
	transform.Translate(pos.x, pos.y, 0.0f);
	transform.Rotate(rotate);
	transform.Scale(scale.x, scale.y, 0.0f);
}

void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	glUseProgram(program->programID);
	glViewport(0, 0, 640, 360);
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	spriteTexture = LoadTexture(RESOURCE_FOLDER"element_blue_square.png");

	// player initialization
	player.pos = Vector(-1.0f, -1.0f);
	player.rotate = 0.0f;
	player.scale = Vector(0.3f, 0.3f);
	player.corners = { Vector(-0.5f, -0.5f), Vector(-0.5f, 0.5f), Vector(0.5f, 0.5f), Vector(0.5f, -0.5f) };
	player.UpdateMatrix();
	toWorldSpace(player);

	// rectangle 1 initialization
	rect1.pos = Vector(0.0f, -0.5f);
	rect1.rotate = M_PI / 6.0f;
	rect1.scale = Vector(0.8f, 0.8f);
	rect1.corners = { Vector(-0.5f, -0.5f), Vector(-0.5f, 0.5f), Vector(0.5f, 0.5f), Vector(0.5f, -0.5f) };
	rect1.UpdateMatrix();
	toWorldSpace(rect1);

	// rectangle 2 initialization
	rect2.pos = Vector(0.0f, 1.0f);
	rect2.rotate = M_PI / 2.0f;
	rect2.scale = Vector(0.8f, 0.3f);
	rect2.corners = { Vector(-0.5f, -0.5f), Vector(-0.5f, 0.5f), Vector(0.5f, 0.5f), Vector(0.5f, -0.5f) };
	rect2.UpdateMatrix();
	toWorldSpace(rect2);

	// rectangle 3 initialization
	rect3.pos = Vector(-1.0f, 0.5f);
	rect3.rotate = M_PI * 1.5f;
	rect3.scale = Vector(0.5f, 0.7f);
	rect3.corners = { Vector(-0.5f, -0.5f), Vector(-0.5f, 0.5f), Vector(0.5f, 0.5f), Vector(0.5f, -0.5f) };
	rect3.UpdateMatrix();
	toWorldSpace(rect3);
}

void ProcessEvents() {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
	}
}

void Update() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;

	float fixedElapsed = elapsed;
	if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
		fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
	}
	while (fixedElapsed >= FIXED_TIMESTEP) {
		fixedElapsed -= FIXED_TIMESTEP;
		Update(FIXED_TIMESTEP);
	}
	Update(fixedElapsed);
}

void Update(float elapsed) {
	// player movement up and down
	if (keys[SDL_SCANCODE_W])
		player.vel.y = 1.0f;
	else if (keys[SDL_SCANCODE_S])
		player.vel.y = -1.0f;
	else
		player.vel.y = 0.0f;

	// player movement left and right
	if (keys[SDL_SCANCODE_A])
		player.vel.x = -1.0f;
	else if (keys[SDL_SCANCODE_D])
		player.vel.x = 1.0f;
	else
		player.vel.x = 0.0f;

	// player rotation
	if (keys[SDL_SCANCODE_LEFT])
		player.rotate += M_PI / 2.0f * elapsed;
	if (keys[SDL_SCANCODE_RIGHT])
		player.rotate -= M_PI / 2.0f * elapsed;

	// Update player position
	player.pos.x += player.vel.x * elapsed;
	player.pos.y += player.vel.y * elapsed;

	// rectangle 1 movement and rotation
	rect1.pos.x += rect1.vel.x * elapsed * 0.3f;
	rect1.pos.y += rect1.vel.y * elapsed * 0.3f;
	rect1.rotate += M_PI / 6.0f * elapsed;

	// rectangle 2 movement and rotation
	rect2.pos.x += rect2.vel.x * elapsed * 0.3f;
	rect2.pos.y += rect2.vel.y * elapsed * 0.3f;
	rect2.rotate -= M_PI / 3.0f * elapsed;

	// rectangle 3 movement and rotation
	rect3.pos.x += rect3.vel.x * elapsed * 0.3f;
	rect3.pos.y += rect3.vel.y * elapsed * 0.3f;
	rect3.rotate += M_PI / 4.0f * elapsed;

	// Update all transformation matrices and world space coordinates
	player.UpdateMatrix();
	toWorldSpace(player);
	rect1.UpdateMatrix();
	toWorldSpace(rect1);
	rect2.UpdateMatrix();
	toWorldSpace(rect2);
	rect3.UpdateMatrix();
	toWorldSpace(rect3);

	// collision checks
	player.checkCollision(rect1);
	player.checkCollision(rect2);
	player.checkCollision(rect3);
	rect1.checkCollision(rect2);
	rect1.checkCollision(rect3);
	rect2.checkCollision(rect3);
}

void ReenterWorld(Entity& e) {
	// check if all corners in world space coordinates are out of bounds
	int switchX = 0;
	int switchY = 0;
	for (size_t i = 0; i < e.worldCorners.size(); ++i) {
		if (e.worldCorners[i].x < -3.55f || e.worldCorners[i].x > 3.55f)
			switchX++;
		if (e.worldCorners[i].y < -2.0f || e.worldCorners[i].y > 2.0f)
			switchY++;
	}

	// place the shape back into the area on the opposite side
	if (switchX == 4) {
		if (e.worldCorners[0].x < -3.6f)
			e.pos.x = 3.7f;
		else if (e.worldCorners[0].x > 3.6f)
			e.pos.x = -3.7f;
		e.vel.x *= -1.0f;
	}

	if (switchY == 4) {
		if (e.worldCorners[0].y < -2.1f)
			e.pos.y = 2.1f;
		else if (e.worldCorners[0].y > 2.1f)
			e.pos.y = -2.1f;
		e.vel.y *= -1.0f;
	}
}

void Render() {
	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, spriteTexture);

	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	player.Render(program);
	rect1.Render(program);
	rect2.Render(program);
	rect3.Render(program);

	SDL_GL_SwapWindow(displayWindow);
}