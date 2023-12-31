#include "ObjectPlayer.h"
#include "ObjectManager.h"
#include "GameHud.h"
using namespace Play3d;

static constexpr float SHIP_HALFWIDTH{0.15f};
static constexpr float COOLDOWN_RESPAWN{2.f};
static constexpr float PLAYER_INVINCIBILITY_TIME{1.5f};

static constexpr float MAX_SPEED{.1f};
static constexpr float STEER_SPEED_X{.6f};
static constexpr float STEER_SPEED_Y{.5f};

static constexpr float COOLDOWN_DOUBLE_TAP{0.2f};
static constexpr float BARREL_ROLL_TIME{0.25f};
static constexpr float BARREL_ROLL_SPEED{MAX_SPEED * 3};

static constexpr float DELAY_AUTOFIRE{0.05f};
static constexpr float DELAY_ROLL{1.f};

static constexpr float SPIN_SPEED{1.f};
static constexpr float MAX_ROT_SPEED{.1f};
static constexpr float MAX_ROT_X{kfQuartPi / 2.f};
static constexpr float MAX_ROT_Y{kfQuartPi / 4.f};

static const Vector2f MIN_POS{-9.f, -7.f};
static const Vector2f MAX_POS{9.f, 5.f};

ObjectPlayer::ObjectPlayer(Vector3f position) : GameObject(TYPE_PLAYER, position)
{
	GameObjectManager* pObjs{ GetObjectManager() };

	// Load and assign player mesh
	m_meshId = pObjs->GetMesh("..\\Assets\\Models\\_fighter.obj");
	m_materialId = pObjs->GetMaterial("..\\Assets\\Models\\_fighter-blue.jpg");

	// Ensure player chunks are preloaded to avoid lag on first death
	pObjs->GetMesh("..\\Assets\\Models\\_fighter-chunk-core.obj");
	pObjs->GetMesh("..\\Assets\\Models\\_fighter-chunk-wingL.obj");
	pObjs->GetMesh("..\\Assets\\Models\\_fighter-chunk-wingR.obj");

	m_sfxDeath[0] = pObjs->GetAudioId("..\\Assets\\Audio\\Death1.wav");
	m_sfxDeath[1] = pObjs->GetAudioId("..\\Assets\\Audio\\Death2.wav");
	m_sfxDeath[2] = pObjs->GetAudioId("..\\Assets\\Audio\\Death3.wav");

	ParticleEmitterSettings s;
	s.capacity = 100;
	s.emitterMinExtents = Vector3f(-0.05f, 0.f, -0.f);
	s.emitterMaxExtents = Vector3f(0.05f, 0.f, 0.f);
	s.particleMinVelocity = Vector3f(-0.f, -8.f, 0.f);
	s.particleMaxVelocity = Vector3f(0.f, -0.5f, 0.f);
	s.emitWaitMin = 0.01f;
	s.emitWaitMax = 0.01f;
	s.particlesRelativeToEmitter = false;
	s.particleLifetime = .05f;
	s.particlesPerEmit = 8;
	s.particleColour = Colour::Orange;

	m_emitterLeftThruster.ApplySettings(s);
	m_emitterRightThruster.ApplySettings(s);

	m_colliders[0].radius = 0.4f;
	m_colliders[0].offset.y = -0.1f;
}

void ObjectPlayer::Update()
{
	m_invincibilityTimer -= Play3d::System::GetDeltaTime();

	if (m_bIsAlive)
	{
		HandleControls();
	}
	else
	{
		m_respawnCooldown -= System::GetDeltaTime();
		if (m_respawnCooldown <= 0.f)
		{
			Respawn();
		}
	}
}

void ObjectPlayer::Respawn()
{
	if(m_lives > 0)
	{
		m_lives--;
		m_pos = Vector3f(0.f, -GetGameHalfHeight() / 1.25f, 0.f);
		m_velocity = Vector3f(0.f, 0.f, 0.f);
		m_rotation = Vector3f(0.f, 0.f, 0.f);
		m_bDoubleTapLeft = false;
		m_bDoubleTapRight = false;
		m_bIsBarrelRoll = false;
		m_shootCooldown = 0.f;
		m_rollCooldown = 0.f;
		m_respawnCooldown = 0.f;

		m_emitterLeftThruster.DestroyAll();
		m_emitterRightThruster.DestroyAll();

		SetHidden(false);
		m_canCollide = true;
		m_bIsAlive = true;

		m_invincibilityTimer = PLAYER_INVINCIBILITY_TIME;

		GameHud::Get()->SetLives(m_lives);
	}
	else if(m_lives == 0)
	{
		Audio::PlaySound(GetObjectManager()->GetAudioId("..\\Assets\\Audio\\GameOver.wav"), 3.5f);
		m_lives = -1;
	}
}

