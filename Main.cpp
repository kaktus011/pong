#include <chrono>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
//const int WINDOW_WIDTH = 1280;
//const int WINDOW_HEIGHT = 720;

const int BALL_WIDTH = 15;
const int BALL_HEIGHT = 15;
const float BALL_SPEED = 1.0f;

const int PADDLE_WIDTH = 15;
const int PADDLE_HEIGHT = 100;
const float PADDLE_SPEED = 1.0f;

enum Buttons {
  PaddleOneUp = 0,
  PaddleOneDown,
  PaddleTwoUp,
  PaddleTwoDown,
};

enum CollisionType {
  None,
  Top,
  Middle,
  Bottom,
  Left,
  Right,
};

struct Contact {
  CollisionType type;
  float penetration;
};

class Vec2
{
public:
  Vec2()
    : x(0.0f), y(0.0f)
  {}

  Vec2(float x, float y)
    : x(x), y(y)
  {}

  Vec2 operator +(Vec2 const& rhs) {
    return Vec2(x + rhs.x, y + rhs.y);
  }

  Vec2& operator +=(Vec2 const& rhs) {
    x += rhs.x;
    y += rhs.y;

    return *this;
  }

  Vec2 operator *(float rhs) {
    return Vec2(x * rhs, y * rhs);
  }

  float x, y;
};

class Ball 
{
public:
  Ball(Vec2 position, Vec2 velocity) 
    : position(position), velocity(velocity) {
      rect.x = static_cast<int>(position.x);
      rect.y = static_cast<int>(position.y);
      rect.w = BALL_WIDTH;
      rect.h = BALL_HEIGHT;
    }

  void Draw(SDL_Renderer* renderer) {
    rect.x = static_cast<int>(position.x);
    rect.y = static_cast<int>(position.y);

    SDL_RenderFillRect(renderer, &rect);
  }

  void Update(float dt) {
    position += velocity * dt;
  }

  void CollideWithPaddle(Contact const& contact) {
    position.x += contact.penetration;
    velocity.x *= -1;

    if (contact.type == CollisionType::Top) {
      velocity.y = -1 * 0.75f * BALL_SPEED;
    }
    else if (contact.type == CollisionType::Bottom) {
      velocity.y = 0.75f * BALL_SPEED;
    }
  }

  void CollideWithWall(Contact const& contact) {
    if (contact.type == CollisionType::Top || contact.type == CollisionType::Bottom) {
      position.y += contact.penetration;
      velocity.y *= -1;
    }
    else if (contact.type == CollisionType::Left) {
      position.x = WINDOW_WIDTH / 2.0f;
      position.y = WINDOW_HEIGHT / 2.0f;
      velocity.x = BALL_SPEED; //go right
      velocity.y = 0.75f * BALL_SPEED;
    }
    else if (contact.type == CollisionType::Right) {
      position.x = WINDOW_WIDTH / 2.0f;
      position.y = WINDOW_HEIGHT / 2.0f;
      velocity.x = -1 * BALL_SPEED; //go left
      velocity.y = 0.75f * BALL_SPEED;
    }
  }

  Vec2 position;
  Vec2 velocity;
  SDL_Rect rect;
};

class Paddle
{
public:
  Paddle(Vec2 position, Vec2 velocity)
    : position(position), velocity(velocity) {
      rect.x = static_cast<int>(position.x);
      rect.y = static_cast<int>(position.y);
      rect.w = PADDLE_WIDTH;
      rect.h = PADDLE_HEIGHT;
    }

  void Draw(SDL_Renderer* renderer) {
    rect.y = position.y;

    SDL_RenderFillRect(renderer, &rect);
  }

  void Update(float dt) {
    position += velocity * dt;

    if (position.y < 0) {
      position.y = 0;
    }
    else if (position.y > WINDOW_HEIGHT - PADDLE_HEIGHT) {
      position.y = WINDOW_HEIGHT - PADDLE_HEIGHT;
    }
  }

  Vec2 position;
  Vec2 velocity;
  SDL_Rect rect;
};

class PlayerScore
{
public:
	PlayerScore(Vec2 position, SDL_Renderer* renderer, TTF_Font* font)
		: renderer(renderer), font(font) {
		surface = TTF_RenderText_Solid(font, "0", {0xFF, 0xFF, 0xFF, 0xFF});
		texture = SDL_CreateTextureFromSurface(renderer, surface);

		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = width;
		rect.h = height;
	}

