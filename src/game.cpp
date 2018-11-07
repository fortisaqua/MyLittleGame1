/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "game_object.h"
#include "ball_object.h"
#include <iostream>

// Game-related State data
SpriteRenderer  *Renderer;
GameObject      *Player;
BallObject      *Ball;

// Collision detection
GLboolean CheckCollision(GameObject &one, GameObject &two);
Collision CheckCollision(BallObject &one, GameObject &two);
Direction VectorDirection(glm::vec2 closest);


Game::Game(GLuint width, GLuint height)
	: State(GAME_ACTIVE), Keys(), KeyPress(), KeyState(), Width(width), Height(height)
{

}

Game::~Game()
{
	delete Renderer;
	delete Player;
	delete Ball;
}

void Game::Init()
{
	// Load shaders
	ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
	// Configure shaders 
	// ͳһͶӰ������Ϊ��2D��Ϸ������ֻ��Ҫ��ָ�����ڳߴ磬�Լ�ӳ�䵽�����䣬���ܱ�֤
	// ��Ⱦʱ�ڴ��ڳߴ�������궼����ȷ��ʾ���������� ���ҡ��¡��ϱ߽磬
	// ���Ұ�������0��800֮���x����任��-1��1֮�䣬����������0��600֮���y����任��-1��1֮��
	glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(this->Width), static_cast<GLfloat>(this->Height), 0.0f, -1.0f, 1.0f);
	ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
	ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
	// Load textures
	ResourceManager::LoadTexture("textures/background.jpg", GL_FALSE, "background");
	ResourceManager::LoadTexture("textures/awesomeface.png", GL_TRUE, "face");
	ResourceManager::LoadTexture("textures/block.png", GL_FALSE, "block");
	ResourceManager::LoadTexture("textures/block_solid.png", GL_FALSE, "block_solid");
	ResourceManager::LoadTexture("textures/paddle.png", GL_TRUE, "paddle");
	// Set render-specific controls
	Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
	// Load levels
	// ��֤���е�ש���ڴ��ڵ��ϰ벿�֣�����ʹ�õ��� height * 0.5 
	GameLevel one; one.Load("levels/one.lvl", this->Width, this->Height * 0.5);
	GameLevel two; two.Load("levels/two.lvl", this->Width, this->Height * 0.5);
	GameLevel three; three.Load("levels/three.lvl", this->Width, this->Height * 0.5);
	GameLevel four; four.Load("levels/four.lvl", this->Width, this->Height * 0.5);
	this->Levels.push_back(one);
	this->Levels.push_back(two);
	this->Levels.push_back(three);
	this->Levels.push_back(four);
	this->Level = 0;
	// Configure geme objects
	// ��������ڵײ��м䣬��Ϊ����ͶӰ��Ч���������Ͻǵ�����Ϊ
	// ����ֵ����Сֵ�����½�Ϊ����ֵ�����ֵ��ӳ�䵽 -1��1 ��
	glm::vec2 playerPos = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2,
									this->Height - PLAYER_SIZE.y);
	Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
	glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -BALL_RADIUS * 2);
	Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY,
		ResourceManager::GetTexture("face"));
}

void Game::Update(GLfloat dt)
{
	// Update objects
	Ball->Move(dt, this->Width);

	//Check for collisions
	this->DoCollisions();

	//Check for lossing game condition -- falling out of window range
	if (Ball->Position.y >= this->Height) {
		this->ResetLevel();
		this->ResetPlayer();
	}
}


void Game::ProcessInput(GLfloat dt)
{
	if (this->State == GAME_ACTIVE)
	{
		GLfloat velocity = PLAYER_VELOCITY * dt;
		if (this->Keys[GLFW_KEY_A]) {
			if (Player->Position.x >= 0) {
				Player->Position.x -= velocity;
				if (Ball->Stuck)
					Ball->Position.x -= velocity;
			}
		}
		if (this->Keys[GLFW_KEY_D]) {
			if (Player->Position.x <= this->Width - Player->Size.x){
				Player->Position.x += velocity;
				if (Ball->Stuck)
					Ball->Position.x += velocity;
			}
		}
		if (this->KeyState[GLFW_KEY_ENTER] == GLFW_PRESS && !this->KeyPress[GLFW_KEY_ENTER]) {
			this->KeyPress[GLFW_KEY_ENTER] = true;
			this->Level = (this->Level + 1) % this->Levels.size();
		}
		if (this->KeyState[GLFW_KEY_ENTER] == GLFW_RELEASE && this->KeyPress[GLFW_KEY_ENTER]) {
			this->KeyPress[GLFW_KEY_ENTER] = false;
		}
		if (this->Keys[GLFW_KEY_SPACE])
			Ball->Stuck = false;
		/*if (this->KeyState[GLFW_KEY_SPACE] == GLFW_PRESS && !this->KeyPress[GLFW_KEY_SPACE]) {
			Ball->Stuck = true;
			this->KeyPress[GLFW_KEY_SPACE] = true;
		}
		if (this->KeyState[GLFW_KEY_SPACE] == GLFW_RELEASE && this->KeyPress[GLFW_KEY_SPACE]) {
			Ball->Stuck = false;
			this->KeyPress[GLFW_KEY_SPACE] = false;
		}*/
			
	}
}