void ObjectPlayer::HandleControls()
{
	float deltaTime = System::GetDeltaTime();

	// FIRE
	m_shootCooldown -= deltaTime;
	if (Input::IsKeyDown(VK_SPACE))
	{
		if (m_shootCooldown < 0)
		{
			m_shootCooldown = DELAY_AUTOFIRE;

			GameObject* pPellet = GetObjectManager()->CreateObject(TYPE_PLAYER_PELLET, m_pos + Vector3f(0.f, .33f, 0.f));
			pPellet->SetVelocity(Vector3f(0.f, .2f, 0.f));
		}
	}
	else
	{
		m_shootCooldown = 0.f; // allow rapid-fire when spamming shoot button
	}

	// BOMB
	if (Input::IsKeyPressed(VK_SHIFT) && m_bombs >= 1)
	{
		m_bombs--;
		GameHud::Get()->SetBombs(m_bombs);

		std::vector<GameObject*> pProjectiles;
		GetObjectManager()->GetAllObjectsOfType(TYPE_BOSS_PELLET, pProjectiles);
		GetObjectManager()->GetAllObjectsOfType(TYPE_BOSS_BOMB, pProjectiles, false); // false => results will append to vector
		for (int i = 0; i < pProjectiles.size(); i++)
		{
			pProjectiles[i]->Destroy();
		}
	}

	// STEER - VERTICAL
	if (Input::IsKeyDown('W'))
	{
		m_velocity.y = std::min(m_velocity.y + (STEER_SPEED_Y * deltaTime), MAX_SPEED);

		if (m_rotSpeed.x < 0)
		{
			m_rotSpeed.x *= 0.7f;
		}
		m_rotSpeed.x = std::min(m_rotSpeed.x + (SPIN_SPEED * deltaTime), MAX_ROT_SPEED);
	}
	else if (Input::IsKeyDown('S'))
	{
		m_velocity.y = std::max(m_velocity.y - (STEER_SPEED_Y * deltaTime), -MAX_SPEED);

		if (m_rotSpeed.x > 0)
		{
			m_rotSpeed.x *= 0.7f;
		}
		m_rotSpeed.x = std::max(m_rotSpeed.x - (SPIN_SPEED * deltaTime), -MAX_ROT_SPEED);
	}
	else
	{
		m_rotSpeed.x *= 0.86f;	// reduce spin
		m_rotation.x *= 0.86f;	// reduce angle
		m_velocity.y *= 0.9f;	// reduce speed
	}

	// STEER - HORIZONTAL
	if (Input::IsKeyDown('A'))
	{
		float thrust = std::min(m_velocity.x + (STEER_SPEED_X * deltaTime), MAX_SPEED);
		m_velocity.x = std::max(m_velocity.x, thrust); // don't clamp velocity if already above max-speed (barrel rolls)

		// reduce rotation faster when coming from opposing direction
		if (m_rotSpeed.y > 0)
		{
			m_rotSpeed.y *= 0.7f;
		}
		m_rotSpeed.y = std::max(m_rotSpeed.y - (SPIN_SPEED * deltaTime), -MAX_ROT_SPEED);
	}
	else if (Input::IsKeyDown('D'))
	{
		float thrust = std::max(m_velocity.x - (STEER_SPEED_X * deltaTime), -MAX_SPEED);
		m_velocity.x = std::min(m_velocity.x, thrust); // don't clamp velocity if already above max-speed (barrel rolls)

		// reduce rotation faster when coming from opposing direction
		if (m_rotSpeed.y < 0)
		{
			m_rotSpeed.y *= 0.7f;
		}
		m_rotSpeed.y = std::min(m_rotSpeed.y + (SPIN_SPEED * deltaTime), MAX_ROT_SPEED);
	}
	else if (!m_bIsBarrelRoll)
	{
		// Reduce all forces when not actively steering/rolling
		m_rotSpeed.y *= 0.86f; // reduce spin
		m_rotation.y *= 0.86f; // reduce current angle
		m_velocity.x *= 0.86f; // reduce speed
	}
	m_rotation.y = std::clamp(m_rotation.y, -MAX_ROT_X, MAX_ROT_X);
	m_rotation.x = std::clamp(m_rotation.x, -MAX_ROT_Y, MAX_ROT_Y);

	// BARREL ROLL - Cooldown
	m_rollCooldown -= deltaTime;
	if (m_rollCooldown <= 0.f)
	{
		m_bDoubleTapLeft = false;
		m_bDoubleTapRight = false;
		m_bIsBarrelRoll = false;
	}

	// BARREL ROLL - Execute
	if (m_bIsBarrelRoll)
	{
		float rollCompletion = 1 - (m_rollCooldown / BARREL_ROLL_TIME); // 0.f to 1.f: start to finish
		m_rotation.y = (m_bDoubleTapLeft ? -kfTwoPi : kfTwoPi) * rollCompletion;
	}
	if (!m_bIsBarrelRoll)
	{
		// Barrel rolls grant velocity greater than max speed, slowly align to max_speed after exiting rolls
		if (abs(m_velocity.x) > MAX_SPEED)
		{
			m_velocity.x *= 0.97f;
		}
	}

	// BARREL ROLL - Trigger
	if (!m_bIsBarrelRoll && Input::IsKeyPressed('A'))
	{
		if (m_bDoubleTapLeft && m_rollCooldown > 0.f)
		{
			// Double tap: Initiate Roll!
			m_velocity.x = BARREL_ROLL_SPEED;
			m_rotation.y = MAX_ROT_X;
			m_bIsBarrelRoll = true;
			m_rollCooldown = BARREL_ROLL_TIME;
		}
		else
		{
			// First press: Wait for double tap
			m_bDoubleTapLeft = true;
			m_bDoubleTapRight = false;
			m_rollCooldown = COOLDOWN_DOUBLE_TAP;
		}
	}
	else if (!m_bIsBarrelRoll && Input::IsKeyPressed('D'))
	{
		if (m_bDoubleTapRight && m_rollCooldown > 0.f)
		{
			// Double tap: Initiate Roll!
			m_velocity.x = -BARREL_ROLL_SPEED;
			m_rotation.y = -MAX_ROT_X;
			m_bIsBarrelRoll = true;
			m_rollCooldown = BARREL_ROLL_TIME;
		}
		else
		{
			// First press: Wait for double tap
			m_bDoubleTapLeft = false;
			m_bDoubleTapRight = true;
			m_rollCooldown = COOLDOWN_DOUBLE_TAP;
		}
	}

	// Thruster particle rotation about ship
	float c = cos(-m_rotation.y);
	float s = sin(-m_rotation.y);
	Vector3f thrusterLeftOffset{ SHIP_HALFWIDTH, -SHIP_HALFWIDTH * 2, 0.f };
	Vector3f thrusterRightOffset{ -SHIP_HALFWIDTH, -SHIP_HALFWIDTH * 2, 0.f };
	Vector3f temp = thrusterLeftOffset;
	thrusterLeftOffset.x = (temp.x * c) - (temp.z * s);
	thrusterLeftOffset.z = (temp.z * c) + (temp.x * s);
	temp = thrusterRightOffset;
	thrusterRightOffset.x = (temp.x * c) - (temp.z * s);
	thrusterRightOffset.z = (temp.z * c) + (temp.x * s);
	m_emitterLeftThruster.m_position = m_pos + thrusterLeftOffset;
	m_emitterLeftThruster.Tick();
	m_emitterRightThruster.m_position = m_pos + thrusterRightOffset;
	m_emitterRightThruster.Tick();

	// Enforce limits
	Vector3f cachedPos = m_pos;
	m_pos.x = std::clamp(m_pos.x, MIN_POS.x, MAX_POS.x);
	m_pos.y = std::clamp(m_pos.y, MIN_POS.y, MAX_POS.y);
	if (cachedPos.x != m_pos.x)
	{
		if (!m_bIsBarrelRoll)
		{
			m_velocity.x = 0.f;
		}
	}
	if (cachedPos.y != m_pos.y)
	{
		m_velocity.y = 0.f;
	}
}

