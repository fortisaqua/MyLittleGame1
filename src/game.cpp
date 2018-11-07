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
	// 统一投影矩阵，因为是2D游戏，所以只需要制指定窗口尺寸，以及映射到的区间，就能保证
	// 渲染时在窗口尺寸里的坐标都能正确显示，参数代表 左、右、下、上边界，
	// 并且把所有在0到800之间的x坐标变换到-1到1之间，并把所有在0到600之间的y坐标变换到-1到1之间
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
	// 保证所有的砖块在窗口的上半部分，所以使用的是 height * 0.5 
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
	// 将滑板放在底部中间，因为正射投影的效果是以左上角的坐标为
	// 坐标值的最小值，右下角为坐标值的最大值来映射到 -1到1 的
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
		// 因为这里是2D游戏界面，所以没有深度检测机制，想要实现前后层次，比如底层是背景图片
		// 就要自行设置绘制顺序，先绘制的在底下
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
	// 检查球对象与每一个砖块是否发生碰撞
	// C++中迭代的一种写法，前面加了 & 就是为了能对迭代到的元素对象进行直接改写
	for (GameObject &box : this->Levels[this->Level].Bricks) {
		if (!box.Destroyed) {
			Collision collision = CheckCollision(*Ball, box);
			// 与 tuple<GLboolean, Direction, glm::vec2> 配合使用
			// 这里代表读取特定 tuple 对象中第0个位置的数值，也就是 GLboolean 型数值
			if (std::get<0>(collision)) {
				// 如果发生碰撞，则将非固定砖块设为“已破坏”状态，下一次循环将不再渲染这块砖块
				if (!box.IsSolid)
					box.Destroyed = true;
				// 处理发生碰撞后的碰撞恢复以及反弹
				Direction dir = std::get<1>(collision);
				glm::vec2 diff_vector = std::get<2>(collision);
				if (dir == LEFT || dir == RIGHT) {
					Ball->Velocity.x = -Ball->Velocity.x; // 反转横坐标上的速度（horizontal）
					GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
					Ball->Position.x += penetration * (dir == LEFT ? 1 : -1);
				}
				if (dir == UP || dir == DOWN) {
					Ball->Velocity.y = -Ball->Velocity.y; // 反转纵坐标上的速度（vertical）
					GLfloat penetration = Ball->Radius - std::abs(diff_vector.y);
					Ball->Position.y += penetration * (dir == UP ? -1 : 1);
				}
			}
		}
	}
	// 检查是否与玩家控制挡板碰撞
	Collision result = CheckCollision(*Ball, *Player);
	if (!Ball->Stuck && std::get<0>(result)) {
		// 实现一个特效：玩家接住球的位置会相应地改变球在横坐标上速度分量的大小
		GLfloat centerBoard = Player->Position.x + Player->Size.x / 2;
		GLfloat distance = (Ball->Position.x + Ball->Radius) - centerBoard;
		GLfloat percentage = distance / (Player->Size.x / 2);
		// 调整球的速度方向和大小
		GLfloat strength = 2.0f;
		glm::vec2 oldVelocity = Ball->Velocity;
		// 为了不让球的速度分量比例变得太离谱，每次横轴上的变速都以初速度为基础
		Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
		// 然后还要保持速度的总量不变，即要保持速度向量的长度不变
		Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);
		// 保证球在纵坐标上的速度分量是往上走的
		Ball->Velocity.y = -1 * abs(Ball->Velocity.y);
	}
}

// 最简单的碰撞检测，检测双方均以矩形外边框作为碰撞外形
GLboolean CheckCollision(GameObject &one, GameObject &two) // AABB - AABB collision
{
	// Collision x-axis?
	// 即判断两个游戏对象在x轴上是否有重合
	// 相当于是两个游戏对象的左上角顶点横坐标中
	// 的一个在另一个的横坐标的范围内
	bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
		two.Position.x + two.Size.x >= one.Position.x;
	// Collision y-axis?
	// 即判断两个游戏对象在y轴上是否有重合
	// 相当于是两个游戏对象的左上角顶点纵坐标中
	// 的一个在另一个的纵坐标的范围内
	bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
		two.Position.y + two.Size.y >= one.Position.y;
	// Collision only if on both axes
	return collisionX && collisionY;
}

// 矩形外边框 - 圆形外边框碰撞检测
Collision CheckCollision(BallObject &one, GameObject &two) // AABB - Circle collision
{
	// 计算圆形碰撞边框的圆心
	// Get center point circle first 
	glm::vec2 center(one.Position + one.Radius);
	// 计算矩形碰撞边框的中心与相对偏移量上限
	//（用以去处圆心到此中心的向量中，处于矩形内部的部分）
	// Calculate AABB info (center, half-extents)
	glm::vec2 aabb_half_extents(two.Size.x / 2, two.Size.y / 2);
	glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
	// Get difference vector between both centers
	// 计算从矩形中心到圆心的向量，并分离出矩形内的分量 clamped
	glm::vec2 difference = center - aabb_center;
	glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
	// Now that we know the the clamped values, add this to AABB_center and we get the value of box closest to circle
	//获取矩形边上到圆心最近的点，也就是从矩形中心偏移一个矩形内分量 clamped
	glm::vec2 closest = aabb_center + clamped;
	// Now retrieve vector between center circle and closest point AABB and check if length < radius
	// 计算圆心到矩形边上最近点的向量，用以计算碰撞边具体是哪一条边（上下左右）以及距离长度（小于半径才是真的碰撞了）
	difference = closest - center;

	// 真的有重合了才能算是碰撞，刚刚碰到不算，所以是 < 而不是 <=
	if (glm::length(difference) < one.Radius)
	{
		std::cout<< "(" << difference[0] << " , " << difference[1] << ")" << std::endl;
		// 返回数值代表 (是否碰撞，碰撞方向--以圆心为出发点看，
		//               圆心与碰撞体最近的向量--用于碰撞恢复，即按照最近的距离和方向复位到没有重合)
		return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
	}
	else
		return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}

// Calculates which direction a vector is facing (N,E,S or W)
// 每一个compass代表一个撞击的方向，target是一个从圆心到矩形的最近
// 向量的方向，它与哪一条边的方向cos值最大说明夹角越小，找出夹角最小的方向，就是它撞击的方向
// 这个 target 比直接用两个中心的向量更加直接，因为这样就能将夹角的数值限定在一个很小的集合内
// 对于矩形，就是 0， 90， 180 这三种
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