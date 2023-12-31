#pragma once
#include "Play3D.h"
#include "UtilityFunctions.h"

// The GameObject type is the only representation of type which is visible to code externally
enum GameObjectType
{
	TYPE_NULL = -1,

	// Player objects
	TYPE_PLAYER,
	TYPE_PLAYER_PELLET,
	TYPE_PLAYER_CHUNK_CORE,
	TYPE_PLAYER_CHUNK_WING_L,
	TYPE_PLAYER_CHUNK_WING_R,

	// Enemy objects
	TYPE_BOSS,
	TYPE_BOSS_PELLET,
	TYPE_BOSS_BOMB,
	TYPE_BOSS_CHUNK_CORE,
	TYPE_BOSS_CHUNK_LEFT,
	TYPE_BOSS_CHUNK_RIGHT,
	TYPE_BOSS_CHUNK_LOWER,

	// Misc. objects
	TYPE_ASTEROID,

	TYPE_TOTAL
};

enum CollisionMode
{
	COLL_RADIAL,
	COLL_RECT,
};

struct CollisionData
{
	Play3d::Vector2f extents{0.f, 0.f};
	Play3d::Vector2f offset{0.f, 0.f};
	float radius{1.f};
	CollisionMode type{ CollisionMode::COLL_RADIAL };
};

class GameObject
{
public:
	// Constructors and (virtual) destructor
	GameObject() = delete; // Delete the default constructor as we don't want unmanaged GameObjects
	GameObject(GameObjectType objType, Play3d::Vector3f position);
	virtual ~GameObject() {}; 
	
	// Providing no implementation of the virtual Update function makes this an abstract base class
	virtual void Update() = 0;
	virtual void Draw() const;
	virtual void DrawCollision() const;
	// These virtuals have implementations and so are optional overrides
	virtual void OnCollision( GameObject* ) {};

	// Various tests on game objects
	bool IsDestroyed() { return m_destroy; }
	bool IsHidden() { return m_hidden; }
	bool IsColliding(GameObject* obj);
	bool IsOutsideOrthoView();

	// Standard updates and destruction flagging
	void StandardMovementUpdate();
	void UpdateAnimation();
	void Destroy();

	// Setters
	void SetVelocity(Play3d::Vector3f velocity) { m_velocity = velocity; }
	void SetAcceleration(Play3d::Vector3f acceleration) { m_acceleration = acceleration; }
	void SetPosition(Play3d::Vector3f pos ) { m_pos = pos; }
	void SetAnimationSpeed( float animationSpeed ) { m_animSpeed = animationSpeed; }
	void SetRotation( Play3d::Vector3f rotation ) { m_rotation = rotation; }
	void SetRotationSpeed( Play3d::Vector3f rotationSpeed ) { m_rotSpeed = rotationSpeed; }
	void SetFrame( float frame ) { m_frame = frame; }
	void SetHidden( bool hidden ) { m_hidden = hidden; }

	// Getters
	GameObjectType GetObjectType() { return m_type; }
	Play3d::Vector3f GetPosition() { return m_pos; }
	Play3d::Vector3f GetVelocity() { return m_velocity; }
	Play3d::Vector3f GetAcceleration() { return m_acceleration; }
	Play3d::Vector3f GetRotation() { return m_rotation; }

protected:
	// Mostly just adapted from Play::GameObject
	GameObjectType m_type{ GameObjectType::TYPE_NULL };

	Play3d::Graphics::MeshId m_meshId{};
	Play3d::Graphics::MaterialId m_materialId{};

	Play3d::Vector3f m_pos{ 0.f, 0.f, 0.f };
	Play3d::Vector3f m_oldPos{ 0.f, 0.f, 0.f };
	Play3d::Vector3f m_velocity{ 0.f, 0.f, 0.f };
	Play3d::Vector3f m_acceleration{ 0.f, 0.f, 0.f };

	Play3d::Vector3f m_rotation{ 0.f, 0.f, 0.f };
	Play3d::Vector3f m_rotSpeed{ 0.f, 0.f, 0.f };
	float m_scale{1.f};
	
	// Optionally supporting multiple collision bounds per object for complex shapes
	std::vector<CollisionData> m_colliders;

	float m_frame{ -1 };
	float m_frameTimer{ 0 };
	float m_animSpeed{ 1 }; 

	bool m_destroy{ false };
	bool m_hidden{ false };
	bool m_canCollide{ true };
};