void ObjectPlayer::OnCollision(GameObject* other)
{
	if (m_invincibilityTimer >= 0.f)
	{
		return;
	}

	if(other->GetObjectType() == GameObjectType::TYPE_BOSS_PELLET || other->GetObjectType() == GameObjectType::TYPE_BOSS)
	{
		// Die > if lives remain, start respawn timer, else gameover
		SetHidden(true);
		m_canCollide = false;
		m_bIsAlive = false;
		m_respawnCooldown = COOLDOWN_RESPAWN;

		GameObjectManager* pObjs{GetObjectManager()};
		GameObject* pChunk;

		pChunk = pObjs->CreateObject(TYPE_PLAYER_CHUNK_CORE, m_pos);
		pChunk->SetVelocity((m_velocity / 8.f) + Vector3f(0.f, 0.01f, 0.f));
		pChunk->SetRotationSpeed(m_rotation / 8.f);

		pChunk = pObjs->CreateObject(TYPE_PLAYER_CHUNK_WING_L, m_pos);
		pChunk->SetVelocity((m_velocity / 8.f) + Vector3f(0.01f, -0.01f, 0.f));
		pChunk->SetRotationSpeed(-m_rotation / 8.f);

		pChunk = pObjs->CreateObject(TYPE_PLAYER_CHUNK_WING_R, m_pos);
		pChunk->SetVelocity((m_velocity / 8.f) + Vector3f(-0.01f, -0.01f, 0.f));
		pChunk->SetRotationSpeed(-m_rotation / 8.f);

		int sfxId = std::floor(RandValueInRange(0.f, SFX_DEATH_SLOTS));
		Audio::PlaySound(m_sfxDeath[sfxId]);
	}
}

void ObjectPlayer::Draw() const
{
	GameObject::Draw();
	m_emitterLeftThruster.Draw();
	m_emitterRightThruster.Draw();
}