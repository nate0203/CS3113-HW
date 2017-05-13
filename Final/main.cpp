/*
	By: Nathan Ly
	Goal: Defend your tower, must attack to beat enemy tower
	ESC - escape game
	1 - Spawn Knight
	2 - Spawn Archer
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
#include "SDL_mixer.h"
#define FIXED_TIMESTEP 0.016666f
#define MAX_TIMESTEPS 6
#include <stdlib.h>
#include "SDL_mixer.h"

float mapValue(float value, float srcMin, float srcMax, float dstMin, float dstMax) {
	float retVal = dstMin + ((value - srcMin) / (srcMax - srcMin) * (dstMax - dstMin));
	if (retVal < dstMin)
		retVal = dstMin;
	if (retVal > dstMax)
		retVal = dstMax;
	return retVal;
}

float easeInOut(float from, float to, float time) {
	float tVal;
	if (time > 0.5) {
		float oneMinusT = 1.0f - ((0.5f - time)*-2.0f);
		tVal = 1.0f - ((oneMinusT * oneMinusT * oneMinusT * oneMinusT *
			oneMinusT) * 0.5f);
	}
	else {
		time *= 2.0;
		tVal = (time*time*time*time*time) / 2.0;
	}
	return (1.0f - tVal)*from + tVal*to;
}

void DrawSpriteSheetSprite(ShaderProgram *program, int index, int spriteCountX,
	int spriteCountY) {
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;

	GLfloat texCoords[] = {
		u, v + spriteHeight, u + spriteWidth, v, u, v,
		u + spriteWidth, v, u, v + spriteHeight, u + spriteWidth, v + spriteHeight
	};	
	float vertices[] = { -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };
	
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

class Vector {
public:
	Vector() {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}
	Vector(float x1, float y1, float z1) {
		x = x1;
		y = y1;
		z = z1;
	}
	float x, y, z;
};

struct EvenSprite {
	GLuint sprite;
	int x, y;
	Vector size;
};

enum Motion { IDLE, WALK, ATTACK, DEAD };
class Entity {
public:
	Entity(){}

	Vector position;
	Vector velocity;
	float health;
	int damage;
	bool alive = true;
	Motion type;
	float attackTime = 0.0f;
	EvenSprite sheet;
};

class Knight : public Entity {
public:
	Knight(float x1, float y1, float vx, float vy, Motion t) {
		health = 100.0f;
		damage = 10;
		position = Vector(x1, y1, 0.0f);
		velocity = Vector(vx, vy, 0.0f);
		type = t;
	}
	void Update(float elapsed);
};

class Archer : public Entity {
public:
	Archer(float x1, float y1, float vx, float vy, Motion t) {
		health = 100.0f;
		damage = 15;
		position = Vector(x1, y1, 0.0f);
		velocity = Vector(vx, vy, 0.0f);
		type = t;
	}
	void Update(float elapsed);
};

void Setup();
void ProcessEvents();
void Update();
void UpdateDefend(float elapsed);
void AI(float elapsed);
void AIMove(float elapsed);
void Collide(float elapsed);
void Render();
void RenderMainMenu();
void RenderControls();
void RenderGame();
void RenderGameNext();
void RenderDeath();
void RenderWin();

SDL_Window* displayWindow;
ShaderProgram* program;
Matrix projectionMatrix, viewMatrix, modelMatrix;
GLuint fontMain, mainScreen;
GLuint night, trees, mountains, bg;
GLuint victory, defeat;
EvenSprite leftKnightAttack, leftKnightWalk, leftKnightIdle, rightKnightAttack, rightKnightWalk, rightKnightIdle;
EvenSprite leftArcherAttack, leftArcherWalk, leftArcherIdle, rightArcherAttack, rightArcherWalk, rightArcherIdle;
EvenSprite tower;
const int anim[] = { 0, 1, 2, 3 };
float animationElapsed = 0.0f;
float framesPerSecond = 4.0f;
int current = 0;
float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
Mix_Chunk* death;
Mix_Chunk* hit;
Mix_Chunk* gameOver;
Mix_Chunk* gameWin;
Mix_Music* music;
std::vector<Knight> knight;
std::vector<Archer> archer;
std::vector<Knight> otherKnight;
std::vector<Archer> otherArcher;
Entity pTower;
Entity enemyTower1;
int gold;
int enemySpawn;
int day;
float wait = 0.0f;
enum GameState { STATE_MAIN_MENU, STATE_CONTROLS, STATE_GAME_DEFEND, STATE_GAME_ATTACK, 
		STATE_GAME_NEXT, STATE_GAME_DEATH, STATE_GAME_WIN };
int state = STATE_MAIN_MENU;
float lastFrameTicks = 0.0f;
float timer = 0.0f;
float animationTime = 0.0f;
float spawnTimer = 0.0f;
float roundTimer = 0.0f;
float playerSpawn = 2.0f;
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
	Mix_FreeChunk(gameOver);
	Mix_FreeChunk(gameWin);
	Mix_FreeMusic(music);
	return 0;
}

void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1216, 684, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	glUseProgram(program->programID);
	glViewport(0, 0, 1216, 684);
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	fontMain = LoadTexture(RESOURCE_FOLDER"font1.png");
	mainScreen = LoadTexture(RESOURCE_FOLDER"dungeon.png");
	night = LoadTexture(RESOURCE_FOLDER"parallax-mountain-bg.png");
	mountains = LoadTexture(RESOURCE_FOLDER"parallax-mountain-mountains.png");
	trees = LoadTexture(RESOURCE_FOLDER"parallax-mountain-trees.png");
	bg = LoadTexture(RESOURCE_FOLDER"bg.png");
	victory = LoadTexture(RESOURCE_FOLDER"coat_of_arm.png");
	defeat = LoadTexture(RESOURCE_FOLDER"broken_tower.png");

	tower.sprite = LoadTexture(RESOURCE_FOLDER"tower.png");
	tower.size = Vector(0.7f, 1.0f, 1.0f);
	tower.x = 1;
	tower.y = 1;

	leftKnightAttack.sprite = LoadTexture(RESOURCE_FOLDER"soldier_attack_1_1x4.png");
	leftKnightAttack.x = 1;
	leftKnightAttack.y = 4;
	leftKnightAttack.size = Vector(0.4f, 0.32f, 0.0f);
	leftKnightWalk.sprite = LoadTexture(RESOURCE_FOLDER"soldier_walk_1_2x2.png");
	leftKnightWalk.x = 2;
	leftKnightWalk.y = 2;
	leftKnightWalk.size = Vector(0.4f, 0.32f, 0.0f);
	leftKnightIdle.sprite = LoadTexture(RESOURCE_FOLDER"soldier_idle_1.png");
	leftKnightIdle.x = 1;
	leftKnightIdle.y = 1;
	leftKnightIdle.size = Vector(0.5f, 0.48f, 0.0f);

	rightKnightAttack.sprite = LoadTexture(RESOURCE_FOLDER"right_soldier_attack.png");
	rightKnightAttack.x = 1;
	rightKnightAttack.y = 4;
	rightKnightAttack.size = Vector(0.4f, 0.32f, 0.0f);
	rightKnightIdle.sprite = LoadTexture(RESOURCE_FOLDER"right_soldier_idle.png");
	rightKnightIdle.x = 1;
	rightKnightIdle.y = 1;
	rightKnightIdle.size = Vector(0.5f, 0.48f, 0.0f);
	rightKnightWalk.sprite = LoadTexture(RESOURCE_FOLDER"right_soldier_walk.png");
	rightKnightWalk.x = 2;
	rightKnightWalk.y = 2;
	rightKnightWalk.size = Vector(0.4f, 0.32f, 0.0f);

	leftArcherAttack.sprite = LoadTexture(RESOURCE_FOLDER"left_archer_attack.png");
	leftArcherAttack.x = 4;
	leftArcherAttack.y = 1;
	leftArcherAttack.size = Vector(0.3f, 0.32f, 0.0f);
	leftArcherWalk.sprite = LoadTexture(RESOURCE_FOLDER"archer_28.png");
	leftArcherWalk.x = 1;
	leftArcherWalk.y = 1;
	leftArcherWalk.size = Vector(0.22f, 0.32f, 0.0f);

	rightArcherAttack.sprite = LoadTexture(RESOURCE_FOLDER"right_archer_attack.png");
	rightArcherAttack.x = 4;
	rightArcherAttack.y = 1;
	rightArcherAttack.size = Vector(0.3f, 0.32f, 0.0f);
	rightArcherWalk.sprite = LoadTexture(RESOURCE_FOLDER"archer_14.png");
	rightArcherWalk.x = 1;
	rightArcherWalk.y = 1;
	rightArcherWalk.size = Vector(0.22f, 0.32f, 0.0f);

	pTower.health = 1000;
	pTower.position = Vector(-3.0f, -1.1f, 0.0f);
	pTower.sheet = tower;
	enemyTower1.health = 3000;
	enemyTower1.position = Vector(3.0f, -1.1f, 0.0f);
	enemyTower1.sheet = tower;

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	music = Mix_LoadMUS("war.mp3");
	Mix_PlayMusic(music, -1);

	death = Mix_LoadWAV("deathh.wav");
	hit = Mix_LoadWAV("pain1.wav");
	gameOver = Mix_LoadWAV("ThisGameIsOver.wav");
	gameWin = Mix_LoadWAV("win.wav");
}

void ProcessEvents() {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
			done = true;
		}
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
				if (state == STATE_MAIN_MENU) {
					state = STATE_GAME_DEFEND;
					animationTime = 0.0f;
					day = 1;
					enemySpawn = rand() % 5 + 2 * (day-1);
					spawnTimer = 0.0f;
					playerSpawn = 2.0f;
					gold = 500;
					pTower.health = 2000;
					enemyTower1.health = 3000;
				}
				if (state == STATE_CONTROLS)
					state == STATE_MAIN_MENU;
				if (state == STATE_GAME_DEATH || state == STATE_GAME_WIN) {
					state = STATE_MAIN_MENU;
					pTower.health = 500;
					enemyTower1.health = 500;
					knight.clear();
					archer.clear();
					otherKnight.clear();
					otherArcher.clear();
					Mix_PlayMusic(music, -1);
				}
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_V && state == STATE_MAIN_MENU)
				state = STATE_CONTROLS;
			if (state == STATE_GAME_NEXT) {
				animationTime = 0.0f;
				knight.clear();
				archer.clear();
				otherKnight.clear();
				otherArcher.clear();
				spawnTimer = 0.0f;
				playerSpawn = 2.0f;
				wait = 0.0f;
				if (event.key.keysym.scancode == SDL_SCANCODE_A) {
					day++;
					state = STATE_GAME_ATTACK;
					enemySpawn = 9999;
					roundTimer = 0.0f;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_D) {
					day++;
					state = STATE_GAME_DEFEND;
					enemySpawn = rand() % 5 + 2 * (day-1);
					gold += 400;
				}
			}
			if (state == STATE_GAME_DEFEND || state == STATE_GAME_ATTACK) {
				if (event.key.keysym.scancode == SDL_SCANCODE_1 && playerSpawn >= 2.0f) {
					if (gold >= 100 && knight.size() <= 10) {
						Knight tmp(-3.6f, -1.0f - knight.size() * 0.02f, 0.3f, 0.0f, WALK);
						tmp.sheet = leftKnightWalk;
						knight.push_back(tmp);
						gold -= 100;
						playerSpawn = 0.0f;
					}
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_2 && playerSpawn >= 2.0f) {
					if (gold >= 150 && archer.size() <= 10) {
						Archer tmp(-3.6f, -1.0f - archer.size() * 0.02f, 0.15f, 0.0f, WALK);
						tmp.sheet = leftArcherWalk;
						archer.push_back(tmp);
						gold -= 150;
						playerSpawn = 0.0f;
					}
				}
			}
		}
	}
}

void Update() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;
	
	if (state == STATE_GAME_DEFEND)
		UpdateDefend(elapsed);
	if (state == STATE_GAME_ATTACK)
		UpdateDefend(elapsed);
	if (state == STATE_GAME_NEXT) {
		animationTime = animationTime + elapsed;
		float animationValue = mapValue(animationTime, 1.0, 0.0, 0.0, 1.0);
		modelMatrix.identity();
		modelMatrix.Translate(easeInOut(0.0, 1.0, animationValue) * 7.1f, 0.0, 0.0);
	}
}

void UpdateDefend(float elapsed) {
	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		current++;
		animationElapsed = 0.0;
		if (current > 3)
			current = 0;
	}

	if (enemySpawn == 0 && otherKnight.size() == 0 && otherArcher.size() == 0 && state == STATE_GAME_DEFEND) {
		wait += elapsed;
		if (wait > 1.5f) {
			state = STATE_GAME_NEXT;
			gold += archer.size() * 75 + knight.size() * 50 + 400;
		}
		return;
	}
	if (state == STATE_GAME_ATTACK)
		roundTimer += elapsed;
	if (roundTimer >= 80.0f && state == STATE_GAME_ATTACK) {
		wait += elapsed;
		if (wait > 1.5f)
			state = STATE_GAME_NEXT;
		return;
	}

	AI(elapsed);
	float fixedElapsed = elapsed;
	playerSpawn += elapsed;
	if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS)
		fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;

	while (fixedElapsed >= FIXED_TIMESTEP) {
		fixedElapsed -= FIXED_TIMESTEP;
		AIMove(fixedElapsed);
		for (size_t i = 0; i < knight.size(); ++i)
			knight[i].Update(FIXED_TIMESTEP);
		for (size_t i = 0; i < archer.size(); ++i)
			archer[i].Update(FIXED_TIMESTEP);
		Collide(fixedElapsed);
	}

	for (size_t i = 0; i < knight.size(); ++i)
		knight[i].Update(fixedElapsed);
	for (size_t i = 0; i < archer.size(); ++i)
		archer[i].Update(fixedElapsed);
	AIMove(fixedElapsed);
	Collide(fixedElapsed);

	if (pTower.health <= 0) {
		state = STATE_GAME_DEATH;
		Mix_HaltMusic();
		Mix_PlayChannel(-1, gameOver, 0);
		return;
	}
	if (enemyTower1.health <= 0) {
		state = STATE_GAME_WIN;
		Mix_HaltMusic();
		Mix_PlayChannel(-1, gameWin, 0);
		return;
	}
}

void AI(float elapsed) { // spawn enemy based on timer
	int choice = rand() % 100 + 1;
	spawnTimer += elapsed;
	if (knight.size() >= 0 && otherKnight.size() == 0 && enemySpawn > 0) {
		Knight tmp(3.6f, -1.0f, -0.3f, 0.0f, WALK);
		tmp.sheet = rightKnightWalk;
		otherKnight.push_back(tmp);
		enemySpawn--;
	}
	if (choice < 60 && enemySpawn > 0) {
		if ((spawnTimer >= 5.0f && state == STATE_GAME_DEFEND) || (spawnTimer >= 4.0f && state == STATE_GAME_ATTACK)) {
			Knight tmp(3.6f, -1.0f - otherKnight.size() * 0.02f, -0.3f, 0.0f, WALK);
			tmp.sheet = rightKnightWalk;
			otherKnight.push_back(tmp);
			spawnTimer = 0.0f;
			enemySpawn--;
		}
	}
	if (choice > 60 && enemySpawn > 0) {
		if ((spawnTimer >= 5.0f && state == STATE_GAME_DEFEND) || (spawnTimer >= 4.0f && state == STATE_GAME_ATTACK)) {
			Archer tmp(3.6f, -1.0f - otherArcher.size() * 0.02f, -0.15f, 0.0f, WALK);
			tmp.sheet = rightArcherWalk;
			otherArcher.push_back(tmp);
			spawnTimer = 0.0f;
			enemySpawn--;
		}
	}
}

void AIMove(float elapsed) {
	for (size_t i = 0; i < otherKnight.size(); ++i) {
		if (state == STATE_GAME_ATTACK) {
			if (otherKnight[i].position.x <= 0.0f)
				otherKnight[i].velocity.x = 0.0f;
		}
		otherKnight[i].position.x += otherKnight[i].velocity.x * elapsed;
	}
	for (size_t i = 0; i < otherArcher.size(); ++i) {
		if (state == STATE_GAME_ATTACK) {
			if (otherArcher[i].position.x <= 1.0f)
				otherArcher[i].velocity.x = 0.0f;
		}
		otherArcher[i].position.x += otherArcher[i].velocity.x * elapsed;
	}
}

void Knight::Update(float elapsed) {
	if (position.x >= 0.0f && state == STATE_GAME_DEFEND) {
		velocity.x = 0.0f;
		type == IDLE;
	}
	if (type == WALK)
		position.x += velocity.x * elapsed;
}

void Archer::Update(float elapsed) {
	if (position.x >= -0.5f && state == STATE_GAME_DEFEND) {
		velocity.x = 0.0f;
		type == IDLE;
	}
	if (type == WALK)
		position.x += velocity.x * elapsed;	
}

void Collide(float elapsed) {	// checks for attack, hitbox location then random value comparison
	if (knight.size() > 0 || otherKnight.size() > 0) {
		for (int i = 0; i < knight.size(); ++i) {
			bool atk = false;
			int k = 0;
			int chance = rand() % 1000 + 1;
			knight[i].attackTime += elapsed;
			knight[i].type = WALK;
			knight[i].sheet = leftKnightWalk;
			knight[i].velocity.x = 0.3f;
			while (k < otherKnight.size()) {
				if (otherKnight[k].position.x - otherKnight[k].sheet.size.x / 3.0f <= knight[i].position.x + knight[i].sheet.size.x / 3.0f) {
					knight[i].type = ATTACK;
					knight[i].sheet = leftKnightAttack;
					knight[i].velocity.x = 0.0f;
					if (chance < 5 && knight[i].attackTime >= 1.5f) {
						otherKnight[k].health -= knight[i].damage;
						Mix_PlayChannel(-1, hit, 0);
						if (otherKnight[k].health <= 0) {
							Mix_PlayChannel(-1, death, 0);
							otherKnight.erase(otherKnight.begin() + k);
							gold += 50;
						}
						knight[i].attackTime = 0.0f;
					}
					atk = true;
					break;
				}
				++k;
			}
			if (atk)
				continue;
			k = 0;
			while (k < otherArcher.size()) {
				if (knight[i].position.x + knight[i].sheet.size.x / 3.0f >= otherArcher[k].position.x - otherArcher[k].sheet.size.x / 3.0f) {
					knight[i].type = ATTACK;
					knight[i].sheet = leftKnightAttack;
					knight[i].velocity.x = 0.0f;
					if (chance < 20 && knight[i].attackTime >= 1.5f) {
						otherArcher[k].health -= knight[i].damage;
						Mix_PlayChannel(-1, hit, 0);
						if (otherArcher[k].health <= 0) {
							Mix_PlayChannel(-1, death, 0);
							otherArcher.erase(otherArcher.begin() + k);
							gold += 125;
						}
						knight[i].attackTime = 0.0f;
					}
					atk = true;
					break;
				}
				++k;
			}
			if (atk)
				continue;
			if (knight[i].position.x + knight[i].sheet.size.x / 3.0f > enemyTower1.position.x - enemyTower1.sheet.size.x / 3.0f
				&& state == STATE_GAME_ATTACK) {
				knight[i].type = ATTACK;
				knight[i].sheet = leftKnightAttack;
				knight[i].velocity.x = 0.0f;
				if (chance < 30 && knight[i].attackTime >= 1.5f) {
					knight[i].velocity.x = 0.0f;
					enemyTower1.health -= 10;
					knight[i].attackTime = 0.0f;
				}
			}
		}

		for (int i = 0; i < otherKnight.size(); ++i) {
			bool atk = false;
			int k = 0;
			int chance = rand() % 1000 + 1;
			otherKnight[i].attackTime += elapsed;
			otherKnight[i].type = WALK;
			otherKnight[i].sheet = rightKnightWalk;
			otherKnight[i].velocity.x = -0.3f;
			while (k < knight.size()) {
				if (otherKnight[i].position.x - otherKnight[i].sheet.size.x / 3.0f <= knight[k].position.x + knight[k].sheet.size.x / 3.0f) {
					otherKnight[i].type = ATTACK;
					otherKnight[i].sheet = rightKnightAttack;
					otherKnight[i].velocity.x = 0.0f;
					if (chance < 5 && otherKnight[i].attackTime >= 1.5f) {
						knight[k].health -= otherKnight[i].damage;
						Mix_PlayChannel(-1, hit, 0);
						if (knight[k].health <= 0) {
							Mix_PlayChannel(-1, death, 0);
							knight.erase(knight.begin() + k);
						}
						otherKnight[i].attackTime = 0.0f;
					}
					atk = true;
					break;
				}
				++k;
			}
			if (atk)
				continue;
			k = 0;
			while (k < archer.size()) {
				if (otherKnight[i].position.x - otherKnight[i].sheet.size.x / 3.0f <= archer[k].position.x + archer[k].sheet.size.x / 3.0f) {
					otherKnight[i].sheet = rightKnightAttack;
					otherKnight[i].velocity.x = 0.0f;
					if (chance < 5 && otherKnight[i].attackTime >= 1.5f) {
						archer[k].health -= otherKnight[i].damage;
						Mix_PlayChannel(-1, hit, 0);
						if (archer[k].health <= 0) {
							Mix_PlayChannel(-1, death, 0);
							archer.erase(archer.begin() + k);
						}
						otherKnight[i].attackTime = 0.0f;
					}
					atk = true;
					break;
				}
				++k;
			}
			if (atk)
				continue;
			if (otherKnight[i].position.x - otherKnight[i].sheet.size.x / 3.0f < pTower.position.x + pTower.sheet.size.x / 3.0f
				&& state == STATE_GAME_DEFEND) {
				otherKnight[i].type = ATTACK;
				otherKnight[i].sheet = rightKnightAttack;
				otherKnight[i].velocity.x = 0.0f;
				if (chance < 30 && otherKnight[i].attackTime >= 1.5f) {
					otherKnight[i].velocity.x = 0.0f;
					pTower.health -= 10;
					otherKnight[i].attackTime = 0.0f;
				}
			}
		}
	}

	if (archer.size() > 0) {
		for (int i = 0; i < archer.size(); ++i) {
			bool atk = false;
			int k = 0;
			int chance = rand() % 1000 + 1;
			archer[i].attackTime += elapsed;
			archer[i].type = WALK;
			archer[i].sheet = leftArcherWalk;
			archer[i].velocity.x = 0.15f;
			while (k < otherKnight.size()) {
				if (otherKnight[k].position.x - 1.0f < archer[i].position.x) {
					archer[i].sheet = leftArcherAttack;
					archer[i].velocity.x = 0.0f;
					if (chance < 5 && archer[i].attackTime >= 2.0f) {
						otherKnight[k].health -= archer[i].damage;
						Mix_PlayChannel(-1, hit, 0);
						if (otherKnight[k].health <= 0) {
							Mix_PlayChannel(-1, death, 0);
							otherKnight.erase(otherKnight.begin() + k);
							gold += 75;
						}
						archer[i].attackTime = 0.0f;
					}
					atk = true;
					break;
				}
				++k;
			}
			if (atk)
				continue;
			k = 0;
			while (k < otherArcher.size()) {
				if (otherArcher[k].position.x - 1.0f < archer[i].position.x) {
					archer[i].sheet = leftArcherAttack;
					archer[i].velocity.x = 0.0f;
					if (chance < 25 && archer[i].attackTime >= 2.0f) {
						otherArcher[k].health -= archer[i].damage;
						Mix_PlayChannel(-1, hit, 0);
						if (otherArcher[k].health <= 0) {
							Mix_PlayChannel(-1, death, 0);
							otherArcher.erase(otherArcher.begin() + k);
							gold += 75;
						}
						archer[i].attackTime = 0.0f;
					}
					atk = true;
					break;
				}
				++k;
			}
			if (atk)
				continue;
			if (archer[i].position.x + 1.0f > enemyTower1.position.x - enemyTower1.sheet.size.x / 2.0f
				&& state == STATE_GAME_ATTACK) {
				archer[i].type = ATTACK;
				archer[i].sheet = leftArcherAttack;
				archer[i].velocity.x = 0.0f;
				if (chance < 50 && archer[i].attackTime >= 2.0f) {
					archer[i].velocity.x = 0.0f;
					enemyTower1.health -= archer[i].damage;
					archer[i].attackTime = 0.0f;
				}
			}
		}
	}

	if (otherArcher.size() > 0) {
		for (int i = 0; i < otherArcher.size(); ++i) {
			bool atk = false;
			int k = 0;
			int chance = rand() % 1000 + 1;
			otherArcher[i].attackTime += elapsed;
			otherArcher[i].type = WALK;
			otherArcher[i].sheet = rightArcherWalk;
			otherArcher[i].velocity.x = -0.15f;
			while (k < knight.size()) {
				if (knight[k].position.x <= otherArcher[i].position.x && knight[k].position.x >= otherArcher[i].position.x - 1.0f) {
					otherArcher[i].sheet = rightArcherAttack;
					if (knight[k].position.x + knight[k].sheet.size.x / 3.0f >= otherArcher[i].position.x - otherArcher[i].sheet.size.x / 3.0f)
						otherArcher[i].velocity.x = 0.0f;
					if (chance < 10 && otherArcher[i].attackTime >= 2.0f) {
						knight[k].health -= otherArcher[i].damage;
						Mix_PlayChannel(-1, hit, 0);
						if (knight[k].health <= 0) {
							Mix_PlayChannel(-1, death, 0);
							knight.erase(knight.begin() + k);
						}
						otherArcher[i].attackTime = 0.0f;
					}
					atk = true;
					break;
				}
				++k;
			}
			if (atk)
				continue;
			k = 0;
			while (k < archer.size()) {
				if (archer[k].position.x + 1.0f >= otherArcher[i].position.x) {
					otherArcher[i].sheet = rightArcherAttack;
					otherArcher[i].velocity.x = 0.0f;
					if (chance < 25 && otherArcher[i].attackTime >= 2.0f) {
						archer[k].health -= otherArcher[i].damage;
						Mix_PlayChannel(-1, hit, 0);
						if (archer[k].health <= 0) {
							Mix_PlayChannel(-1, death, 0);
							archer.erase(archer.begin() + k);
						}
						otherArcher[i].attackTime = 0.0f;
					}
					atk = true;
					break;
				}
				++k;
			}
			if (atk)
				continue;
			if (otherArcher[i].position.x - 1.0f <= pTower.position.x - pTower.sheet.size.x / 2.0f
				&& state == STATE_GAME_DEFEND) {
				otherArcher[i].type = ATTACK;
				otherArcher[i].sheet = rightArcherAttack;
				otherArcher[i].velocity.x = 0.0f;
				if (chance < 50 && otherArcher[i].attackTime >= 2.0f) {
					otherArcher[i].velocity.x = 0.0f;
					pTower.health -= otherArcher[i].damage;
					otherArcher[i].attackTime = 0.0f;
				}
			}
		}
	}
}

void Render() {
	glClear(GL_COLOR_BUFFER_BIT);
	switch (state) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_CONTROLS:
		RenderControls();
		break;
	case STATE_GAME_DEFEND:
		RenderGame();
		break;
	case STATE_GAME_ATTACK:
		RenderGame();
		break;
	case STATE_GAME_NEXT:
		RenderGameNext();
		break;
	case STATE_GAME_DEATH:
		RenderDeath();
		break;
	case STATE_GAME_WIN:
		RenderWin();
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}

void RenderMainMenu() {
	float vertices[] = { -3.55f, -2.0f, 3.55f, -2.0f, 3.55f, 2.0f, -3.55f, -2.0f, 3.55f, 2.0f, -3.55f, 2.0f };
	modelMatrix.identity();
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	glBindTexture(GL_TEXTURE_2D, mainScreen);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);

	modelMatrix.Translate(-1.0f, 1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "SIEGE", 0.5f, -0.25f);

	modelMatrix.Translate(0.0f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "PRESS ENTER", 0.4f, -0.15f);

	modelMatrix.Translate(0.0f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "Controls - V", 0.3f, -0.15f);

	modelMatrix.Translate(-1.2f, -1.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "Press ESC to Quit at Any Time", 0.2f, -0.1f);
}

void RenderControls() {
	modelMatrix.identity();
	float vertices[] = { -3.55f, -2.0f, 3.55f, -2.0f, 3.55f, 2.0f, -3.55f, -2.0f, 3.55f, 2.0f, -3.55f, 2.0f };
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	glBindTexture(GL_TEXTURE_2D, mainScreen);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);

	modelMatrix.Translate(-2.0f, 1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "Controls", 0.5f, -0.25f);

	modelMatrix.Translate(0.2f, -1.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "LEFT/RIGHT - Move Camera", 0.3f, -0.17f);

	modelMatrix.Translate(0.0f, -0.3f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "1 - Summon Knight", 0.3f, -0.17f);

	modelMatrix.Translate(0.0f, -0.3f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "2 - Summon Archer", 0.3f, -0.17f);
}

void RenderGame() {
	modelMatrix.identity();
	float vertices[] = { -3.55f, -1.5f, 3.55f, -1.5f, 3.55f, 2.0f, -3.55f, -1.5f, 3.55f, 2.0f, -3.55f, 2.0f };
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	glBindTexture(GL_TEXTURE_2D, night);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindTexture(GL_TEXTURE_2D, mountains);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindTexture(GL_TEXTURE_2D, trees);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix.identity();
	modelMatrix.Translate(-3.4f, -1.8f, 0.0f);
	modelMatrix.Scale(0.4f, 0.32f, 1.0f);
	program->setModelMatrix(modelMatrix);
	glBindTexture(GL_TEXTURE_2D, leftKnightAttack.sprite);
	DrawSpriteSheetSprite(program, 0, 1, 4);
	modelMatrix.identity();
	modelMatrix.Translate(-3.0f, -1.82f, 0.0f);
	modelMatrix.Scale(0.3f, 0.3f, 1.0f);
	program->setModelMatrix(modelMatrix);
	glBindTexture(GL_TEXTURE_2D, leftArcherAttack.sprite);
	DrawSpriteSheetSprite(program, 0, 4, 1);

	modelMatrix.identity();
	modelMatrix.Translate(-3.5f, -1.7f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "100G", 0.13f, -0.06f);
	modelMatrix.Translate(0.1f, -0.15f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "1", 0.25, 0.0f);
	modelMatrix.identity();
	modelMatrix.Translate(-3.12f, -1.7f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "150G", 0.13f, -0.06f);
	modelMatrix.Translate(0.1f, -0.15f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "2", 0.25f, 0.0f);
	modelMatrix.Translate(0.6f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, std::to_string((int)gold) + "G", 0.25f, -0.1f);
	modelMatrix.Translate(1.0f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "Day:" + std::to_string((int)day), 0.25f, -0.15f);

	if (state == STATE_GAME_DEFEND) {
		modelMatrix.Translate(1.5f, 0.0f, 0.0f);
		program->setModelMatrix(modelMatrix);
		DrawText(program, fontMain, "Enemies Left:" + std::to_string(enemySpawn + otherArcher.size() + otherKnight.size()), 0.25f, -0.15f);

		modelMatrix.identity();
		modelMatrix.Translate(pTower.position.x, pTower.position.y, 0.0f);
		modelMatrix.Scale(pTower.sheet.size.x, pTower.sheet.size.y, 1.0f);
		program->setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, pTower.sheet.sprite);
		DrawSpriteSheetSprite(program, 0, 1, 1);
		DrawText(program, fontMain, std::to_string((int)pTower.health), 0.2f, -0.1f);
	}
	if (state == STATE_GAME_ATTACK) {
		modelMatrix.Translate(1.5f, 0.0f, 0.0f);
		program->setModelMatrix(modelMatrix);
		DrawText(program, fontMain, "Time Left:" + std::to_string((int)(81.0f - roundTimer)), 0.25f, -0.15f);

		if (enemyTower1.health > 0) {
			modelMatrix.identity();
			modelMatrix.Translate(enemyTower1.position.x, enemyTower1.position.y, 0.0f);
			modelMatrix.Scale(enemyTower1.sheet.size.x, enemyTower1.sheet.size.y, 1.0f);
			program->setModelMatrix(modelMatrix);
			glBindTexture(GL_TEXTURE_2D, enemyTower1.sheet.sprite);
			DrawSpriteSheetSprite(program, 0, 1, 1);
			DrawText(program, fontMain, std::to_string((int)enemyTower1.health), 0.2f, -0.1f);
		}
	}

	for (size_t i = 0; i < knight.size(); ++i) {
		modelMatrix.identity();
		modelMatrix.Translate(knight[i].position.x, knight[i].position.y, 0.0f);
		modelMatrix.Scale(knight[i].sheet.size.x, knight[i].sheet.size.y, knight[i].sheet.size.z);
		program->setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, knight[i].sheet.sprite);
		DrawSpriteSheetSprite(program, anim[current], knight[i].sheet.x, knight[i].sheet.y);
		DrawText(program, fontMain, std::to_string((int)knight[i].health), 0.3f, -0.15f);
	}

	for (size_t i = 0; i < archer.size(); ++i) {
		modelMatrix.identity();
		modelMatrix.Translate(archer[i].position.x, archer[i].position.y, 0.0f);
		modelMatrix.Scale(archer[i].sheet.size.x, archer[i].sheet.size.y, archer[i].sheet.size.z);
		program->setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, archer[i].sheet.sprite);
		DrawSpriteSheetSprite(program, anim[current], archer[i].sheet.x, archer[i].sheet.y);
		DrawText(program, fontMain, std::to_string((int)archer[i].health), 0.3f, -0.15f);
	}

	for (size_t i = 0; i < otherKnight.size(); ++i) {
		modelMatrix.identity();
		modelMatrix.Translate(otherKnight[i].position.x, otherKnight[i].position.y, 0.0f);
		modelMatrix.Scale(otherKnight[i].sheet.size.x, otherKnight[i].sheet.size.y, otherKnight[i].sheet.size.z);
		program->setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, otherKnight[i].sheet.sprite);
		DrawSpriteSheetSprite(program, anim[current], otherKnight[i].sheet.x, otherKnight[i].sheet.y);
		DrawText(program, fontMain, std::to_string((int)otherKnight[i].health), 0.3f, -0.15f);
	}

	for (size_t i = 0; i < otherArcher.size(); ++i) {
		modelMatrix.identity();
		modelMatrix.Translate(otherArcher[i].position.x, otherArcher[i].position.y, 0.0f);
		modelMatrix.Scale(otherArcher[i].sheet.size.x, otherArcher[i].sheet.size.y, otherArcher[i].sheet.size.z);
		program->setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, otherArcher[i].sheet.sprite);
		DrawSpriteSheetSprite(program, anim[current], otherArcher[i].sheet.x, otherArcher[i].sheet.y);
		DrawText(program, fontMain, std::to_string((int)otherArcher[i].health), 0.3f, -0.15f);
	}
}

void RenderGameNext() {
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	float vertices[] = { -3.55f, -2.0f, 3.55f, -2.0f, 3.55f, 2.0f, -3.55f, -2.0f, 3.55f, 2.0f, -3.55f, 2.0f };
	glBindTexture(GL_TEXTURE_2D, bg);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix.Translate(-1.0f, 0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "GOLD:" + std::to_string(gold), 0.4f, -0.2f);

	modelMatrix.Translate(-0.5f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "A. ATTACK", 0.3f, -0.1f);

	modelMatrix.Translate(0.0f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "D. REST", 0.3f, -0.1f);

	modelMatrix.Translate(-0.5f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	if (enemyTower1.health <= 0)
		DrawText(program, fontMain, "Enemy Tower 1: DOWN", 0.3f, -0.15f);
}

void RenderDeath() {
	modelMatrix.identity();
	modelMatrix.Translate(0.0f, -0.3f, 0.0f);
	float vertices[] = { -3.55f, -1.5f, 3.55f, -1.5f, 3.55f, 2.0f, -3.55f, -1.5f, 3.55f, 2.0f, -3.55f, 2.0f };
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	glBindTexture(GL_TEXTURE_2D, defeat);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix.identity();
	modelMatrix.Translate(-0.6f, 1.4f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "DEFEAT", 0.5f, -0.25f);

	modelMatrix.identity();
	modelMatrix.Translate(-0.5f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "DAY:" + std::to_string(day), 0.5f, -0.25f);

	modelMatrix.Translate(0.0, -1.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "PRESS ENTER", 0.2f, -0.1f);
}

void RenderWin() {
	modelMatrix.identity();
	modelMatrix.Scale(2.0f, 2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	glBindTexture(GL_TEXTURE_2D, victory);
	DrawSpriteSheetSprite(program, 1, 5, 6);

	modelMatrix.identity();
	modelMatrix.Translate(-0.7f, 1.4f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "VICTORY", 0.5f, -0.25f);

	modelMatrix.identity();
	modelMatrix.Translate(-0.5f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "DAY:" + std::to_string(day), 0.5f, -0.25f);

	modelMatrix.Translate(0.0, -1.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontMain, "PRESS ENTER", 0.2f, -0.1f);
}