void Game::Render()
{
	if (this->State == GAME_ACTIVE)
	{
		// ��Ϊ������2D��Ϸ���棬����û����ȼ����ƣ���Ҫʵ��ǰ���Σ�����ײ��Ǳ���ͼƬ
		// ��Ҫ�������û���˳���Ȼ��Ƶ��ڵ���
		// Draw background
		Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0, 0), glm::vec2(this->Width, this->Height), 0.0f);
		// Draw level
		this->Levels[this->Level].Draw(*Renderer);
		// Draw player
		Player->Draw(*Renderer);
		// Draw ball
		Ball->Draw(*Renderer);
	}
}

void Game::ResetLevel(){
	if (this->Level == 0)this->Levels[0].Load("levels/one.lvl", this->Width, this->Height * 0.5f);
	else if (this->Level == 1)
		this->Levels[1].Load("levels/two.lvl", this->Width, this->Height * 0.5f);
	else if (this->Level == 2)
		this->Levels[2].Load("levels/three.lvl", this->Width, this->Height * 0.5f);
	else if (this->Level == 3)
		this->Levels[3].Load("levels/four.lvl", this->Width, this->Height * 0.5f);
}

void Game::ResetPlayer() {
	// Reset player and ball states
	Player->Size = PLAYER_SIZE;
	Player->Position = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
	Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -(BALL_RADIUS * 2)),INITIAL_BALL_VELOCITY);
}

void Game::DoCollisions() {
	// Check for ball - bricks collisions
	// ����������ÿһ��ש���Ƿ�����ײ
	// C++�е�����һ��д����ǰ����� & ����Ϊ���ܶԵ�������Ԫ�ض������ֱ�Ӹ�д
	for (GameObject &box : this->Levels[this->Level].Bricks) {
		if (!box.Destroyed) {
			Collision collision = CheckCollision(*Ball, box);
			// �� tuple<GLboolean, Direction, glm::vec2> ���ʹ��
			// ��������ȡ�ض� tuple �����е�0��λ�õ���ֵ��Ҳ���� GLboolean ����ֵ
			if (std::get<0>(collision)) {
				// ���������ײ���򽫷ǹ̶�ש����Ϊ�����ƻ���״̬����һ��ѭ����������Ⱦ���ש��
				if (!box.IsSolid)
					box.Destroyed = true;
				// ��������ײ�����ײ�ָ��Լ�����
				Direction dir = std::get<1>(collision);
				glm::vec2 diff_vector = std::get<2>(collision);
				if (dir == LEFT || dir == RIGHT) {
					Ball->Velocity.x = -Ball->Velocity.x; // ��ת�������ϵ��ٶȣ�horizontal��
					GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
					Ball->Position.x += penetration * (dir == LEFT ? 1 : -1);
				}
				if (dir == UP || dir == DOWN) {
					Ball->Velocity.y = -Ball->Velocity.y; // ��ת�������ϵ��ٶȣ�vertical��
					GLfloat penetration = Ball->Radius - std::abs(diff_vector.y);
					Ball->Position.y += penetration * (dir == UP ? -1 : 1);
				}
			}
		}
	}
	// ����Ƿ�����ҿ��Ƶ�����ײ
	Collision result = CheckCollision(*Ball, *Player);
	if (!Ball->Stuck && std::get<0>(result)) {
		// ʵ��һ����Ч����ҽ�ס���λ�û���Ӧ�ظı����ں��������ٶȷ����Ĵ�С
		GLfloat centerBoard = Player->Position.x + Player->Size.x / 2;
		GLfloat distance = (Ball->Position.x + Ball->Radius) - centerBoard;
		GLfloat percentage = distance / (Player->Size.x / 2);
		// ��������ٶȷ���ʹ�С
		GLfloat strength = 2.0f;
		glm::vec2 oldVelocity = Ball->Velocity;
		// Ϊ�˲�������ٶȷ����������̫���ף�ÿ�κ����ϵı��ٶ��Գ��ٶ�Ϊ����
		Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
		// Ȼ��Ҫ�����ٶȵ��������䣬��Ҫ�����ٶ������ĳ��Ȳ���
		Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);
		// ��֤�����������ϵ��ٶȷ����������ߵ�
		Ball->Velocity.y = -1 * abs(Ball->Velocity.y);
	}
}

