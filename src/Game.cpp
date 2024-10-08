#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "../includes/CollisionHelpers.h"
#include "../includes/EntityManager.h"
#include "../includes/EntityTags.h"
#include "../includes/Game.h"

Game::Game() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL video system is not ready to go: " << SDL_GetError() << std::endl;
    return;
  }
  std::cout << "SDL video system is ready to go" << std::endl;

  const std::string &WINDOW_TITLE = m_gameConfig.windowTitle;

  m_window = SDL_CreateWindow(WINDOW_TITLE.c_str(), SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 1366, 768, SDL_WINDOW_SHOWN);
  if (m_window == nullptr) {
    std::cerr << "Window could not be created: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return;
  }
  std::cout << "Window created successfully!" << std::endl;

  m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
  if (m_renderer == nullptr) {
    std::cerr << "Renderer could not be created: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(m_window);
    SDL_Quit();
    return;
  }
  std::cout << "Renderer created successfully!" << std::endl;

  SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
  SDL_RenderClear(m_renderer);
  SDL_RenderPresent(m_renderer);

  m_isRunning = true; // Set isRunning to true after successful initialization

  std::cout << "Game initialized successfully, use the WASD keys to move the player"
            << std::endl;

  spawnPlayer();
}
Game::~Game() {
  if (m_renderer != nullptr) {
    SDL_DestroyRenderer(m_renderer);
    m_renderer = nullptr;
  }
  if (m_window != nullptr) {
    SDL_DestroyWindow(m_window);
    m_window = nullptr;
  }
  SDL_Quit();
  std::cout << "Cleanup completed, SDL exited." << std::endl;
}

void Game::mainLoop(void *arg) {
  Game *game = static_cast<Game *>(arg);

  game->sInput();

  if (!game->m_paused) {
    game->sMovement();
    game->sCollision();
    game->sSpawner();
    game->sLifespan();
    game->sRender();
  }
}

void Game::run() {
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(Game::mainLoop, this, 0, 1);
#else
  while (m_isRunning) {
    mainLoop(this);
  }
#endif
  std::cout << "Game loop exited" << std::endl;
}

void Game::sInput() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {

    if (event.type == SDL_QUIT) {
      m_isRunning = false; // Stop the game loop
      return;
    }

    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_w: {
          m_player->cInput->forward = true;
          break;
        }
        case SDLK_s: {
          m_player->cInput->backward = true;
          break;
        }

        case SDLK_a: {
          m_player->cInput->left = true;
          break;
        }

        case SDLK_d: {
          m_player->cInput->right = true;
          break;
        }

        case SDLK_p: {
          setPaused(!m_paused);
          break;
        }
      }
    }

    if (event.type == SDL_KEYUP) {
      switch (event.key.keysym.sym) {
        case SDLK_w: {
          m_player->cInput->forward = false;
          break;
        }
        case SDLK_s: {
          m_player->cInput->backward = false;
          break;
        }
        case SDLK_a: {
          m_player->cInput->left = false;
          break;
        }
        case SDLK_d: {
          m_player->cInput->right = false;
          break;
        }
      }
    }
  }
}

void Game::sRender() {
  SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
  SDL_RenderClear(m_renderer);

  for (auto &entity : m_entities.getEntities()) {
    if (entity->cShape == nullptr) {
      continue;
    }

    SDL_Rect &rect = entity->cShape->rect;
    Vec2     &pos  = entity->cTransform->topLeftCornerPos;

    rect.x = static_cast<int>(pos.x);
    rect.y = static_cast<int>(pos.y);

    const RGBA &color = entity->cShape->color;
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(m_renderer, &rect);
  }

  // Update the screen
  SDL_RenderPresent(m_renderer);
}

void Game::sCollision() {
  const Vec2     windowSize = Vec2(1366, 768);
  std::bitset<4> collides   = CollisionHelpers::detectOutOfBounds(m_player, windowSize);
  CollisionHelpers::enforcePlayerBounds(m_player, collides, windowSize);

  for (auto &entity : m_entities.getEntities()) {
    if (entity->tag() == EntityTags::Player) {
      continue;
    }

    if (CollisionHelpers::calculateCollisionBetweenEntities(m_player, entity)) {
      m_score += 1;
      std::cout << "Player collided with enemy" << std::endl;
      std::cout << "Score: " << m_score << std::endl;
      entity->destroy();
    }
  }

  m_entities.update();
}

