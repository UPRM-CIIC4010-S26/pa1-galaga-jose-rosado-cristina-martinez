#include "Program.hpp"
#include <vector>
#include <utility>
#include <algorithm>

int score = 0;
int nextLifeScore = 1000;
int lives = 3;

struct FloatingText {
    Vector2 position;
    int value;
    int frames;
};

std::vector<FloatingText> floatingTexts;

Program::Program() {
    Background::sideWalls = { 
        HitBox(0, 0, 10, GetScreenHeight()), 
        HitBox(GetScreenWidth() - 10, 0, 10, GetScreenHeight())
    };

    Enemy::enemies.push_back({ {350, 150}, new SpEnemy(350, 150) });
    Enemy::enemies.push_back({ {600, 150}, new SpEnemy(600, 150) });

    for (int i = 0; i < 30; i++) {
        float x = 250 + 50 * (i % 10);
        float y = 200 + 50 * (i / 10);
        Enemy::enemies.push_back({ {x, y}, new StdEnemy(x, y) });
    }
}

void Program::Update() {
    for (Animation& a : Animation::animations) a.update();

    for (int i = Animation::animations.size() - 1; i >= 0; i--) {
        if (Animation::animations[i].done) {
            Animation::animations.erase(Animation::animations.begin() + i);
        }
    }

    pauseFrames = std::max(pauseFrames - 1, 0);

    if (!startup && !paused && !gameOver && pauseFrames <= 0) {
        Enemy::ManageEnemies(player->hitBox, score);
        StdEnemy::attackReset();
        ManageEnemyRespawns();
        player->update();

        for (Projectile& p : Projectile::projectiles) { 
            p.update(); 
            if(p.ID != 0 && HitBox::Collision(player->hitBox, p.getHitBox())){
                PlayerReset();
            }
        }

        Projectile::ProjectileCollision();

        for (auto &p : Enemy::enemies) {
            if (p.second && p.second->health <= 0) {
                int points = 0;
                if (dynamic_cast<StdEnemy*>(p.second)) points = 100;
                else if (dynamic_cast<SpEnemy*>(p.second)) points = 200;
                else if (dynamic_cast<StEnemy*>(p.second)) points = 150;
                else if (dynamic_cast<DyEnemy*>(p.second)) points = 300;

                score += points;

                floatingTexts.push_back({
                    Vector2{p.second->position.first, p.second->position.second},
                    points,
                    120
                });

                delete p.second;
                p.second = nullptr;
            }
        }

        if (score >= nextLifeScore) {
            if (lives < 5) lives++;
            nextLifeScore += 1000;
        }

        for (auto &p : Enemy::enemies) {
            if (p.second && HitBox::Collision(player->hitBox, p.second->hitBox)) {
                Animation::animations.push_back(
                    Animation(player->position.first, player->position.second, 16, 0, 33, 34, 30 ,30, 3, ImageManager::SpriteSheet)
                );
                PlaySound(SoundManager::gameOver);
                Projectile::projectiles.clear();
                player->position.first = GetScreenWidth() / 2 - 15;
                p.second->health = 0;
                pauseFrames = 120;
                lives--;
            }
        }

        if (lives <= 0 && pauseFrames <= 0) gameOver = true;

        Projectile::CleanProjectiles();
    }

    for (int i = floatingTexts.size() - 1; i >= 0; i--) {
        floatingTexts[i].position.y -= 1.0f;
        floatingTexts[i].frames--;
        if (floatingTexts[i].frames <= 0) {
            floatingTexts.erase(floatingTexts.begin() + i);
        }
    }
}

void Program::Draw() {
    background.Draw();
    if (pauseFrames <= 0 && !gameOver) player->draw();
    for (Animation& a : Animation::animations) a.draw();

    for (int i = 0; i < lives; i++) {
        DrawTexturePro(ImageManager::SpriteSheet, {0, 0, 17, 18}, 
            {10.0f + i * 30, (float)GetScreenHeight() - 30.0f, 20, 20}, 
            {0, 0}, 0, WHITE);
    }

    for (Projectile p : Projectile::projectiles) p.draw();
    for (auto &p : Enemy::enemies) if (p.second) p.second->draw();

    for (auto &t : floatingTexts) {
        DrawText(TextFormat("+%d", t.value), t.position.x, t.position.y, 20, YELLOW);
    }

    DrawText(TextFormat("Score: %1", score), GetScreenWidth() - 180, 20, 24, WHITE);

    if (startup) DrawStartup();
    if (paused) DrawPauseScreen();
    if (gameOver) DrawGameOver();
}

