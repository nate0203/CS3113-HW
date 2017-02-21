/*
	HW2.cpp, CS3113 Pong Game
	User 1 (Left/Red Paddle): W - up, S - down
	User 2 (Blue/Right Paddle): UP arrow - up, DOWN arrow -down
	Notes: 
	If the ball hits the top of the paddle, current round is still good.
	All movement by the ball is at a 45 degree angle when hit anywhere on the paddle.
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

#include <math.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
SDL_Window* displayWindow;

class Entity {
public:
	float x;
	float y;

	float width;
	float height;

	float speed;
	float direction_x;
	float direction_y;
};

bool collided(Entity& ball, Entity& object) { // collision detection for 4 cases
	if (object.x + object.width / 2.0f < ball.x - ball.width / 2.0f)
		return false;
	if (object.x - object.width / 2.0f > ball.x + ball.width / 2.0f)
		return false;
	if ((object.y + object.height / 2.0f) < ball.y - ball.height / 2.0f)
		return false;
	if (object.y - object.height / 2.0f > ball.y + ball.height / 2.0f)
		return false;
	return true;
}

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

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
	glewInit();
	#endif
	
	Matrix modelMatrix, projectionMatrix, viewMatrix;
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint ballTexture = LoadTexture(RESOURCE_FOLDER"green_button09.png");
	GLuint wallTexture = LoadTexture(RESOURCE_FOLDER"brickWall.png");
	GLuint l_paddleTexture = LoadTexture(RESOURCE_FOLDER"paddleRed.png");
	GLuint r_paddleTexutre = LoadTexture(RESOURCE_FOLDER"paddleBlu.png");
	glUseProgram(program.programID);
	glViewport(0, 0, 640, 360);
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	const Uint8 *keys = SDL_GetKeyboardState(NULL); // for keyboard polling
	float lastFrameTicks = 0.0f;
	float win = 0.5f; // counter used for color change when point scored
	Entity ball, l_pad, r_pad, botWall, topWall; // objects on pong board

	// values for ball, will automatically move
	ball.height = 0.2f;
	ball.width = 0.2f;
	ball.x = 0.0f;
	ball.y = 0.0f;
	ball.direction_x = (float)cos(45 * 3.14159f / 180.0f);
	ball.direction_y = (float)sin(45 * 3.14159f / 180.0f);
	ball.speed = 4.5f;

	// values for left wall/paddle, controlled by user
	l_pad.height = 0.8f;
	l_pad.width = 0.15f;
	l_pad.x = -3.4f;
	l_pad.y = 0.0f;
	l_pad.speed = 1.1f;

	// values for right wall/paddle, controlled by another user
	r_pad.height = 0.8f;
	r_pad.width = 0.15f;
	r_pad.x = 3.4f;
	r_pad.y = 0.0f;
	r_pad.speed = 1.1f;

	// values for bottom wall, static object
	botWall.height = 0.2f;
	botWall.width = 7.1f;
	botWall.x = 0.0f;
	botWall.y = -1.9f;

	// values for top wall, static object
	topWall.height = 0.2f;
	topWall.width = 7.1f;
	topWall.x = 0.0f;
	topWall.y = 1.9f;
	
	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		/*
			POLLING FOR PLAYER 1, W key to move up, S key to move down
		*/
		if (keys[SDL_SCANCODE_W]) {
			if (l_pad.y + elapsed * 1.1f < 1.4f)
				l_pad.y += elapsed * l_pad.speed;
			else
				l_pad.y = 1.4f;
		}
		if (keys[SDL_SCANCODE_S]) {
			if (l_pad.y - elapsed * 1.1f > -1.4f)
				l_pad.y -= elapsed * l_pad.speed;
			else
				l_pad.y = -1.4f;
		}
		/*
			POLLING FOR PLAYER 2, UP arrow to move up, DOWN arrow to move down
		*/
		if (keys[SDL_SCANCODE_UP]) {
			if (r_pad.y + elapsed * 1.1f < 1.4f)
				r_pad.y += elapsed * r_pad.speed;
			else
				r_pad.y = 1.4f;
		}
		if (keys[SDL_SCANCODE_DOWN]) {
			if (r_pad.y - elapsed * 1.1f > -1.4f)
				r_pad.y -= elapsed * r_pad.speed;
			else
				r_pad.y = -1.4f;
		}

		// Ball is off screen, player has won, reset game
		if (ball.x > 4.0f || ball.x < -4.0f) {
			if (ball.x > 4.0f)
				glClearColor(1.0f, 0.7f, 0.7f, 0.5f); // W/S keys user wins
			else
				glClearColor(0.0f, 0.7f, 0.9f, 0.5f); // UP/DOWN arrow user wins
			ball.x = 0.0f;
			ball.y = 0.0f;
			l_pad.y = 0.0f;
			r_pad.y = 0.0f;
			win = 0.0f;
		}

		/* 
			Collision Detection
			Checks ball's next position and compares to the 4 objects
			Ball's location is changed if there is a collision
		*/
		ball.x += ball.direction_x * elapsed * ball.speed;
		ball.y += ball.direction_y * elapsed * ball.speed;
		if (collided(ball, topWall)) {
			ball.direction_y = -1 * (float)sin(45 * 3.14159f / 180.0f);
			ball.y = topWall.y - topWall.height / 2.0f - ball.height / 2.0f + ball.direction_y * elapsed * ball.speed;
		}
		if (collided(ball, botWall)) {
			ball.direction_y = (float)sin(45 * 3.14159f / 180.0f);
			ball.y = botWall.y + botWall.height / 2.0f + ball.height / 2.0f + ball.direction_y * elapsed * ball.speed;
		}
		
		if (collided(ball, r_pad) && ball.x <= r_pad.x) {
			ball.direction_x = -1 * (float)cos(45 * 3.14159f / 180.0f);
			ball.x = r_pad.x - r_pad.width / 2.0f - ball.width / 2.0f + ball.direction_x * elapsed * ball.speed;
		}
		if (collided(ball, l_pad) && ball.x >= l_pad.x) {
			ball.direction_x = (float)cos(45 * 3.14159f / 180.0f);
			ball.x = l_pad.x + l_pad.width / 2.0f + ball.width / 2.0f + ball.direction_x * elapsed * ball.speed;
		}

		if (win < 0.5f)
			win += elapsed;
		else
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // reset background color after some elapsed time
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glClear(GL_COLOR_BUFFER_BIT);

		// Render the halfcourt line
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		glBindTexture(GL_TEXTURE_2D, wallTexture);
		float halfcourt[] = { -0.05f, 1.8f, -0.05f, -1.8f, 0.05f, 1.8f, 0.05f, 1.8f, -0.05f, -1.8f, 0.05f, -1.8f };
		float texCoords[] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, halfcourt);
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.positionAttribute);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Render the topWall
		modelMatrix.identity();
		modelMatrix.Translate(topWall.x, topWall.y, 0.0f);
		program.setModelMatrix(modelMatrix);
		float tWall[] = { -topWall.width / 2.0f, topWall.height / 2.0f,
						-topWall.width / 2.0f, -topWall.height / 2.0f,
						topWall.width / 2.0f, topWall.height / 2.0f,
						topWall.width / 2.0f, topWall.height / 2.0f,
						-topWall.width / 2.0f, -topWall.height / 2.0f,
						topWall.width / 2.0f, -topWall.height / 2.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, tWall);
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Render the bottom wall
		modelMatrix.identity();
		modelMatrix.Translate(botWall.x, botWall.y, 0.0f);
		program.setModelMatrix(modelMatrix);
		float bWall[] = { -botWall.width / 2.0f, botWall.height / 2.0f,
						-botWall.width / 2.0f, -botWall.height / 2.0f,
						botWall.width / 2.0f, botWall.height / 2.0f,
						botWall.width / 2.0f, botWall.height / 2.0f,
						-botWall.width / 2.0f, -botWall.height / 2.0f,
						botWall.width / 2.0f, -botWall.height / 2.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, bWall);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Render the left paddle/wall
		modelMatrix.identity();
		modelMatrix.Translate(l_pad.x, l_pad.y, 0.0f);
		program.setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, l_paddleTexture);
		float l_paddle[] = { -l_pad.width / 2.0f, l_pad.height / 2.0f,
							-l_pad.width / 2.0f, -l_pad.height / 2.0f,
							l_pad.width / 2.0f, l_pad.height / 2.0f,
							l_pad.width / 2.0f, l_pad.height / 2.0f,
							-l_pad.width / 2.0f, -l_pad.height / 2.0f,
							l_pad.width / 2.0f, -l_pad.height / 2.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, l_paddle);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		// Render the right paddle/wall
		modelMatrix.identity();
		modelMatrix.Translate(r_pad.x, r_pad.y, 0.0f);
		program.setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, r_paddleTexutre);
		float r_paddle[] = { - r_pad.width / 2.0f, r_pad.height / 2.0f,
							- r_pad.width / 2.0f, -r_pad.height / 2.0f,
							r_pad.width / 2.0f, r_pad.height / 2.0f,
							r_pad.width / 2.0f, r_pad.height / 2.0f,
							- r_pad.width / 2.0f, - r_pad.height / 2.0f,
							r_pad.width / 2.0f, - r_pad.height / 2.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, r_paddle);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		// Render the pong ball
		modelMatrix.identity();
		modelMatrix.Translate(ball.x, ball.y, 0.0f);
		program.setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, ballTexture);
		float ballCoord[] = { -ball.width / 2.0f, ball.height / 2.0f,
							- ball.width / 2.0f, - ball.height / 2.0f,
							ball.width / 2.0f, ball.height / 2.0f,
							ball.width / 2.0f, ball.height / 2.0f,
							- ball.width / 2.0f, - ball.height / 2.0f,
							ball.width / 2.0f, -ball.height / 2.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballCoord);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}