void Game::sMovement() {
  Vec2 playerVelocity = {0, 0};

  if (m_player->cInput == nullptr) {
    throw std::runtime_error("Player entity lacks an input component.");
  }

  if (m_player->cInput->forward) {
    playerVelocity.y = -1;
  }
  if (m_player->cInput->backward) {
    playerVelocity.y = 1;
  }
  if (m_player->cInput->left) {
    playerVelocity.x = -1;
  }
  if (m_player->cInput->right) {
    playerVelocity.x = 1;
  }

  for (auto e : m_entities.getEntities()) {
    if (e->cTransform == nullptr) {
      throw std::runtime_error("Entity " + e->tag() + ", with ID " + std::to_string(e->id()) +
                               " lacks a transform component.");
    }

    Vec2 &position = e->cTransform->topLeftCornerPos;

    if (e->tag() == EntityTags::Player) {
      position += playerVelocity * 2;
    }
  }
}
void Game::sSpawner() {
  const Uint32 ticks = SDL_GetTicks();
  if (ticks - m_lastEnemySpawnTime < 2500) {
    return;
  }
  m_lastEnemySpawnTime = ticks;
  spawnEnemy();
}

void Game::sLifespan() {
  for (auto &entity : m_entities.getEntities()) {

    if (entity->tag() == EntityTags::Player) {
      continue;
    }
    if (entity->cLifespan == nullptr) {
      std::cout << "Entity with ID " << entity->id() << " lacks a lifespan component"
                << std::endl;
      continue;
    }

    const Uint32 currentTime = SDL_GetTicks();

    const double lifespanPercentage = (currentTime - entity->cLifespan->birthTime) /
                                      static_cast<double>(entity->cLifespan->lifespan);

    if (currentTime - entity->cLifespan->birthTime > entity->cLifespan->lifespan) {
      std::cout << "Entity with ID " << entity->id() << " has expired" << std::endl;
      entity->destroy();
    }
  }
}

void Game::spawnPlayer() {
  m_player = m_entities.addEntity(EntityTags::Player);

  std::shared_ptr<CTransform> &playerCTransform = m_player->cTransform;
  std::shared_ptr<CShape>     &playerCShape     = m_player->cShape;
  std::shared_ptr<CInput>     &playerCInput     = m_player->cInput;

  playerCTransform = std::make_shared<CTransform>(Vec2(0, 0), Vec2(0, 0), 0);
  playerCShape     = std::make_shared<CShape>(m_renderer, m_playerConfig.shape);
  playerCInput     = std::make_shared<CInput>();

  std::cout << "Player entity created" << std::endl;

  m_entities.update();
}

void Game::spawnEnemy() {
  const Vec2 &windowSize = m_gameConfig.windowSize;
  const int   x          = rand() % static_cast<int>(windowSize.x);
  const int   y          = rand() % static_cast<int>(windowSize.y);

  std::shared_ptr<Entity>      enemy            = m_entities.addEntity(EntityTags::Enemy);
  std::shared_ptr<CTransform> &entityCTransform = enemy->cTransform;
  std::shared_ptr<CShape>     &entityCShape     = enemy->cShape;
  std::shared_ptr<CLifespan>  &entityLifespan   = enemy->cLifespan;

  entityCTransform = std::make_shared<CTransform>(Vec2(x, y), Vec2(0, 0), 0);
  entityCShape = std::make_shared<CShape>(m_renderer, ShapeConfig({40, 40, {255, 0, 0, 255}}));
  entityLifespan = std::make_shared<CLifespan>(30000);

  bool touchesBoundary = CollisionHelpers::detectOutOfBounds(enemy, windowSize).any();

  bool touchesOtherEntities = false;

  for (auto &entity : m_entities.getEntities()) {
    if (CollisionHelpers::calculateCollisionBetweenEntities(entity, enemy)) {
      touchesOtherEntities = true;
      break;
    }
  }

  bool      isValidSpawn       = !touchesBoundary && !touchesOtherEntities;
  const int MAX_SPAWN_ATTEMPTS = 10;
  int       spawnAttempt       = 1;

  while (!isValidSpawn && spawnAttempt < MAX_SPAWN_ATTEMPTS) {
    const int randomX = rand() % static_cast<int>(windowSize.x);
    const int randomY = rand() % static_cast<int>(windowSize.y);

    enemy->cTransform->topLeftCornerPos = Vec2(randomX, randomY);
    touchesBoundary      = CollisionHelpers::detectOutOfBounds(enemy, windowSize).any();
    touchesOtherEntities = false;

    for (auto &entity : m_entities.getEntities()) {
      if (CollisionHelpers::calculateCollisionBetweenEntities(entity, enemy)) {
        touchesOtherEntities = true;
        break;
      }
    }

    spawnAttempt += 1;
    isValidSpawn = !touchesBoundary && !touchesOtherEntities;
  }

  if (!isValidSpawn) {
    std::cout << "Could not spawn enemy after " << MAX_SPAWN_ATTEMPTS << " attempts"
              << std::endl;
    enemy->destroy();
  }

  m_entities.update();
}