void Program::ManageEnemyRespawns() {
    delay = std::max(delay - 1, 0);
    respawnCooldown -= 1;

    if (respawnCooldown <= 0) {
        respawnCooldown = 1080;

        for (auto &p : Enemy::enemies) {
            if (!p.second && p.first.second != 150) {
                int eType = GetRandomValue(1, 3);
                if (eType == 1) p.second = new StEnemy(GetScreenWidth() / 2 - 15, 0, true);
                else p.second = new StdEnemy(GetScreenWidth() / 2 - 15, 0, true);
                respawns++;
                break;
            } else if (!p.second && p.first.second == 150) {
                p.second = new SpEnemy(GetScreenWidth() / 2 - 15, 0, true);
                respawns++;
                break;
            }
        }
    }

    if (respawns >= 4) {
        count = 4;
        respawns = 0;
    }

    if (count > 0 && delay <= 0) {
        Enemy::enemies.push_back({ {0, 0}, new DyEnemy(GetScreenWidth(), 300) });
        count--;
        delay = 20;
    }
}

void Program::DrawStartup() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0,0,0,125});
    DrawText("Galaga", (GetScreenWidth() / 2 - 237), 75, 144, WHITE);
    DrawText("Press Enter", (GetScreenWidth() / 2) - 75, GetScreenHeight() / 2, 24, GRAY);
}

void Program::DrawPauseScreen() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0,0,0,125});
    DrawText("Paused", (GetScreenWidth() / 2) - 85, GetScreenHeight() / 2 - 60, 48, WHITE);
    DrawText("Press Enter", (GetScreenWidth() / 2) - 75, GetScreenHeight() / 2, 24, GRAY);
}

void Program::DrawGameOver() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0,0,0,125});
    DrawText("Game Over", (GetScreenWidth() / 2) - 380, 50, 144, WHITE);
    DrawText("Press Enter", (GetScreenWidth() / 2) - 75, GetScreenHeight() / 2, 24, GRAY);
}

void Program::KeyInputs() {
    if ((!gameOver && !startup && IsKeyPressed('P')) || (paused && IsKeyPressed(KEY_ENTER))) paused = !paused;
    if (!paused && !startup && IsKeyPressed('O')) gameOver = !gameOver;
    if (!gameOver && !paused && IsKeyPressed('I')) startup = !startup;
    if (IsKeyPressed('H')) HitBox::drawHitbox = !HitBox::drawHitbox;

    if (gameOver && IsKeyPressed(KEY_ENTER)) {
        gameOver = false;
        Reset();
    }

    if (startup && IsKeyPressed(KEY_ENTER)) startup = false;

    if (!startup && !paused && !gameOver && pauseFrames <= 0) player->keyInputs();
}

void Program::PlayerReset() {
    Animation::animations.push_back(
        Animation(player->position.first, player->position.second, 16, 0, 33, 34, 30 ,30, 3, ImageManager::SpriteSheet)
    );
    PlaySound(SoundManager::gameOver);
    Projectile::projectiles.clear();
    player->position.first = GetScreenWidth() / 2 - 15;
    pauseFrames = 120;
    lives--;
}

void Program::Reset() {
    Enemy::enemies.clear();
    StdEnemy::attackInProgress = false;
    player = new Player((GetScreenWidth() / 2) - 15, GetScreenHeight() * 0.75f);
    respawnCooldown = 1080;
    respawns = 0;
    count = 0;
    delay = 0;
    lives = 3;

    score = 0;
    nextLifeScore = 1000;

    Enemy::enemies.push_back({{350, 150}, new SpEnemy(350, 150)});
    Enemy::enemies.push_back({{600, 150}, new SpEnemy(600, 150)});

    for (int i = 0; i < 30; i++) {
        float x = 250 + 50 *(i%10);
        float y = 200 + 50 * (i/10);
        Enemy::enemies.push_back({{x, y}, new StdEnemy(x, y)});
    }
}
    
