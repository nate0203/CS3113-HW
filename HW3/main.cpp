/*
	HW3 Space Invaders, By: Nathan Ly
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

#include "ShaderProgram.h"
#include "Matrix.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#define MAX_BULLETS 5
#include <stdlib.h>

class SheetSprite {
public:
	SheetSprite() {}
	SheetSprite(unsigned int texID, float u1, float v1, float width1, float height1, float size1) {
		size = size1;
		textureID = texID;
		u = u1;
		v = v1;
		width = width1;
		height = height1;
	}

	void Draw(ShaderProgram *program) {
		glBindTexture(GL_TEXTURE_2D, textureID);

		GLfloat texCoords[] = {
			u, v + height, u + width, v, u, v,
			u + width, v, u, v + height, u + width, v + height
		};

		float aspect = width / height;
		float vertices[] = {
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, 0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, -0.5f * size ,
			0.5f * size * aspect, -0.5f * size
		};

		glUseProgram(program->programID);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	float size;
	unsigned int textureID;
	float u, v;
	float width, height;
};

class Entity {
public:
	Entity(){}

	float positionX;
	float positionY;
	float velocityX;
	float velocityY;
	float width;
	float height;

	SheetSprite sprite;
	bool alive = true;
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

void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (size_t i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, fontTexture);

	glUseProgram(program->programID);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void Restart(); // reset player, enemies, bullets
void Setup(); // set up textures
void ProcessEvents(); // for shooting and entering game
bool Collision(Entity obj1, Entity obj2); // checks for collisions
void newBullet(int index); // update bullets
void Update(); // update game level
void RenderMenu(); // render menu to start
void RenderGame(); // render game level
void RenderWin(); // render for player win
void RenderLose(); // render for player loss
void Render();

SDL_Window* displayWindow;
ShaderProgram* program;
Matrix projectionMatrix, viewMatrix, modelMatrix;
GLuint fontTexture, spriteTexture;
float lastFrameTicks = 0.0f;
std::vector<Entity> aliens;
Entity alienBullets[MAX_BULLETS];
Entity player;
Entity bullet;
int enemiesAlive = 55; // number of enemies alive
int score = 0;
bool moveLeft = false; // movement of enemies
bool moveRight = true; // movement of enemies
enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_LOSE, STATE_GAME_WIN };
int state;
const Uint8 *keys = SDL_GetKeyboardState(NULL);
bool done = false;
SDL_Event event;

int main(int argc, char *argv[]) {
	Setup();
	while (!done) {
		ProcessEvents();
		Update();
		Render();
	}
	SDL_Quit();
	delete program;
	return 0;
}

void Restart() {
	// player reset
	player.positionX = 0.0f;
	player.positionY = -1.8f;
	player.velocityX = 0.8f;

	// bullet reset
	bullet.alive = false;

	// aliens/invaders reset
	aliens.clear();
	float size = 0.33f;
	float spacing = 0.4f;
	float posX = -2.4f;
	float posY = 0.1f;
	for (int i = 0; i < 55; ++i) {
		Entity enemy;
		if (i % 11 == 0 && i > 0) {
			size -= 0.03f;
			posX = -2.4f;
			posY += 0.1f + aliens[i - 1].height;
		}
		enemy.sprite = SheetSprite(spriteTexture, 518.0f / 1024.0f, 409.0f / 1024.0f, 82.0f / 1024.0f, 84.0f / 1024.0f, size);
		enemy.height = enemy.sprite.size;
		enemy.width = enemy.sprite.width / enemy.sprite.height * enemy.sprite.size;
		enemy.positionX = posX;
		enemy.positionY = posY;
		enemy.velocityX = 0.3f;
		enemy.velocityY = 0.02f;
		aliens.push_back(enemy);
		posX += aliens[0].width + 0.15f;
	}

	// enemy bullets reset
	for (int i = 0; i < MAX_BULLETS; ++i) {
		int index = rand() % aliens.size();
		Entity aBullet;
		aBullet.sprite = SheetSprite(spriteTexture, 841.0f / 1024.0f, 647.0f / 1024.0f, 13.0f / 1024.0f, 37.0f / 1024.0f, 0.2f);
		aBullet.height = aBullet.sprite.size;
		aBullet.width = aBullet.sprite.width / aBullet.sprite.height * aBullet.sprite.size;
		aBullet.positionX = aliens[index].positionX;
		aBullet.positionY = aliens[index].positionY - aliens[index].height / 2.0f - aBullet.height / 2.0f;
		alienBullets[i] = aBullet;
	}

	// other game mechanics reset
	enemiesAlive = 55;
	score = 0;
	moveLeft = false;
	moveRight = true;
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

	fontTexture = LoadTexture(RESOURCE_FOLDER"font1.png");
	spriteTexture = LoadTexture(RESOURCE_FOLDER"sheet.png");

	player.sprite = SheetSprite(spriteTexture, 434.0f / 1024.0f, 234.0f / 1024.0f, 91.0f / 1024.0f, 91.0f / 1024.0f, 0.3f);
	player.width = player.sprite.width / player.sprite.height * player.sprite.size;
	player.height = player.sprite.size;

	bullet.sprite = SheetSprite(spriteTexture, 848.0f / 1024.0f, 565.0f / 1024.0f, 13.0f / 1024.0f, 37.0f / 1024.0f, 0.2f);
	bullet.height = bullet.sprite.size;
	bullet.width = bullet.sprite.width / bullet.sprite.height * bullet.sprite.size;
}

void ProcessEvents() {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
				if (state == STATE_MAIN_MENU) {
					// enter game level with proper positioning
					state = STATE_GAME_LEVEL;
					Restart();
				}
				if (state == STATE_GAME_WIN) {
					// switch to main menu after win
					state = STATE_MAIN_MENU;
				}
				if (state == STATE_GAME_LOSE) {
					// switch to main menu after loss
					state = STATE_MAIN_MENU;
				}
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
				if (!bullet.alive) {
					// shoot bullet from player ship
					bullet.alive = true;
					bullet.positionX = player.positionX;
					bullet.positionY = player.positionY + player.height / 2 + bullet.height / 2;
				}
			}
		}
	}
}

bool Collision(Entity obj1, Entity obj2) {
	// 4 collision checks
	if (obj1.positionX + obj1.width / 2.0f < obj2.positionX - obj2.width / 2.0f)
		return false;
	if (obj1.positionX - obj1.width / 2.0f > obj2.positionX + obj2.width / 2.0f)
		return false;
	if (obj1.positionY + obj1.height / 2.0f < obj2.positionY - obj2.height / 2.0f)
		return false;
	if (obj1.positionY - obj1.height / 2.0f > obj2.positionY + obj2.height / 2.0f)
		return false;
	return true;
}

void newBullet(int index) {
	// random alien that is alive shoots a bullet
	int aIndex = rand() % aliens.size();
	if (aliens[aIndex].alive) {
		alienBullets[index].alive = true;
		alienBullets[index].positionX = aliens[aIndex].positionX;
		alienBullets[index].positionY = aliens[aIndex].positionY - aliens[aIndex].height / 2.0f - alienBullets[index].height / 2.0f;
	}
	else
		alienBullets[index].alive = false;
}

void Update() {
	// if game level is not active then no update happens
	if (state != STATE_GAME_LEVEL)
		return;

	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;

	// player movement left and right
	if (keys[SDL_SCANCODE_LEFT]) {
		if (player.positionX - player.width / 2 > -3.55f)
			player.positionX -= elapsed * player.velocityX;
		else
			player.positionX = -3.55f + player.width / 2;
	}
	if (keys[SDL_SCANCODE_RIGHT]) {
		if (player.positionX + player.width / 2 < 3.55f)
			player.positionX += elapsed * player.velocityX;
		else
			player.positionX = 3.55f - player.width / 2;
	}

	// player bullet position changes
	if (bullet.positionY > 2.1f)
		bullet.alive = false;
	else if (bullet.positionY <= 2.1f && bullet.alive)
		bullet.positionY += elapsed * 2.5f;
	
	// switch directions if an alien reaches end of game screen
	for (size_t i = 0; i < aliens.size(); ++i) {
		if (aliens[i].positionX - aliens[i].width / 2 < -3.5f && aliens[i].alive) {
			moveRight = true;
			moveLeft = false;
			break;
		}	
		if (aliens[i].positionX + aliens[i].width / 2 > 3.5f && aliens[i].alive) {
			moveLeft = true;
			moveRight = false;
			break;
		}
	}

	// update alien's position
	for (size_t i = 0; i < aliens.size(); ++i) {
		if (moveRight)
			aliens[i].velocityX = 0.3f;
		else if (moveLeft)
			aliens[i].velocityX = -0.3f;

		aliens[i].positionX += elapsed * aliens[i].velocityX;
		aliens[i].positionY -= elapsed * aliens[i].velocityY;

		// checks if alien is hit by player bullet
		if (Collision(aliens[i], bullet) && bullet.alive && aliens[i].alive) {
			aliens[i].alive = false;
			bullet.alive = false;
			score += 182;
			if (score >= 9999)
				score = 9999;
			enemiesAlive--;
		}

		// checks if alien reaches a certain point
		if (aliens[i].positionY - aliens[i].height / 2 < -1.5f && aliens[i].alive) {
			state = STATE_MAIN_MENU;
			break;
		}
	}

	// check if there are any aliens alive
	if (enemiesAlive == 0)
		state = STATE_GAME_WIN;

	// check enemy bullets
	for (int i = 0; i < MAX_BULLETS; ++i) {
		alienBullets[i].positionY -= elapsed * 0.4f;

		// collisions with live bullets and player
		if (Collision(alienBullets[i], player) && alienBullets[i].alive) {
			state = STATE_GAME_LOSE;
			break;
		}
		
		// make new bullet if it goes past player
		if (alienBullets[i].positionY + alienBullets[i].height / 2.0f < player.positionY - player.height / 2.0f)
			newBullet(i);
	}
}

void RenderMenu() {
	// render information for main menu and how to play
	modelMatrix.identity();
	modelMatrix.Translate(-2.5f, 0.75f, 0.0f);
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	DrawText(program, fontTexture, "SPACE INVADERS", 0.5f, -0.15f);

	modelMatrix.identity();
	modelMatrix.Translate(-1.5f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "Press ENTER To Begin", 0.35f, -0.2f);

	modelMatrix.Translate(0.0f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "SPACE - Shoot", 0.3f, -0.15f);

	modelMatrix.Translate(-0.75f, -0.3f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "LEFT Arrow - Move Left", 0.3f, -0.15f);

	modelMatrix.Translate(-0.15f, -0.3f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "RIGHT Arrow - Move Right", 0.3f, -0.15f);
}

void RenderGame() {
	// render aliens, bullets, and player
	modelMatrix.identity();
	modelMatrix.Translate(player.positionX, player.positionY, 0.0f);
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	player.sprite.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(-3.5f, 1.9f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "Score: ", 0.25f, -0.15f);
	modelMatrix.Translate(0.7f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, std::to_string(score), 0.3f, -0.15f);

	if (bullet.alive) {
		modelMatrix.identity();
		modelMatrix.Translate(bullet.positionX, bullet.positionY, 0.0f);
		program->setModelMatrix(modelMatrix);
		bullet.sprite.Draw(program);
	}
	
	for (size_t i = 0; i < aliens.size(); ++i) {
		if (aliens[i].alive) {
			modelMatrix.identity();
			modelMatrix.Translate(aliens[i].positionX, aliens[i].positionY, 0.0f);
			program->setModelMatrix(modelMatrix);
			aliens[i].sprite.Draw(program);
		}
	}

	for (int i = 0; i < MAX_BULLETS; ++i) {
		if (alienBullets[i].alive) {
			modelMatrix.identity();
			modelMatrix.Translate(alienBullets[i].positionX, alienBullets[i].positionY, 0.0f);
			program->setModelMatrix(modelMatrix);
			alienBullets[i].sprite.Draw(program);
		}
	}
}

void RenderWin() {
	// render player win and score
	modelMatrix.identity();
	modelMatrix.Translate(player.positionX, player.positionY, 0.0f);
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	player.sprite.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(-1.0f, 0.8f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "YOU WIN!", 0.5f, -0.25f);
	modelMatrix.Translate(0.0f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "Score: " + std::to_string(score), 0.5f, -0.3f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.2f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "Press ENTER to return to Main Menu", 0.35f, -0.2f);
}

void RenderLose() {
	// render lose screen
	modelMatrix.identity();
	modelMatrix.Translate(-2.5f, 0.8f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "GAME OVER YOU WERE HIT!", 0.5f, -0.25f);
	modelMatrix.Translate(0.0f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "Score: " + std::to_string(score), 0.5f, -0.3f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.2f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "Press ENTER to return to Main Menu", 0.35f, -0.2f);
}

void Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	switch (state) {
	case STATE_MAIN_MENU:
		RenderMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGame();
		break;
	case STATE_GAME_LOSE:
		RenderLose();
		break;
	case STATE_GAME_WIN:
		RenderWin();
		break;
	}

	SDL_GL_SwapWindow(displayWindow);
}