	~PlayerScore() {
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);
	}

	void Draw() {
		SDL_RenderCopy(renderer, texture, nullptr, &rect);
	}

  void SetScore(int score) {
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);

		surface = TTF_RenderText_Solid(font, std::to_string(score).c_str(), {0xFF, 0xFF, 0xFF, 0xFF});
		texture = SDL_CreateTextureFromSurface(renderer, surface);

    int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
		rect.w = width;
		rect.h = height;
  }

	SDL_Renderer* renderer;
	TTF_Font* font;
	SDL_Surface* surface{};
	SDL_Texture* texture{};
	SDL_Rect rect{};
};

Contact CheckPaddleCollision(Ball const& ball, Paddle const& paddle) {
  float ballLeft = ball.position.x;
  float ballRight = ball.position.x + BALL_WIDTH;
  float ballTop = ball.position.y;
  float ballBottom = ball.position.y + BALL_HEIGHT;

  float paddleLeft = paddle.position.x;
  float paddleRight = paddle.position.x + PADDLE_WIDTH;
  float paddleTop = paddle.position.y;
  float paddleBottom = paddle.position.y + PADDLE_HEIGHT;

  Contact contact{};
  if ((ballLeft >= paddleRight) || (ballRight <= paddleLeft)) {
    return contact;
  }
  if ((ballTop >= paddleBottom) || (ballBottom <= paddleTop)) {
    return contact;
  }

  float paddleUpperPart = paddleBottom - (2.0f * PADDLE_HEIGHT / 3.0f);
  float paddleMiddlePart = paddleBottom - (PADDLE_HEIGHT / 3.0f);

  if (ball.velocity.x < 0) {
    contact.penetration = paddleRight - ballLeft;
  }
  else if (ball.velocity.x > 0) {
    contact.penetration = paddleLeft - ballRight;
  }

  if (ballBottom > paddleTop && ballBottom < paddleUpperPart) {
    contact.type = CollisionType::Top;
  }
  else if (ballBottom > paddleUpperPart && ballBottom < paddleMiddlePart) {
    contact.type = CollisionType::Middle;
  }
  else {
    contact.type = CollisionType::Bottom;
  }

  return contact;
};

Contact CheckWallCollision(Ball const& ball) {
  float ballLeft = ball.position.x;
  float ballRight = ball.position.x + BALL_WIDTH;
  float ballTop = ball.position.y;
  float ballBottom = ball.position.y + BALL_HEIGHT;
  Contact contact{};

  if (ballLeft < 0.0f) {
    contact.type = CollisionType::Left;
  }
  else if (ballRight > WINDOW_WIDTH) {
    contact.type = CollisionType::Right;
  }
  else if (ballTop < 0.0f) {
    contact.type = CollisionType::Top;
    contact.penetration = -1 * ballTop;
  }
  else if (ballBottom > WINDOW_HEIGHT) {
    contact.type = CollisionType::Bottom;
    contact.penetration = WINDOW_HEIGHT - ballBottom;
  }

  return contact;
};