// ��򵥵���ײ��⣬���˫�����Ծ�����߿���Ϊ��ײ����
GLboolean CheckCollision(GameObject &one, GameObject &two) // AABB - AABB collision
{
	// Collision x-axis?
	// ���ж�������Ϸ������x�����Ƿ����غ�
	// �൱����������Ϸ��������ϽǶ����������
	// ��һ������һ���ĺ�����ķ�Χ��
	bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
		two.Position.x + two.Size.x >= one.Position.x;
	// Collision y-axis?
	// ���ж�������Ϸ������y�����Ƿ����غ�
	// �൱����������Ϸ��������ϽǶ�����������
	// ��һ������һ����������ķ�Χ��
	bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
		two.Position.y + two.Size.y >= one.Position.y;
	// Collision only if on both axes
	return collisionX && collisionY;
}

// ������߿� - Բ����߿���ײ���
Collision CheckCollision(BallObject &one, GameObject &two) // AABB - Circle collision
{
	// ����Բ����ײ�߿��Բ��
	// Get center point circle first 
	glm::vec2 center(one.Position + one.Radius);
	// ���������ײ�߿�����������ƫ��������
	//������ȥ��Բ�ĵ������ĵ������У����ھ����ڲ��Ĳ��֣�
	// Calculate AABB info (center, half-extents)
	glm::vec2 aabb_half_extents(two.Size.x / 2, two.Size.y / 2);
	glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
	// Get difference vector between both centers
	// ����Ӿ������ĵ�Բ�ĵ�������������������ڵķ��� clamped
	glm::vec2 difference = center - aabb_center;
	glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
	// Now that we know the the clamped values, add this to AABB_center and we get the value of box closest to circle
	//��ȡ���α��ϵ�Բ������ĵ㣬Ҳ���ǴӾ�������ƫ��һ�������ڷ��� clamped
	glm::vec2 closest = aabb_center + clamped;
	// Now retrieve vector between center circle and closest point AABB and check if length < radius
	// ����Բ�ĵ����α������������������Լ�����ײ�߾�������һ���ߣ��������ң��Լ����볤�ȣ�С�ڰ뾶���������ײ�ˣ�
	difference = closest - center;

	// ������غ��˲���������ײ���ո��������㣬������ < ������ <=
	if (glm::length(difference) < one.Radius)
	{
		std::cout<< "(" << difference[0] << " , " << difference[1] << ")" << std::endl;
		// ������ֵ���� (�Ƿ���ײ����ײ����--��Բ��Ϊ�����㿴��
		//               Բ������ײ�����������--������ײ�ָ�������������ľ���ͷ���λ��û���غ�)
		return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
	}
	else
		return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}

// Calculates which direction a vector is facing (N,E,S or W)
// ÿһ��compass����һ��ײ���ķ���target��һ����Բ�ĵ����ε����
// �����ķ���������һ���ߵķ���cosֵ���˵���н�ԽС���ҳ��н���С�ķ��򣬾�����ײ���ķ���
// ��� target ��ֱ�����������ĵ���������ֱ�ӣ���Ϊ�������ܽ��нǵ���ֵ�޶���һ����С�ļ�����
// ���ھ��Σ����� 0�� 90�� 180 ������
Direction VectorDirection(const glm::vec2 target)
{
	glm::vec2 compass[] = {
		glm::vec2(0.0f, 1.0f),	// up
		glm::vec2(1.0f, 0.0f),	// right
		glm::vec2(0.0f, -1.0f),	// down
		glm::vec2(-1.0f, 0.0f)	// left
	};
	GLfloat max = 0.0f;
	GLuint best_match = -1;
	for (GLuint i = 0; i < 4; i++)
	{
		GLfloat dot_product = glm::dot(glm::normalize(target), compass[i]);
		if (dot_product > max)
		{
			max = dot_product;
			best_match = i;
		}
	}
	return (Direction)best_match;
}