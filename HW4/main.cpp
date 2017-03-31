/*
	HW4 Very Simple Platform Demo	
	By: Nathan Ly
	
	Left - Move left		Right - Move right
	Space - Jump			Up - Enter door

	Find the key, enter the door and jump on top of the slime to defeat it to get the gem.
	Close and re-open window to restart.
	* Ran out of time to implement other features such as resetting the game
	* Key is directly above the beginning, follow platforms
	* Door is to the right on lowest platform
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
#define FIXED_TIMESTEP 0.016666f
#define MAX_TIMESTEPS 6

class SheetSprite {
public:
	SheetSprite() {}
	SheetSprite(unsigned int texID, float u1, float v1, float width1, float height1, 
				float hor = 1.0f, float vert = 1.0f) {
		textureID = texID;
		u = u1;
		v = v1;
		width = width1;
		height = height1;
		horizontal = hor;
		vertical = vert;
	}

	void Draw(ShaderProgram *program) {
		glBindTexture(GL_TEXTURE_2D, textureID);

		GLfloat texCoords[] = {
			u, v + height, u + width, v, u, v,
			u + width, v, u, v + height, u + width, v + height
		};

		float aspect = width / height;
		float vertices[] = {
			-0.5f * size * aspect * horizontal, -0.5f * size * vertical,
			0.5f * size * aspect * horizontal, 0.5f * size * vertical,
			-0.5f * size * aspect * horizontal, 0.5f * size * vertical,
			0.5f * size * aspect * horizontal, 0.5f * size * vertical,
			-0.5f * size * aspect * horizontal, -0.5f * size * vertical,
			0.5f * size * aspect * horizontal, -0.5f * size * vertical
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

	// scale X and Y for render values of length and height of an entity
	void scale(float x, float y) {
		horizontal = x;
		vertical = y;
	}

	float size  = 1.0f;
	unsigned int textureID;
	float u, v;
	float width, height; // sprite values
	float horizontal, vertical; // actual render values
};

enum EntityType { PLAYER, KEY, DOOR, ENEMY };
class Entity {
public:
	Entity() {}

	bool collides(const Entity& other);
	void Render(ShaderProgram* program);

	void Reset() {
		collidedBot = false;
		collidedTop = false;
		collidedLeft = false;
		collidedRight = false;
	}

	float pos_x, pos_y;
	float vel_x, vel_y;
	float accel_x, accel_y;

	SheetSprite sprite;
	EntityType type;

	bool isStatic = false;
	bool collidedTop = false;
	bool collidedBot = false;
	bool collidedLeft = false;
	bool collidedRight = false;
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

void Setup();
void ProcessEvents();
void Update();
void Update(float fixedTime);
void collisionX();
void collisionY();
void Render();

SDL_Window* displayWindow;
ShaderProgram* program;
Matrix projectionMatrix, viewMatrix, modelMatrix;
GLuint tileTexture, spriteTexture, itemTexture, enemyTexture, fontTexture;
float lastFrameTicks = 0.0f;
float timer = 0.0f;
Entity player, key, gem, doorMid1, doorTop1, doorMid2, doorTop2, enemy;
std::vector<Entity> walls;
bool hasKey = false;
bool enemyAlive = true;
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

	SDL_Quit();
	delete program;
	return 0;
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

	tileTexture = LoadTexture(RESOURCE_FOLDER"tiles_spritesheet.png");
	spriteTexture = LoadTexture(RESOURCE_FOLDER"p1_spritesheet.png");
	itemTexture = LoadTexture(RESOURCE_FOLDER"items_spritesheet.png");
	enemyTexture = LoadTexture(RESOURCE_FOLDER"enemies_spritesheet.png");
	fontTexture = LoadTexture(RESOURCE_FOLDER"font1.png");

	// bottom platform
	Entity l;
	l.sprite = SheetSprite(tileTexture, 504.0f / 1024.0f, 576.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 28.4f, 0.4f);
	l.pos_x = 10.65f;
	l.pos_y = -1.8f;
	l.isStatic = true;
	walls.push_back(l);

	// right end
	l.sprite = SheetSprite(tileTexture, 576.0f / 1024.0f, 864.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 0.3f, 7.6f);
	l.pos_x = 24.7f;
	l.pos_y = 2.2f;
	walls.push_back(l);

	// enemy room wall
	l.pos_x = 17.75f;
	walls.push_back(l);

	// left end
	l.pos_x = -3.4f;
	l.pos_y = 2.2f;
	walls.push_back(l);

	// vertical wall
	l.sprite = SheetSprite(tileTexture, 576.0f / 1024.0f, 864.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 0.4f, 6.0f);
	l.pos_x = 7.1f;
	l.pos_y = 2.5f;
	walls.push_back(l);

	// horizontal platforms
	l.sprite = SheetSprite(tileTexture, 504.0f / 1024.0f, 576.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 6.0f, 0.4f);
	l.pos_x = 1.0f;
	l.pos_y = -0.5f;
	walls.push_back(l);

	l.sprite.scale(2.0f, 0.4f);
	l.pos_x = -2.25f;
	l.pos_y = 0.9f;
	walls.push_back(l);

	l.sprite.scale(3.6f, 0.4f);
	l.pos_x = 5.1f;
	walls.push_back(l);

	l.sprite.scale(5.0f, 0.4f);
	l.pos_x = 1.0f;
	l.pos_y = 2.4f;
	walls.push_back(l);

	l.sprite.scale(6.0f, 0.4f);
	l.pos_x = 8.5f;
	l.pos_y = -0.6f;
	walls.push_back(l);

	l.pos_x = 13.5f;
	l.pos_y = 0.9f;
	walls.push_back(l);

	l.sprite.scale(3.0f, 0.3f);
	l.pos_x = 22.0f;
	l.pos_y = -0.25f;
	walls.push_back(l);

	// player
	player.sprite = SheetSprite(spriteTexture, 67.0f / 512.0f, 196.0f / 512.0f, 66.0f / 512.0f, 92.0f / 512.0f, 0.5f, 0.5f);
	player.pos_x = -3.05f;
	player.pos_y = -1.45f;
	player.vel_x = 0.0f;
	player.accel_x = 0.0f;
	player.vel_y = 0.0f;
	player.type = PLAYER;

	// key
	key.sprite = SheetSprite(itemTexture, 131.0f / 576.0f, 0.0f, 70.0f / 576.0f, 70.0f / 576.0f, 0.3f, 0.3f);
	key.pos_x = -3.05f;
	key.pos_y = 3.6f;
	key.type = KEY;

	// door sprites 1
	doorMid1.sprite = SheetSprite(tileTexture, 648.0 / 1024.0f, 432.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 0.5f, 0.5f);
	doorMid1.pos_x = 17.0f;
	doorMid1.pos_y = -1.45f;
	doorMid1.type = DOOR;
	doorTop1.sprite = SheetSprite(tileTexture, 648.0 / 1024.0f, 360.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 0.5f, 0.5f);
	doorTop1.pos_x = 17.0f;
	doorTop1.pos_y = -0.95f;
	doorTop1.isStatic = true;

	// door sprites 2
	doorMid2.sprite = SheetSprite(tileTexture, 648.0 / 1024.0f, 432.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 0.5f, 0.5f);
	doorMid2.pos_x = 22.0f;
	doorMid2.pos_y = 0.15f;
	doorMid2.type = DOOR;
	doorTop2.sprite = SheetSprite(tileTexture, 648.0 / 1024.0f, 360.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 0.5f, 0.5f);
	doorTop2.pos_x = 22.0f;
	doorTop2.pos_y = 0.65f;
	doorTop2.isStatic = true;

	// enemy
	enemy.sprite = SheetSprite(enemyTexture, 52.0f / 353.0f, 125.0f / 153.0f, 50.0f / 353.0f, 28.0f / 153.0f, 0.8f, 0.4f);
	enemy.pos_x = 20.0f;
	enemy.pos_y = -1.4f;
	enemy.vel_x = -0.5f;
	enemy.type = ENEMY;
}

void ProcessEvents() {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
	}
}

void Update() {
	// fixed timestep for collisions
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
	timer += elapsed;

	// player movement
	if (keys[SDL_SCANCODE_LEFT] && !player.collidedLeft)
		player.accel_x = -4.5f;
	else if (keys[SDL_SCANCODE_RIGHT] && !player.collidedRight)
		player.accel_x = 4.5f;
	else
		player.accel_x = 0.0f;

	// reset contact flags
	player.Reset();
	enemy.Reset();

	// player movement calculations
	player.vel_x = lerp(player.vel_x, 0.0f, elapsed * 1.5f);
	player.vel_y = lerp(player.vel_y, 0.0f, elapsed * 0.1f);

	player.vel_x += player.accel_x * elapsed;
	player.vel_y += -5.0f * elapsed;

	// player collision update
	player.pos_y += player.vel_y * elapsed;
	collisionY();
	player.pos_x += player.vel_x * elapsed;
	collisionX();

	// enemy movement calculations
	enemy.pos_x += enemy.vel_x * elapsed;
	if (enemy.pos_x - enemy.sprite.horizontal / 2.6f < walls[2].pos_x + walls[2].sprite.horizontal / 2.0f) {
		enemy.vel_x = 0.5f;
		enemy.collidedLeft = true;
	}
	else if (enemy.pos_x + enemy.sprite.horizontal / 2.5f > walls[1].pos_x - walls[1].sprite.horizontal / 2.0f) {
		enemy.vel_x = -0.5f;
		enemy.collidedRight = true;
	}

	// enemy contact flags, reverse directions
	if (enemy.collidedLeft) {
		float penetration = fabs(enemy.pos_x - walls[2].pos_x - enemy.sprite.horizontal / 2.6f - walls[2].sprite.horizontal / 2.0f);
		enemy.pos_x += penetration + 0.001f;
		enemy.vel_x = 0.5f;
	}
	if (enemy.collidedRight) {
		float penetration = fabs(walls[1].pos_x - enemy.pos_x - enemy.sprite.horizontal / 2.5f - walls[1].sprite.horizontal / 2.0f);
		enemy.pos_x -= (penetration + 0.001f);
		enemy.vel_x = -0.5f;
	}

	// player jumping
	if (keys[SDL_SCANCODE_SPACE]) {
		if (player.collidedBot)
			player.vel_y = 4.0f + fabs(0.01f * player.vel_x);
	}

	// key obtained, door opens
	if (player.collides(key)) {
		hasKey = true;
		doorMid1.sprite = SheetSprite(tileTexture, 648.0f / 1024.0f, 288.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 0.5f, 0.5f);
		doorMid2.sprite = SheetSprite(tileTexture, 648.0f / 1024.0f, 288.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 0.5f, 0.5f);
		doorTop1.sprite = SheetSprite(tileTexture, 648.0f / 1024.0f, 216.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 0.5f, 0.5f);
		doorTop2.sprite = SheetSprite(tileTexture, 648.0f / 1024.0f, 216.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 0.5f, 0.5f);
	}

	// enter door if open, delay in entering door from both sides
	if (hasKey && keys[SDL_SCANCODE_UP] && timer > 1.0f) {
		if (player.collides(doorMid1)) {
			player.pos_x = doorMid2.pos_x;
			player.pos_y = doorMid2.pos_y;
		}
		else if (player.collides(doorMid2)) {
			player.pos_x = doorMid1.pos_x;
			player.pos_y = doorMid1.pos_y;
		}
		player.accel_x = 0.0f;
		player.vel_x = 0.0f;
		timer = 0.0f;
	}

	// enemy collision occurred
	if (player.collides(enemy) && enemyAlive && !enemy.isStatic) {
		if (player.pos_y - player.sprite.vertical / 2.0f < enemy.pos_y + enemy.sprite.vertical / 2.0f
			&& player.pos_y > enemy.pos_y + enemy.sprite.vertical / 2.0f && player.vel_y < 0.0f) {
			enemyAlive = false;
			enemy.isStatic = true;
			enemy.vel_x = 0.0f;
			enemy.pos_y -= 0.05f;
			enemy.sprite = SheetSprite(enemyTexture, 0.0f, 112.0f / 153.0f, 59.0f / 353.0f, 12.0f / 153.0f, 0.3f, 0.3f);
			gem.sprite = SheetSprite(itemTexture, 144.0f / 576.0f, 362.0f / 576.0f, 70.0f / 576.0f, 70.0f / 576.0f, 0.4f, 0.4f);
			gem.pos_y = enemy.pos_y + player.sprite.vertical / 2.0f + enemy.sprite.vertical / 2.0f;
			gem.pos_x = enemy.pos_x;
			player.sprite = SheetSprite(spriteTexture, 67.0f / 512.0f, 196.0f / 512.0f, 66.0f / 512.0f, 92.0f / 512.0f, 0.5f, 0.5f);
		}
		else if (enemyAlive){
			player.sprite = SheetSprite(spriteTexture, 438.0f / 512.0f, 0.0f, 69.0f / 512.0f, 92.0f / 512.0f, 0.5f, 0.5f);
		}
	}
}

bool Entity::collides(const Entity& other) {
	if (type == PLAYER) {
		// door collision case
		if (other.type == DOOR) {
			if (player.pos_x + player.sprite.horizontal / 3.0f < other.pos_x - other.sprite.horizontal / 2.0f)
				return false;
			if (player.pos_x - player.sprite.horizontal / 3.0f > other.pos_x + other.sprite.horizontal / 2.0f)
				return false;
			if (player.pos_y + player.sprite.vertical / 3.0f < other.pos_y - other.sprite.vertical / 2.0f)
				return false;
			if (player.pos_y - player.sprite.vertical / 3.0f > other.pos_y + other.sprite.vertical / 2.0f)
				return false;
			return true;
		}
		// enemy collision case
		if (other.type == ENEMY) {
			if (player.pos_x + player.sprite.horizontal / 2.7f < other.pos_x - other.sprite.horizontal / 2.6f)
				return false;
			if (player.pos_x - player.sprite.horizontal / 2.7f > other.pos_x + other.sprite.horizontal / 2.5f)
				return false;
			if (player.pos_y + player.sprite.vertical / 2.0f < other.pos_y - other.sprite.vertical / 2.0f)
				return false;
			if (player.pos_y - player.sprite.vertical / 2.2f > other.pos_y + other.sprite.vertical / 2.0f)
				return false;
			return true;
		}
		// wall collision and key collision case
		if (other.isStatic || other.type == KEY) {
			if (player.pos_x + player.sprite.horizontal / 2.0f < other.pos_x - other.sprite.horizontal / 2.0f)
				return false;
			if (player.pos_x - player.sprite.horizontal / 2.0f > other.pos_x + other.sprite.horizontal / 2.0f)
				return false;
			if (player.pos_y + player.sprite.vertical / 2.0f < other.pos_y - other.sprite.vertical / 2.0f)
				return false;
			if (player.pos_y - player.sprite.vertical / 2.0f > other.pos_y + other.sprite.vertical / 2.0f)
				return false;
			return true;
		}
	}
}

void collisionX() {
	// player collisionX
	size_t i = 0;
	for (i = 0; i < walls.size(); ++i) {
		if (!player.collides(walls[i]))
			continue;

		// top collision prevents suddent movement to left or right
		if (!player.collidedTop) {
			// left collision check
			if (player.pos_x - player.sprite.horizontal / 2.7f < walls[i].pos_x + walls[i].sprite.horizontal / 2.0f
				&& player.pos_x > walls[i].pos_x) {
				player.collidedLeft = true;
				break;
			} // right collision check
			else if (player.pos_x + player.sprite.horizontal / 2.7f > walls[i].pos_x - walls[i].sprite.horizontal / 2.0f
				&& player.pos_x < walls[i].pos_x) {
				player.collidedRight = true;
				break;
			}
		}
	}

	float penetration;
	if (player.collidedLeft) {
		penetration = fabs(player.pos_x - walls[i].pos_x - player.sprite.horizontal / 2.7f - walls[i].sprite.horizontal / 2.0f);
		player.pos_x += penetration + 0.001f;
		player.accel_x = 0.0f;
		player.vel_x = 0.0f;
	}
	if (player.collidedRight) {
		penetration = fabs(walls[i].pos_x - player.pos_x - player.sprite.horizontal / 2.7f - walls[i].sprite.horizontal / 2.0f);
		player.pos_x -= (penetration + 0.001f);
		player.accel_x = 0.0f;
		player.vel_x = 0.0f;
	}
}

void collisionY() {
	// player collisionY
	size_t i = 0;
	for (i = 0; i < walls.size(); ++i) {
		if (!player.collides(walls[i]))
			continue;

		// bottom collision check, (legs of player still on platform, etc.)
		if (player.pos_y - player.sprite.vertical / 2.0f < walls[i].pos_y + walls[i].sprite.vertical / 2.0f
			&& walls[i].pos_y < player.pos_y 
			&& player.pos_x - player.sprite.horizontal / 4.0f < walls[i].pos_x + walls[i].sprite.horizontal / 2.0f
			&& player.pos_x + player.sprite.horizontal / 4.0f > walls[i].pos_x - walls[i].sprite.horizontal / 2.0f) {
			player.collidedBot = true;
			break;
		} // top collision check (head of player still hits platform, etc.)
		else if (player.pos_y + player.sprite.vertical / 2.0f > walls[i].pos_y - walls[i].sprite.vertical / 2.0f 
			&& !player.collidedBot && walls[i].pos_y > player.pos_y
			&& player.pos_x - player.sprite.horizontal / 3.0f  < walls[i].pos_x + walls[i].sprite.horizontal / 2.0f
			&& player.pos_x + player.sprite.horizontal / 3.0f > walls[i].pos_x - walls[i].sprite.horizontal / 2.0f) {
			player.collidedTop = true;
			break;
		}
	}

	float penetration;
	if (player.collidedBot) {
		penetration = fabs(player.pos_y - walls[i].pos_y - player.sprite.vertical / 2.0f - walls[i].sprite.vertical / 2.0f);
		player.pos_y += penetration + 0.001f;
		player.vel_y = 0.0f;
	}
	if (player.collidedTop) {
		penetration = fabs(walls[i].pos_y - player.pos_y - player.sprite.vertical / 2.0f - walls[i].sprite.vertical / 2.0f);
		player.pos_y -= (penetration + 0.001f);
		player.vel_y = 0.0f;
	}
}

void Entity::Render(ShaderProgram* program) {
	// render current entity
	modelMatrix.identity();
	modelMatrix.Translate(pos_x, pos_y, 0.0f);
	program->setModelMatrix(modelMatrix);
	sprite.Draw(program);
}

void Render() {
	// render entities
	glClear(GL_COLOR_BUFFER_BIT);

	program->setProjectionMatrix(projectionMatrix);
	doorMid1.Render(program);
	doorTop1.Render(program);
	doorMid2.Render(program);
	doorTop2.Render(program);

	enemy.Render(program);
	if (!enemyAlive) {
		gem.Render(program);
		modelMatrix.Translate(0.0f, 0.5f, 0.0f);
		program->setModelMatrix(modelMatrix);
		DrawText(program, fontTexture, "You Win!", 0.4f, -0.15f);
	}

	viewMatrix.identity();
	viewMatrix.Translate(-player.pos_x, -player.pos_y, 0.0f);
	if (player.pos_x < 0.0f)
		viewMatrix.Translate(player.pos_x, 0.0f, 0.0f);
	else if (player.pos_x > 21.3f)
		viewMatrix.Translate(player.pos_x - 21.3f, 0.0f, 0.0f);
	if (player.pos_y < 0.0f)
		viewMatrix.Translate(0.0f, player.pos_y, 0.0f);
	else if (player.pos_y > 3.0f)
		viewMatrix.Translate(0.0f, player.pos_y - 3.0f, 0.0f);
	program->setViewMatrix(viewMatrix);
	player.Render(program);
	
	if (!hasKey)
		key.Render(program);

	for (size_t i = 0; i < walls.size(); ++i)
		walls[i].Render(program);

	SDL_GL_SwapWindow(displayWindow);
}