int main()
{
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

  SDL_Window* window = SDL_CreateWindow("Pong", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

  TTF_Font* scoreFont = TTF_OpenFont("0xProto-Regular.ttf", 40);

  Mix_Chunk* wallHitSound = Mix_LoadWAV("wallHit.wav");
  Mix_Chunk* paddleHitSound = Mix_LoadWAV("paddleHit.wav");

  {
    Ball ball (
        Vec2(
          (WINDOW_WIDTH / 2.0f) - (BALL_WIDTH / 2.0f),
          (WINDOW_HEIGHT / 2.0f) - (BALL_HEIGHT / 2.0f)),
        Vec2(BALL_SPEED, 0.0f));

    Paddle paddleOne(
        Vec2(50.0f, WINDOW_HEIGHT / 2.0f),
        Vec2(0.0f, 0.0f));

    Paddle paddleTwo(
        Vec2(WINDOW_WIDTH - 50.0f, WINDOW_HEIGHT / 2.0f),
        Vec2(0.0f, 0.0f));

    PlayerScore playerOneScoreText(Vec2(WINDOW_WIDTH / 4, 20), renderer, scoreFont);
    PlayerScore playerTwoScoreText(Vec2(3 * WINDOW_WIDTH / 4, 20), renderer, scoreFont);

    int playerOneScore = 0;
    int playerTwoScore = 0;

    bool running = true;
    float dt = 0.0f;
    bool buttons[4] = {};

    while (running) {
      SDL_Event event;
      auto startTime = std::chrono::high_resolution_clock::now();


      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
          running = false;
        }
        else if (event.type == SDL_KEYDOWN) {
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
          }
          else if (event.key.keysym.sym == SDLK_w) {
            buttons[Buttons::PaddleOneUp] = true;
          }
          else if (event.key.keysym.sym == SDLK_s) {
            buttons[Buttons::PaddleOneDown] = true;
          }
          else if (event.key.keysym.sym == SDLK_UP) {
            buttons[Buttons::PaddleTwoUp] = true;
          }
          else if (event.key.keysym.sym == SDLK_DOWN) {
            buttons[Buttons::PaddleTwoDown] = true;
          }
        }
        else if (event.type == SDL_KEYUP) {
          if (event.key.keysym.sym == SDLK_w) {
            buttons[Buttons::PaddleOneUp] = false;
          }
          else if (event.key.keysym.sym == SDLK_s) {
            buttons[Buttons::PaddleOneDown] = false;
          }
          else if (event.key.keysym.sym == SDLK_UP) {
            buttons[Buttons::PaddleTwoUp] = false;
          }
          else if (event.key.keysym.sym == SDLK_DOWN) {
            buttons[Buttons::PaddleTwoDown] = false;
          }
        }
      }

      if (buttons[Buttons::PaddleOneUp]) {
        paddleOne.velocity.y = PADDLE_SPEED * -1;
      }
      else if (buttons[Buttons::PaddleOneDown]) {
        paddleOne.velocity.y = PADDLE_SPEED;
      }
      else {
        paddleOne.velocity.y = 0.0f;
      }

      if (buttons[Buttons::PaddleTwoUp]) {
        paddleTwo.velocity.y = PADDLE_SPEED * -1;
      }
      else if (buttons[Buttons::PaddleTwoDown]) {
        paddleTwo.velocity.y = PADDLE_SPEED;
      }
      else {
        paddleTwo.velocity.y = 0.0f;
      }

      SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xFF);
      SDL_RenderClear(renderer);

      SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);


      paddleOne.Draw(renderer);
      paddleTwo.Draw(renderer);
      paddleOne.Update(dt);
      paddleTwo.Update(dt);

      ball.Draw(renderer);
      ball.Update(dt);

      if (Contact contact = CheckPaddleCollision(ball, paddleOne); contact.type != CollisionType::None) {
        ball.CollideWithPaddle(contact);
        Mix_PlayChannel(-1, paddleHitSound, 0);
      }
      else if (contact = CheckPaddleCollision(ball, paddleTwo); contact.type != CollisionType::None) {
        ball.CollideWithPaddle(contact);
        Mix_PlayChannel(-1, paddleHitSound, 0);
      }
      else if(contact = CheckWallCollision(ball); contact.type != CollisionType::None) {
        ball.CollideWithWall(contact);

        if (contact.type == CollisionType::Left) {
          ++playerTwoScore;
          playerTwoScoreText.SetScore(playerTwoScore);
        }
        else if (contact.type == CollisionType::Right) {
          ++playerOneScore;
          playerOneScoreText.SetScore(playerOneScore);
        }
        else {
          Mix_PlayChannel(-1, wallHitSound, 0);
        }
      }

      playerOneScoreText.Draw();
      playerTwoScoreText.Draw();

      for (int y = 0; y < WINDOW_HEIGHT; ++y) {
        if (y % 5) {
          SDL_RenderDrawPoint(renderer, WINDOW_WIDTH / 2, y);
        }
      }

      SDL_RenderPresent(renderer);
      //frame time
      auto stopTime = std::chrono::high_resolution_clock::now();
      dt = std::chrono::duration<float, std::chrono::milliseconds::period>(stopTime - startTime).count();
    }
  }

  Mix_FreeChunk(wallHitSound);
  Mix_FreeChunk(paddleHitSound);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_CloseFont(scoreFont);
  Mix_Quit();
  TTF_Quit();
  SDL_Quit();

  return 0;
}
