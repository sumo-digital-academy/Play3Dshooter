#pragma once
#include "GameObject.h"

class ObjectPellet : public GameObject
{
public:
	ObjectPellet(Play3d::Vector3f position);

	void Update() override;
	void OnCollision(GameObject* other) override;

private:
};

